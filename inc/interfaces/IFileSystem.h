#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include <string>
#include <vector>

/**
 * @file IFileSystem.h
 * @brief Interface for file system operations (Dependency Inversion Principle)
 *
 * Abstracts file system access to enable testing and different implementations.
 */

/**
 * @brief File information structure
 */
struct FileInfo
{
    std::string path;
    std::string name;
    std::string extension;
    bool isDirectory;
    size_t size;
};

/**
 * @brief File system interface
 *
 * Provides methods for browsing directories, scanning for media files,
 * and managing USB devices.
 */
class IFileSystem
{
  public:
    virtual ~IFileSystem() = default;

    /**
     * @brief List all files and directories in a path
     * @param path Directory path to browse
     * @return Vector of FileInfo structures
     */
    virtual std::vector<FileInfo> browse(const std::string &path) = 0;

    /**
     * @brief Recursively scan directory for media files
     * @param path Directory path to scan
     * @param extensions Supported file extensions (e.g., {".mp3", ".mp4"})
     * @param maxDepth Maximum recursion depth (-1 for infinite)
     * @return Vector of media file paths
     */
    virtual std::vector<std::string> scanDirectory(const std::string &path, const std::vector<std::string> &extensions,
                                                   int maxDepth = -1) = 0;

    /**
     * @brief Get all media files in a directory
     * @param path Directory path
     * @param extensions Supported file extensions
     * @param maxDepth Maximum recursion depth (0 for current dir only, -1 for infinite)
     * @return Vector of media file paths
     */
    virtual std::vector<std::string> getMediaFiles(const std::string &path, const std::vector<std::string> &extensions,
                                                   int maxDepth = -1) = 0;

    /**
     * @brief Detect connected USB devices
     * @return Vector of USB device mount points
     */
    virtual std::vector<std::string> detectUSBDevices() = 0;

    /**
     * @brief Mount a USB device
     * @param device Device path (e.g., /dev/sdb1)
     * @param mountPoint Where to mount the device
     * @return true if mounted successfully
     */
    virtual bool mountUSB(const std::string &device, const std::string &mountPoint) = 0;

    /**
     * @brief Unmount a USB device
     * @param mountPoint Mount point to unmount
     * @return true if unmounted successfully
     */
    virtual bool unmountUSB(const std::string &mountPoint) = 0;

    /**
     * @brief Check if a path exists
     * @param path Path to check
     * @return true if path exists
     */
    virtual bool exists(const std::string &path) = 0;

    /**
     * @brief Check if a path is a directory
     * @param path Path to check
     * @return true if path is a directory
     */
    virtual bool isDirectory(const std::string &path) = 0;
};

#endif // IFILESYSTEM_H
