#ifndef LOCAL_FILE_SYSTEM_H
#define LOCAL_FILE_SYSTEM_H

#include "../interfaces/IFileSystem.h"
#include <string>
#include <vector>

/**
 * @file LocalFileSystem.h
 * @brief Concrete implementation of IFileSystem for local file access
 * 
 * Provides file system operations using standard C++ filesystem library.
 */

/**
 * @brief Local file system class
 * 
 * Concrete implementation of IFileSystem.
 * Uses std::filesystem for file operations.
 */
class LocalFileSystem : public IFileSystem {
public:
    /**
     * @brief Constructor
     */
    LocalFileSystem() = default;
    
    /**
     * @brief Destructor
     */
    ~LocalFileSystem() override = default;
    
    // IFileSystem implementation
    std::vector<FileInfo> browse(const std::string& path) override;
    
    std::vector<std::string> scanDirectory(
        const std::string& path,
        const std::vector<std::string>& extensions,
        int maxDepth = -1) override;
    
    std::vector<std::string> getMediaFiles(
        const std::string& path,
        const std::vector<std::string>& extensions,
        int maxDepth = -1) override;
    
    std::vector<std::string> detectUSBDevices() override;
    
    bool mountUSB(const std::string& device, const std::string& mountPoint) override;
    
    bool unmountUSB(const std::string& mountPoint) override;
    
    bool exists(const std::string& path) override;
    
    bool isDirectory(const std::string& path) override;
    
private:
    /**
     * @brief Recursive helper for scanDirectory
     * @param path Path to scan
     * @param extensions Extensions to filter
     * @param results Output vector
     * @param maxDepth Max recursion depth
     * @param currentDepth Current recursion depth
     */
    void scanDirectoryRecursive(
        const std::string& path,
        const std::vector<std::string>& extensions,
        std::vector<std::string>& results,
        int maxDepth,
        int currentDepth);
    
    /**
     * @brief Check if file has supported extension
     * @param filepath File to check
     * @param extensions Supported extensions
     * @return true if supported
     */
    bool hasExtension(const std::string& filepath, const std::vector<std::string>& extensions);
};

#endif // LOCAL_FILE_SYSTEM_H
