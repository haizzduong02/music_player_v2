#include "service/LocalFileSystem.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

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

TEST_F(LocalFileSystemTest, PermissionDenied)
{
    std::string protectedDir = testDir + "/protected";
    fs::create_directories(protectedDir);
    std::ofstream(protectedDir + "/secret.mp3").close();

    // Remove permissions
    fs::permissions(protectedDir, fs::perms::none);

    // browse should catch exception
    auto results = fsClient.browse(protectedDir);
    EXPECT_TRUE(results.empty());

    // scan should catch exception
    auto results2 = fsClient.scanDirectory(protectedDir, {".mp3"});
    EXPECT_TRUE(results2.empty());

    // Restore for cleanup (or TearDown will fail)
    fs::permissions(protectedDir, fs::perms::owner_all);
}

TEST_F(LocalFileSystemTest, ScanDepthLimits)
{
    // Create deep structure: testDir/level1/level2/level3/file.mp3
    std::string d1 = testDir + "/level1";
    std::string d2 = d1 + "/level2";
    std::string d3 = d2 + "/level3";
    fs::create_directories(d3);
    std::ofstream(d3 + "/file.mp3").close();

    // 1. Depth 0 (non-recursive)
    EXPECT_EQ(fsClient.scanDirectory(testDir, {".mp3"}, 0).size(), 1); // Only root file1.mp3

    // 2. Depth 1 (root + level1 + subdir)
    // subdir/file3.mp3 is at depth 1
    EXPECT_EQ(fsClient.scanDirectory(testDir, {".mp3"}, 1).size(), 2); 

    // 3. Depth 2
    // level2 is at depth 2, but level3/file.mp3 is at depth 3
    EXPECT_EQ(fsClient.scanDirectory(testDir, {".mp3"}, 2).size(), 2);

    // 4. Depth 3
    // level3/file.mp3 is at depth 3
    EXPECT_EQ(fsClient.scanDirectory(testDir, {".mp3"}, 3).size(), 3); 
}
