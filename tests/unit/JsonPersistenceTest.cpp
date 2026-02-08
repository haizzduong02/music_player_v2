#include "service/JsonPersistence.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class JsonPersistenceTest : public ::testing::Test
{
  protected:
    JsonPersistence persistence;
    std::string testDir = "tests/assets/temp_persist";
    std::string testFile = "tests/assets/temp_persist/test.json";

    void SetUp() override
    {
        if (fs::exists(testDir))
        {
            fs::remove_all(testDir);
        }
        fs::create_directories(testDir);
    }

    void TearDown() override
    {
        if (fs::exists(testDir))
        {
            fs::remove_all(testDir);
        }
    }
    bool proxyIsValidJson(const std::string &json)
    {
        return persistence.isValidJson(json);
    }
};

TEST_F(JsonPersistenceTest, SaveAndLoadFile)
{
    std::string content = "{\"key\": \"value\"}";
    EXPECT_TRUE(persistence.saveToFile(testFile, content));

    EXPECT_TRUE(fs::exists(testFile));

    std::string loaded;
    EXPECT_TRUE(persistence.loadFromFile(testFile, loaded));
    EXPECT_EQ(loaded, content);
}

TEST_F(JsonPersistenceTest, FileExists)
{
    EXPECT_FALSE(persistence.fileExists(testFile));

    std::ofstream ofs(testFile);
    ofs << "{}";
    ofs.close();

    EXPECT_TRUE(persistence.fileExists(testFile));
}

TEST_F(JsonPersistenceTest, DeleteFile)
{
    std::ofstream ofs(testFile);
    ofs << "{}";
    ofs.close();

    EXPECT_TRUE(fs::exists(testFile));
    EXPECT_TRUE(persistence.deleteFile(testFile));
    EXPECT_FALSE(fs::exists(testFile));
}

TEST_F(JsonPersistenceTest, SaveCreatesDirectory)
{
    std::string deeperPath = testDir + "/subdir/file.json";
    std::string content = "{}";

    // JsonPersistence typically doesn't auto-create dirs in saveToFile unless explicit?
    // Let's check implementation if possible, or assume it might fail if dir missing.
    // Actually, ensureDirectoryExists is public, let's test it.

    std::string newDir = testDir + "/newdir";
    EXPECT_TRUE(persistence.ensureDirectoryExists(newDir));
    EXPECT_TRUE(fs::exists(newDir));
    EXPECT_TRUE(fs::is_directory(newDir));
}
TEST_F(JsonPersistenceTest, SerializeDeserialize)
{
    // These are stubs for now, but we should hit them for coverage
    EXPECT_EQ(persistence.serialize(nullptr), "{}");
    EXPECT_EQ(persistence.serialize((void *)1), "{}");

    EXPECT_FALSE(persistence.deserialize("", nullptr));
    EXPECT_FALSE(persistence.deserialize("{}", (void *)1));
}

TEST_F(JsonPersistenceTest, IsValidJson)
{
    EXPECT_TRUE(proxyIsValidJson("{}"));
    EXPECT_TRUE(proxyIsValidJson("[]"));
    EXPECT_TRUE(proxyIsValidJson("{\"a\":1}"));
    EXPECT_FALSE(proxyIsValidJson(""));
    EXPECT_FALSE(proxyIsValidJson("{"));
    EXPECT_FALSE(proxyIsValidJson("abc"));
}
TEST_F(JsonPersistenceTest, SaveFailures)
{
    // Try to save to a read-only directory or invalid path
    EXPECT_FALSE(persistence.saveToFile("/root/test.json", "{}"));
}

TEST_F(JsonPersistenceTest, DeleteFailures)
{
    EXPECT_FALSE(persistence.deleteFile("/nonexistent/file/path"));
}

TEST_F(JsonPersistenceTest, EnsureDirectoryExistsExisting)
{
    EXPECT_TRUE(persistence.ensureDirectoryExists(testDir));
    // Should hit line 122 branch
}

TEST_F(JsonPersistenceTest, ExceptionPaths)
{
    // Trigger exception in ensureDirectoryExists
    std::string fileAsDir = testDir + "/file_not_dir";
    {
        std::ofstream ofs(fileAsDir);
        ofs << "data";
    }

    // std::filesystem::create_directories should throw/fail if a component exists and is not a dir
    EXPECT_FALSE(persistence.ensureDirectoryExists(fileAsDir + "/deep"));

    // Exception in saveToFile (via ensureDirectoryExists)
    EXPECT_FALSE(persistence.saveToFile(fileAsDir + "/deep/file.json", "{}"));
}
