#include "service/LocalFileSystem.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

class LocalFileSystemTest : public ::testing::Test
{
  protected:
    LocalFileSystem fsClient;
    std::string testDir = "tests/assets/temp_fs";

    void SetUp() override
    {
        std::filesystem::create_directories(testDir);
        std::ofstream(testDir + "/file1.mp3").close();
        std::ofstream(testDir + "/file2.txt").close();
        std::filesystem::create_directories(testDir + "/subdir");
        std::ofstream(testDir + "/subdir/file3.mp3").close();
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testDir);
    }

    bool contains(const std::vector<FileInfo> &items, const std::string &name)
    {
        for (const auto &item : items)
        {
            if (item.name == name)
                return true;
        }
        return false;
    }

    bool containsStr(const std::vector<std::string> &items, const std::string &fragment)
    {
        for (const auto &item : items)
        {
            if (item.find(fragment) != std::string::npos)
                return true;
        }
        return false;
    }
};

TEST_F(LocalFileSystemTest, Exists)
{
    EXPECT_TRUE(fsClient.exists(testDir + "/file1.mp3"));
    EXPECT_FALSE(fsClient.exists(testDir + "/nonexistent.mp3"));
}

TEST_F(LocalFileSystemTest, BrowseDirectory)
{
    auto results = fsClient.browse(testDir);
    EXPECT_GE(results.size(), 3); // file1.mp3, file2.txt, subdir
    EXPECT_TRUE(contains(results, "file1.mp3"));
    EXPECT_TRUE(contains(results, "subdir"));
}

TEST_F(LocalFileSystemTest, ScanDirectoryDeep)
{
    std::vector<std::string> exts = {".mp3"};
    // depth -1 means recursive
    auto results = fsClient.getMediaFiles(testDir, exts, -1);

    EXPECT_EQ(results.size(), 2);
    EXPECT_TRUE(containsStr(results, "file1.mp3"));
    EXPECT_TRUE(containsStr(results, "file3.mp3"));
}

TEST_F(LocalFileSystemTest, ScanDirectoryShallow)
{
    std::vector<std::string> exts = {".mp3"};
    // depth 0 means non-recursive
    auto results = fsClient.getMediaFiles(testDir, exts, 0);

    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(containsStr(results, "file1.mp3"));
}

TEST_F(LocalFileSystemTest, FilterByExtension)
{
    std::vector<std::string> exts = {".txt"};
    auto results = fsClient.getMediaFiles(testDir, exts, -1);

    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(containsStr(results, "file2.txt"));
}

TEST_F(LocalFileSystemTest, BrowseInvalidPath)
{
    auto results = fsClient.browse(testDir + "/nonexistent");
    EXPECT_TRUE(results.empty());

    auto results2 = fsClient.browse(testDir + "/file1.mp3"); // Not a directory
    EXPECT_TRUE(results2.empty());
}

TEST_F(LocalFileSystemTest, ExtensionEdgeCases)
{
    // No extension
    std::string noExt = testDir + "/noextension";
    std::ofstream(noExt).close();

    auto results = fsClient.scanDirectory(testDir, {".mp3"});
    // Should NOT find it
    for (const auto &item : results)
    {
        EXPECT_FALSE(item.find("noextension") != std::string::npos);
    }
}

TEST_F(LocalFileSystemTest, FileExistsOnDirectory)
{
    EXPECT_TRUE(fsClient.exists(testDir));
}

TEST_F(LocalFileSystemTest, UnimplementedUSB)
{
    EXPECT_TRUE(fsClient.detectUSBDevices().empty());
    EXPECT_FALSE(fsClient.mountUSB("dev", "mnt"));
    EXPECT_FALSE(fsClient.unmountUSB("mnt"));
}

TEST_F(LocalFileSystemTest, ErrorPaths)
{
    // Trigger isDirectory exception (invalid path)
    EXPECT_FALSE(fsClient.isDirectory("\0"));

    // browse exception
    auto results = fsClient.browse("\0");
    EXPECT_TRUE(results.empty());

    // scan exception
    EXPECT_TRUE(fsClient.scanDirectory("\0", {}).empty());
}
