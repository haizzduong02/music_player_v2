#ifndef USB_CONTROLLER_H
#define USB_CONTROLLER_H

#include "interfaces/IFileSystem.h"
#include "utils/Subject.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

/**
 * @file USBController.h
 * @brief Controller for USB device management
 * 
 * Handles USB hotplug detection and mounting.
 * Runs in separate thread to detect USB events.
 */

/**
 * @brief USB event structure
 */
struct USBEvent {
    enum Type {
        CONNECTED,
        DISCONNECTED
    };
    
    Type type;
    std::string device;
    std::string mountPoint;
};

/**
 * @brief USB controller class
 * 
 * Monitors USB hotplug events and manages mounting.
 * Extends Subject to notify observers of USB events.
 */
class USBController : public Subject {
public:
    /**
     * @brief Constructor with dependency injection
     * @param fileSystem File system interface
     */
    explicit USBController(IFileSystem* fileSystem);
    
    /**
     * @brief Destructor - stops monitoring
     */
    ~USBController();
    
    /**
     * @brief Start USB monitoring thread
     */
    void startMonitoring();
    
    /**
     * @brief Stop USB monitoring thread
     */
    void stopMonitoring();
    
    /**
     * @brief Manually detect USB devices
     * @return Vector of detected device paths
     */
    std::vector<std::string> detectUSB();
    
    /**
     * @brief Mount a USB device
     * @param device Device path
     * @param mountPoint Mount point
     * @return true if mounted successfully
     */
    bool mountUSB(const std::string& device, const std::string& mountPoint);
    
    /**
     * @brief Unmount a USB device
     * @param mountPoint Mount point to unmount
     * @return true if unmounted successfully
     */
    bool unmountUSB(const std::string& mountPoint);
    
    /**
     * @brief Scan USB for media files
     * @param mountPoint USB mount point
     * @param extensions Supported extensions
     * @return Vector of media file paths
     */
    std::vector<std::string> scanUSBMedia(
        const std::string& mountPoint,
        const std::vector<std::string>& extensions);
    
    /**
     * @brief Get currently mounted USB devices
     * @return Vector of mount points
     */
    std::vector<std::string> getMountedDevices() const {
        return mountedDevices_;
    }
    
    /**
     * @brief Check if monitoring is active
     * @return true if monitoring
     */
    bool isMonitoring() const {
        return monitoring_;
    }
    
    /**
     * @brief Get last USB event
     * @return Last event
     */
    USBEvent getLastEvent() const {
        return lastEvent_;
    }
    
private:
    IFileSystem* fileSystem_;
    std::vector<std::string> mountedDevices_;
    std::thread monitorThread_;
    std::atomic<bool> monitoring_;
    USBEvent lastEvent_;
    
    /**
     * @brief Monitoring thread function
     */
    void monitorLoop();
};

#endif // USB_CONTROLLER_H
