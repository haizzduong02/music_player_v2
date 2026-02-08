#ifndef MOCK_FILESYSTEM_H
#define MOCK_FILESYSTEM_H

#include "interfaces/IFileSystem.h"
#include <gmock/gmock.h>

class MockFileSystem : public IFileSystem
{
  public:
    MOCK_METHOD(std::vector<FileInfo>, browse, (const std::string &), (override));
    MOCK_METHOD(std::vector<std::string>, scanDirectory, (const std::string &, const std::vector<std::string> &, int),
                (override));
    MOCK_METHOD(std::vector<std::string>, getMediaFiles, (const std::string &, const std::vector<std::string> &, int),
                (override));
    MOCK_METHOD(std::vector<std::string>, detectUSBDevices, (), (override));
    MOCK_METHOD(bool, mountUSB, (const std::string &, const std::string &), (override));
    MOCK_METHOD(bool, unmountUSB, (const std::string &), (override));
    MOCK_METHOD(bool, exists, (const std::string &), (override));
    MOCK_METHOD(bool, isDirectory, (const std::string &), (override));
};

#endif // MOCK_FILESYSTEM_H
