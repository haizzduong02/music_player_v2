#include "utils/Config.h"
#include "service/JsonPersistence.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class ConfigIntegrationTest : public ::testing::Test
{
  protected:
    std::string tempDir;
    std::string configPath;
    JsonPersistence persistence; // Use stack or pointer, shared_ptr is fine too but Config takes raw pointer

    void SetUp() override
    {
        // 1. Create a unique temp folder
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        auto tempPath = fs::temp_directory_path() / ("ConfigTest_" + std::to_string(now));
        fs::create_directories(tempPath);
        tempDir = tempPath.string();

        configPath = (tempPath / "config.json").string();

        // 2. Setup Dependencies
        // We use the singleton, but we need to reset it ideally.
        // Since it's a singleton, we must be careful.
        // We will reinject our persistence layer and config path every time.

        Config &config = Config::getInstance();
        config.init(&persistence);

        // Point config to our temp file by modifying the internal AppConfig
        AppConfig defaults;
        defaults.configPath = configPath;
        // Also set log path to temp to avoid writing to real logs during test
        defaults.logPath = (tempPath / "test_app.log").string();

        config.setAppConfig(defaults);
    }

    void TearDown() override
    {
        fs::remove_all(tempDir);
    }
};

TEST_F(ConfigIntegrationTest, LoadDefaultsWhenFileMissing)
{
    Config &config = Config::getInstance();

    // Ensure file doesn't exist yet
    ASSERT_FALSE(fs::exists(configPath));

    // Load should succeed (and save defaults)
    EXPECT_TRUE(config.load());

    // Verify file was created
    EXPECT_TRUE(fs::exists(configPath));

    // Verify a default value
    EXPECT_EQ(config.getConfig().defaultVolume, 0.5f);
}

TEST_F(ConfigIntegrationTest, SaveAndReloadValues)
{
    Config &config = Config::getInstance();

    // 1. Load initial (creates default file)
    config.load();

    // 2. Change a value
    config.getMutable().windowWidth = 1920;
    config.getMutable().theme = "Light";

    // 3. Save
    EXPECT_TRUE(config.save());

    // 4. Force a reload from disk
    // To simulate a "fresh" load, we could clear the config or just load again.
    // Since load() reads from disk, this works.
    // For extra safety, verify the file content on disk changed.
    std::string fileContent;
    persistence.loadFromFile(configPath, fileContent);
    EXPECT_TRUE(fileContent.find("Light") != std::string::npos);

    // Reload
    EXPECT_TRUE(config.load());

    // 5. Verify
    EXPECT_EQ(config.getConfig().windowWidth, 1920);
    EXPECT_EQ(config.getConfig().theme, "Light");
}

TEST_F(ConfigIntegrationTest, HandleCorruptedFile)
{
    Config &config = Config::getInstance();

    // 1. Write garbage to the file
    std::ofstream ofs(configPath);
    ofs << "{ INVALID JSON ";
    ofs.close();

    // 2. Load should fail and probably return false
    // (Based on Config.cpp implementation: catch exception -> log error -> return false)
    EXPECT_FALSE(config.load());
}
