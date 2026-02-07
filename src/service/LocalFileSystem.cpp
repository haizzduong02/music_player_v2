#include "service/LocalFileSystem.h"
#include "utils/Logger.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<FileInfo> LocalFileSystem::browse(const std::string &path)
{
    std::vector<FileInfo> files;

    try
    {
        if (!fs::exists(path) || !fs::is_directory(path))
        {
            Logger::warn("Invalid directory: " + path);
            return files;
        }

        for (const auto &entry : fs::directory_iterator(path))
        {
            FileInfo info;
            info.path = entry.path().string();
            info.name = entry.path().filename().string();
            info.extension = entry.path().extension().string();
            info.isDirectory = entry.is_directory();
            info.size = info.isDirectory ? 0 : entry.file_size();

            files.push_back(info);
        }

        // Sort: directories first, then files, alphabetically
        std::sort(files.begin(), files.end(),
                  [](const FileInfo &a, const FileInfo &b)
                  {
                      if (a.isDirectory != b.isDirectory)
                      {
                          return a.isDirectory > b.isDirectory;
                      }
                      return a.name < b.name;
                  });
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error("Failed to browse directory '" + path + "': " + e.what());
    }

    return files;
}

std::vector<std::string> LocalFileSystem::scanDirectory(const std::string &path,
                                                        const std::vector<std::string> &extensions, int maxDepth)
{

    std::vector<std::string> results;
    scanDirectoryRecursive(path, extensions, results, maxDepth, 0);
    return results;
}

std::vector<std::string> LocalFileSystem::getMediaFiles(const std::string &path,
                                                        const std::vector<std::string> &extensions, int maxDepth)
{

    return scanDirectory(path, extensions, maxDepth);
}

std::vector<std::string> LocalFileSystem::detectUSBDevices()
{
    Logger::warn("USB detection not implemented for this platform");
    return std::vector<std::string>();
}

bool LocalFileSystem::mountUSB(const std::string &device, const std::string &mountPoint)
{
    Logger::warn("USB mounting not implemented for this platform");
    return false;
}

bool LocalFileSystem::unmountUSB(const std::string &mountPoint)
{
    Logger::warn("USB unmounting not implemented for this platform");
    return false;
}

bool LocalFileSystem::exists(const std::string &path)
{
    return fs::exists(path);
}

bool LocalFileSystem::isDirectory(const std::string &path)
{
    try
    {
        return fs::is_directory(path);
    }
    catch (const fs::filesystem_error &)
    {
        return false;
    }
}

void LocalFileSystem::scanDirectoryRecursive(const std::string &path, const std::vector<std::string> &extensions,
                                             std::vector<std::string> &results, int maxDepth, int currentDepth)
{

    try
    {
        if (!fs::exists(path) || !fs::is_directory(path))
        {
            return;
        }

        // Check depth limit if maxDepth is set (>= 0)
        if (maxDepth >= 0 && currentDepth > maxDepth)
        {
            return;
        }

        for (const auto &entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                // Determine if we should recurse
                if (maxDepth < 0 || currentDepth < maxDepth)
                {
                    scanDirectoryRecursive(entry.path().string(), extensions, results, maxDepth, currentDepth + 1);
                }
            }
            else if (entry.is_regular_file())
            {
                if (hasExtension(entry.path().string(), extensions))
                {
                    results.push_back(entry.path().string());
                }
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        Logger::error("Error scanning directory '" + path + "': " + e.what());
    }
}

bool LocalFileSystem::hasExtension(const std::string &filepath, const std::vector<std::string> &extensions)
{

    fs::path p(filepath);
    std::string ext = p.extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto &validExt : extensions)
    {
        std::string lowerValidExt = validExt;
        std::transform(lowerValidExt.begin(), lowerValidExt.end(), lowerValidExt.begin(), ::tolower);

        if (ext == lowerValidExt)
        {
            return true;
        }
    }

    return false;
}
