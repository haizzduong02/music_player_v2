#ifndef S32K144_INTERFACE_H
#define S32K144_INTERFACE_H

#include "interfaces/IHardwareInterface.h"
#include "utils/Subject.h"
#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

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
class S32K144Interface : public IHardwareInterface, public Subject
{
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
    bool initialize(const std::string &port, int baudRate) override;
    void close() override;
    bool isConnected() const override;
    bool sendCommand(const std::string &command) override;
    std::string readData() override;
    void sendVolume(float volume) override;
    void displayText(const std::string &text) override;
    float readADC() override;
    int readButton() override;
    HardwareEvent getLastEvent() const override;
    void startListening() override;
    void stopListening() override;

    // ISubject implementation (inherited from Subject)
    // ISubject implementation (resolving diamond inheritance)
    void attach(IObserver *observer) override { Subject::attach(observer); }
    void detach(IObserver *observer) override { Subject::detach(observer); }
    void notify() override
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto *observer : observers_)
        {
            if (observer)
            {
                observer->update(this);
            }
        }
    }

  private:
    int socketFd_; // Socket file descriptor
    std::string ipAddress_;
    int port_;
    std::atomic<bool> connected_;
    std::atomic<bool> listening_;
    std::atomic<int> currentButton_;
    std::atomic<float> currentAdc_;

    std::thread listenerThread_;
    std::thread connectThread_;
    mutable std::mutex mutex_;
    
    std::atomic<bool> running_;
    std::atomic<bool> connecting_;
    
    bool connect();
    void connectLoop();

    std::string receiveBuffer_;
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
    HardwareEvent parseData(const std::string &data);

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
    int sendRaw(const std::string &data);

    /**
     * @brief Read raw data from serial port
     * @param buffer Output buffer
     * @param size Buffer size
     * @return Number of bytes read
     */
    int readRaw(char *buffer, size_t size);
};

#endif // S32K144_INTERFACE_H
