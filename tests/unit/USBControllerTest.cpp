#include "app/controller/USBController.h"
#include "tests/mocks/MockFileSystem.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

class USBControllerTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockFileSystem> mockFs;
    std::unique_ptr<USBController> controller;

    void SetUp() override
    {
        mockFs = std::make_shared<MockFileSystem>();
        controller = std::make_unique<USBController>(mockFs.get());
    }
};

TEST_F(USBControllerTest, DetectUSBDelegatesToFileSystem)
{
    std::vector<std::string> devices = {"/dev/sdb1"};
    EXPECT_CALL(*mockFs, detectUSBDevices()).WillOnce(Return(devices));

    auto result = controller->detectUSB();
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "/dev/sdb1");
}

TEST_F(USBControllerTest, MountUSBSuccess)
{
    EXPECT_CALL(*mockFs, mountUSB("/dev/sdb1", "/mnt/usb")).WillOnce(Return(true));

    bool success = controller->mountUSB("/dev/sdb1", "/mnt/usb");
    EXPECT_TRUE(success);
}

TEST_F(USBControllerTest, ScanUSBMediaDelegatesToFileSystem)
{
    std::vector<std::string> files = {"/mnt/usb/song.mp3"};

    // Controller checks if path exists AND is directory
    EXPECT_CALL(*mockFs, exists("/mnt/usb")).WillOnce(Return(true));
    EXPECT_CALL(*mockFs, isDirectory("/mnt/usb")).WillOnce(Return(true));

    // Updated expectation to match signature: path, extensions, maxDepth
    EXPECT_CALL(*mockFs, getMediaFiles("/mnt/usb", _, _)).WillOnce(Return(files));

    auto result = controller->scanUSBMedia("/mnt/usb", {".mp3"});
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "/mnt/usb/song.mp3");
}
TEST_F(USBControllerTest, MountUSBFailure)
{
    EXPECT_CALL(*mockFs, mountUSB("/dev/sdb1", "/mnt/usb")).WillOnce(Return(false));
    bool success = controller->mountUSB("/dev/sdb1", "/mnt/usb");
    EXPECT_FALSE(success);
}

TEST_F(USBControllerTest, UnmountUSB)
{
    EXPECT_CALL(*mockFs, unmountUSB("/mnt/usb")).WillOnce(Return(true)).WillOnce(Return(false));

    EXPECT_TRUE(controller->unmountUSB("/mnt/usb"));
    EXPECT_FALSE(controller->unmountUSB("/mnt/usb"));
}

TEST_F(USBControllerTest, ScanUSBMediaInvalidPaths)
{
    // Path not exists
    EXPECT_CALL(*mockFs, exists("/bad")).WillOnce(Return(false));
    auto result1 = controller->scanUSBMedia("/bad", {".mp3"});
    EXPECT_TRUE(result1.empty());

    // Path is not a directory
    EXPECT_CALL(*mockFs, exists("/file")).WillOnce(Return(true));
    EXPECT_CALL(*mockFs, isDirectory("/file")).WillOnce(Return(false));
    auto result2 = controller->scanUSBMedia("/file", {".mp3"});
    EXPECT_TRUE(result2.empty());
}
