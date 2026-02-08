#include "service/JsonPersistence.h"
#include "service/LocalFileSystem.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class FileServicesTest : public ::testing::Test
{
  protected:
    std::string tempDir;

    void SetUp() override
    {
        // Create unique temp directory using timestamp
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        auto tempPath = fs::temp_directory_path() / ("ServiceTest_" + std::to_string(now));
        fs::create_directories(tempPath);
        tempDir = tempPath.string();
    }

    void TearDown() override
    {
        fs::remove_all(tempDir);
    }
};

// --- JsonPersistence Tests ---

TEST_F(FileServicesTest, SaveAndLoadString)
{
    JsonPersistence persistence;
    std::string filePath = (fs::path(tempDir) / "data.json").string();
    std::string content = "{\"key\": \"value\"}";

    // 1. Save
    ASSERT_TRUE(persistence.saveToFile(filePath, content));

    // 2. Load
    std::string loaded;
    ASSERT_TRUE(persistence.loadFromFile(filePath, loaded));

    // 3. Verify
    EXPECT_EQ(content, loaded);
}

TEST_F(FileServicesTest, SaveCreatesMissingDirectories)
{
    JsonPersistence persistence;
    // Nest deep: temp/a/b/c/data.json
    std::string deepPath = (fs::path(tempDir) / "a" / "b" / "c" / "data.json").string();

    ASSERT_TRUE(persistence.saveToFile(deepPath, "test"));
    EXPECT_TRUE(fs::exists(deepPath));
}

// --- LocalFileSystem Tests ---

TEST_F(FileServicesTest, BrowseReturnsCorrectStructure)
{
    LocalFileSystem lfs;

    // Setup: Create 1 folder and 1 file
    fs::create_directory(fs::path(tempDir) / "SubFolder");
    std::ofstream((fs::path(tempDir) / "File.txt").string()) << "hi";

    auto files = lfs.browse(tempDir);

    // Expect 2 entries
    // Note: The order depends on implementation/filesystem, but usually directories are sorted first if implemented
    // that way. Let's just verify existence in the result
    bool foundFolder = false;
    bool foundFile = false;

    for (const auto &file : files)
    {
        if (file.name == "SubFolder" && file.isDirectory)
            foundFolder = true;
        if (file.name == "File.txt" && !file.isDirectory)
            foundFile = true;
    }

    EXPECT_TRUE(foundFolder);
    EXPECT_TRUE(foundFile);
}

TEST_F(FileServicesTest, ScanRecursiveFindsExtensions)
{
    LocalFileSystem lfs;

    // Setup:
    // root/song.mp3
    // root/nested/video.mp4
    // root/nested/ignore.txt

    fs::create_directories(fs::path(tempDir) / "nested");
    std::ofstream((fs::path(tempDir) / "song.mp3").string()) << ".";
    std::ofstream((fs::path(tempDir) / "nested" / "video.mp4").string()) << ".";
    std::ofstream((fs::path(tempDir) / "nested" / "ignore.txt").string()) << ".";

    std::vector<std::string> exts = {".mp3", ".mp4"};
    auto results = lfs.scanDirectory(tempDir, exts, -1); // -1 = infinite depth

    // Depending on implementation, it might return exact paths or relative to scan root.
    // Assuming absolute paths based on typical behavior, or checking substring.

    bool foundMp3 = false;
    bool foundMp4 = false;

    for (const auto &path : results)
    {
        if (path.find("song.mp3") != std::string::npos)
            foundMp3 = true;
        if (path.find("video.mp4") != std::string::npos)
            foundMp4 = true;
    }

    EXPECT_TRUE(foundMp3);
    EXPECT_TRUE(foundMp4);
}
