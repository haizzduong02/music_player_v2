#ifndef IHARDWARE_INTERFACE_H
#define IHARDWARE_INTERFACE_H

#include "ISubject.h"
#include <string>

/**
 * @file IHardwareInterface.h
 * @brief Interface for hardware communication (Dependency Inversion Principle)
 * 
 * Abstracts communication with hardware devices (S32K144 via UART).
 * Extends ISubject to notify observers of hardware events.
 */

/**
 * @brief Hardware command types
 */
enum class HardwareCommand {
    PLAY,
    PAUSE,
    NEXT,
    PREVIOUS,
    VOLUME_CHANGE,
    UNKNOWN
};

/**
 * @brief Hardware event structure
 */
struct HardwareEvent {
    HardwareCommand command;
    float value;  // For VOLUME_CHANGE, value is 0.0-1.0
};

/**
 * @brief Hardware interface
 * 
 * Provides methods for communicating with hardware devices.
 * Notifies observers when hardware events occur (button presses, ADC changes).
 */
class IHardwareInterface : public ISubject {
public:
    virtual ~IHardwareInterface() = default;
    
    /**
     * @brief Initialize hardware connection
     * @param port Serial port path (e.g., "/dev/ttyUSB0")
     * @param baudRate Baud rate for UART communication
     * @return true if connection successful
     */
    virtual bool initialize(const std::string& port, int baudRate) = 0;
    
    /**
     * @brief Close hardware connection
     */
    virtual void close() = 0;
    
    /**
     * @brief Check if hardware is connected
     * @return true if connected
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Send a command to hardware
     * @param command Command string to send
     * @return true if sent successfully
     */
    virtual bool sendCommand(const std::string& command) = 0;
    
    /**
     * @brief Read data from hardware
     * @return Data string received
     */
    virtual std::string readData() = 0;
    
    /**
     * @brief Send volume value to hardware
     * @param volume Volume level (0.0 to 1.0)
     */
    virtual void sendVolume(float volume) = 0;
    
    /**
     * @brief Display text on LCD (if hardware supports)
     * @param text Text to display
     */
    virtual void displayText(const std::string& text) = 0;
    
    /**
     * @brief Read ADC value from hardware (for volume control)
     * @return ADC value normalized to 0.0-1.0
     */
    virtual float readADC() = 0;
    
    /**
     * @brief Get the last hardware event received
     * @return Last hardware event
     */
    virtual HardwareEvent getLastEvent() const = 0;
    
    /**
     * @brief Start listening for hardware events (runs in separate thread)
     */
    virtual void startListening() = 0;
    
    /**
     * @brief Stop listening for hardware events
     */
    virtual void stopListening() = 0;
};

#endif // IHARDWARE_INTERFACE_H
