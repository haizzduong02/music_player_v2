#ifndef S32K144_INTERFACE_H
#define S32K144_INTERFACE_H

#include "interfaces/IHardwareInterface.h"
#include "utils/Subject.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

/**
 * @file S32K144Interface.h
 * @brief Concrete implementation of IHardwareInterface for S32K144 board
 * 
 * Provides UART communication with S32K144 microcontroller.
 * Runs listener thread to handle hardware events.
 */

/**
 * @brief S32K144 hardware interface class
 * 
 * Concrete implementation of IHardwareInterface.
 * Communicates via UART and notifies observers of hardware events.
 * Extends Subject to implement Observer pattern.
 */
class S32K144Interface : public IHardwareInterface, public Subject {
public:
    /**
     * @brief Constructor
     */
    S32K144Interface();
    
    /**
     * @brief Destructor - cleanup resources
     */
    ~S32K144Interface() override;
    
    // IHardwareInterface implementation
    bool initialize(const std::string& port, int baudRate) override;
    void close() override;
    bool isConnected() const override;
    bool sendCommand(const std::string& command) override;
    std::string readData() override;
    void sendVolume(float volume) override;
    void displayText(const std::string& text) override;
    float readADC() override;
    HardwareEvent getLastEvent() const override;
    void startListening() override;
    void stopListening() override;
    
    // ISubject implementation (inherited from Subject)
    using Subject::attach;
    using Subject::detach;
    using Subject::notify;
    
private:
    int serialFd_;  // Serial port file descriptor
    std::string portName_;
    int baudRate_;
    std::atomic<bool> connected_;
    std::atomic<bool> listening_;
    
    std::thread listenerThread_;
    mutable std::mutex mutex_;
    
    HardwareEvent lastEvent_;
    std::queue<HardwareEvent> eventQueue_;
    
    /**
     * @brief Listener thread function
     */
    void listenerLoop();
    
    /**
     * @brief Parse received data into hardware event
     * @param data Received data string
     * @return Parsed hardware event
     */
    HardwareEvent parseData(const std::string& data);
    
    /**
     * @brief Configure serial port
     * @param fd File descriptor
     * @param baudRate Baud rate
     * @return true if configured successfully
     */
    bool configureSerial(int fd, int baudRate);
    
    /**
     * @brief Send raw data to serial port
     * @param data Data to send
     * @return Number of bytes sent
     */
    int sendRaw(const std::string& data);
    
    /**
     * @brief Read raw data from serial port
     * @param buffer Output buffer
     * @param size Buffer size
     * @return Number of bytes read
     */
    int readRaw(char* buffer, size_t size);
};

#endif // S32K144_INTERFACE_H
