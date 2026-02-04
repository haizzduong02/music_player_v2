#include "../../inc/interfaces/IHardwareInterface.h"
#include "../../inc/utils/Logger.h"
#include <thread>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <algorithm>

/**
 * @file LinuxHardware.cpp
 * @brief Linux/POSIX implementation of hardware interface for production use
 * 
 * This implementation provides real UART communication with S32K144 hardware
 * for the production music player system.
 */

class LinuxHardware : public IHardwareInterface {
private:
    int serialFd_;
    bool connected_;
    bool listening_;
    std::thread listenerThread_;
    HardwareEvent lastEvent_;
    std::string port_;
    std::vector<IObserver*> observers_;
    std::mutex observerMutex_;
    
public:
    LinuxHardware() 
        : serialFd_(-1), 
          connected_(false), 
          listening_(false),
          lastEvent_{HardwareCommand::UNKNOWN, 0.0f} {
    }
    
    ~LinuxHardware() override {
        stopListening();
        close();
    }
    
    // ISubject interface implementation
    void attach(IObserver* observer) override {
        std::lock_guard<std::mutex> lock(observerMutex_);
        if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
            observers_.push_back(observer);
        }
    }
    
    void detach(IObserver* observer) override {
        std::lock_guard<std::mutex> lock(observerMutex_);
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }
    
    void notify() override {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto observer : observers_) {
            if (observer) {
                observer->update(this);
            }
        }
    }
    
    bool initialize(const std::string& port, int baudRate) override {
        port_ = port;
        
        // Open serial port
        serialFd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (serialFd_ == -1) {
            Logger::getInstance().error("Failed to open serial port: " + port);
            return false;
        }
        
        // Configure serial port
        struct termios options;
        tcgetattr(serialFd_, &options);
        
        // Set baud rate
        speed_t speed;
        switch (baudRate) {
            case 9600: speed = B9600; break;
            case 19200: speed = B19200; break;
            case 38400: speed = B38400; break;
            case 57600: speed = B57600; break;
            case 115200: speed = B115200; break;
            default: speed = B115200; break;
        }
        
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);
        
        // 8N1 configuration
        options.c_cflag &= ~PARENB;  // No parity
        options.c_cflag &= ~CSTOPB;  // 1 stop bit
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;       // 8 data bits
        options.c_cflag |= (CLOCAL | CREAD);
        
        // Raw input
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY);
        options.c_oflag &= ~OPOST;
        
        // Apply settings
        tcsetattr(serialFd_, TCSANOW, &options);
        tcflush(serialFd_, TCIOFLUSH);
        
        connected_ = true;
        Logger::getInstance().info("Hardware initialized on port: " + port + " at " + std::to_string(baudRate) + " baud");
        return true;
    }
    
    void close() override {
        if (serialFd_ != -1) {
            ::close(serialFd_);
            serialFd_ = -1;
        }
        connected_ = false;
        Logger::getInstance().info("Hardware connection closed");
    }
    
    bool isConnected() const override {
        return connected_;
    }
    
    bool sendCommand(const std::string& command) override {
        if (!connected_ || serialFd_ == -1) {
            return false;
        }
        
        std::string cmdWithNewline = command + "\n";
        ssize_t written = write(serialFd_, cmdWithNewline.c_str(), cmdWithNewline.length());
        
        if (written < 0) {
            Logger::getInstance().error("Failed to send command: " + command);
            return false;
        }
        
        return true;
    }
    
    std::string readData() override {
        if (!connected_ || serialFd_ == -1) {
            return "";
        }
        
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytesRead = read(serialFd_, buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            return std::string(buffer, bytesRead);
        }
        
        return "";
    }
    
    void sendVolume(float volume) override {
        // Clamp volume to 0.0-1.0
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        
        // Send volume command to hardware
        int volumePercent = static_cast<int>(volume * 100);
        std::string cmd = "VOL:" + std::to_string(volumePercent);
        sendCommand(cmd);
    }
    
    void displayText(const std::string& text) override {
        // Send display command to hardware LCD
        std::string cmd = "LCD:" + text;
        sendCommand(cmd);
    }
    
    float readADC() override {
        // Request ADC value from hardware
        sendCommand("ADC?");
        
        // Wait briefly for response
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::string response = readData();
        if (!response.empty() && response.find("ADC:") == 0) {
            try {
                int adcValue = std::stoi(response.substr(4));
                // Assuming 12-bit ADC (0-4095)
                return static_cast<float>(adcValue) / 4095.0f;
            } catch (...) {
                return 0.0f;
            }
        }
        
        return 0.0f;
    }
    
    HardwareEvent getLastEvent() const override {
        return lastEvent_;
    }
    
    void startListening() override {
        if (listening_) {
            return;
        }
        
        listening_ = true;
        listenerThread_ = std::thread(&LinuxHardware::listenerLoop, this);
        Logger::getInstance().info("Hardware listener started");
    }
    
    void stopListening() override {
        if (!listening_) {
            return;
        }
        
        listening_ = false;
        if (listenerThread_.joinable()) {
            listenerThread_.join();
        }
        Logger::getInstance().info("Hardware listener stopped");
    }
    
private:
    void listenerLoop() {
        while (listening_ && connected_) {
            std::string data = readData();
            
            if (!data.empty()) {
                parseHardwareData(data);
            }
            
            // Small delay to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    void parseHardwareData(const std::string& data) {
        // Parse hardware commands
        if (data.find("BTN:PLAY") != std::string::npos) {
            lastEvent_ = {HardwareCommand::PLAY, 0.0f};
            notify();
        }
        else if (data.find("BTN:PAUSE") != std::string::npos) {
            lastEvent_ = {HardwareCommand::PAUSE, 0.0f};
            notify();
        }
        else if (data.find("BTN:NEXT") != std::string::npos) {
            lastEvent_ = {HardwareCommand::NEXT, 0.0f};
            notify();
        }
        else if (data.find("BTN:PREV") != std::string::npos) {
            lastEvent_ = {HardwareCommand::PREVIOUS, 0.0f};
            notify();
        }
        else if (data.find("VOL:") != std::string::npos) {
            try {
                int volPercent = std::stoi(data.substr(4));
                float volume = static_cast<float>(volPercent) / 100.0f;
                lastEvent_ = {HardwareCommand::VOLUME_CHANGE, volume};
                notify();
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }
};

// Factory function for creating LinuxHardware instance
extern "C" IHardwareInterface* createHardwareInterface() {
    return new LinuxHardware();
}
