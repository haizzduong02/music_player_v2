#include "../../../inc/app/controller/USBController.h"
#include "../../../inc/utils/Logger.h"

USBController::USBController(IFileSystem* fileSystem)
    : fileSystem_(fileSystem) {
}

USBController::~USBController() {
}

std::vector<std::string> USBController::detectUSB() {
    return fileSystem_->detectUSBDevices();
}

bool USBController::mountUSB(const std::string& device, const std::string& mountPoint) {
    if (fileSystem_->mountUSB(device, mountPoint)) {
        Logger::getInstance().info("Mounted USB device: " + device + " at " + mountPoint);
        return true;
    }
    
    Logger::getInstance().error("Failed to mount USB device: " + device);
    return false;
}

bool USBController::unmountUSB(const std::string& mountPoint) {
    if (fileSystem_->unmountUSB(mountPoint)) {
        Logger::getInstance().info("Unmounted USB from: " + mountPoint);
        return true;
    }
    
    Logger::getInstance().error("Failed to unmount USB from: " + mountPoint);
    return false;
}

std::vector<std::string> USBController::scanUSBMedia(
    const std::string& mountPoint,
    const std::vector<std::string>& extensions) {
    
    if (!fileSystem_->exists(mountPoint) || !fileSystem_->isDirectory(mountPoint)) {
        Logger::getInstance().error("Invalid USB path: " + mountPoint);
        return {};
    }
    
    // Scan for media files with given extensions
    Logger::getInstance().info("USB scanning: " + mountPoint);
    // TODO: Full implementation needs LibraryController integration
    return {};
}
