#include "utils/Config.h"
#include "tests/mocks/MockPersistence.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;

class ConfigUnitTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;

    void SetUp() override
    {
        mockPersist = std::make_shared<MockPersistence>();
        // Reset config to defaults before each test
        Config::getInstance().setAppConfig(AppConfig{});
    }

    void TearDown() override
    {
        Config::getInstance().init(nullptr);
    }
};

// Singleton tests
TEST_F(ConfigUnitTest, GetInstanceReturnsSameInstance)
{
    Config& instance1 = Config::getInstance();
    Config& instance2 = Config::getInstance();
    EXPECT_EQ(&instance1, &instance2);
}

// Init tests
TEST_F(ConfigUnitTest, InitWithValidPersistence)
{
    Config::getInstance().init(mockPersist.get());
    // No crash means success
    SUCCEED();
}

TEST_F(ConfigUnitTest, InitWithNullPersistence)
{
    Config::getInstance().init(nullptr);
    SUCCEED();
}

// Load tests
TEST_F(ConfigUnitTest, LoadSuccess)
{
    std::string configJson = R"({
        "defaultVolume": 0.75,
        "loopEnabled": true,
        "theme": "Light"
    })";

    EXPECT_CALL(*mockPersist, fileExists(_))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockPersist, loadFromFile(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(configJson), Return(true)));

    Config::getInstance().init(mockPersist.get());
    EXPECT_TRUE(Config::getInstance().load());
}

TEST_F(ConfigUnitTest, LoadFailure)
{
    EXPECT_CALL(*mockPersist, fileExists(_))
        .WillOnce(Return(true));

    EXPECT_CALL(*mockPersist, loadFromFile(_, _))
        .WillOnce(Return(false));

    Config::getInstance().init(mockPersist.get());
    EXPECT_FALSE(Config::getInstance().load());
}

TEST_F(ConfigUnitTest, LoadWithoutPersistence)
{
    Config::getInstance().init(nullptr);
    EXPECT_FALSE(Config::getInstance().load());
}

// Save tests
TEST_F(ConfigUnitTest, SaveSuccess)
{
    EXPECT_CALL(*mockPersist, saveToFile(_, _))
        .WillOnce(Return(true));

    Config::getInstance().init(mockPersist.get());
    EXPECT_TRUE(Config::getInstance().save());
}

TEST_F(ConfigUnitTest, SaveFailure)
{
    EXPECT_CALL(*mockPersist, saveToFile(_, _))
        .WillOnce(Return(false));

    Config::getInstance().init(mockPersist.get());
    EXPECT_FALSE(Config::getInstance().save());
}

TEST_F(ConfigUnitTest, SaveWithoutPersistence)
{
    Config::getInstance().init(nullptr);
    EXPECT_FALSE(Config::getInstance().save());
}

// Custom settings tests
TEST_F(ConfigUnitTest, SetAndGetCustomSetting)
{
    Config::getInstance().setCustomSetting("myKey", "myValue");
    EXPECT_EQ(Config::getInstance().getCustomSetting("myKey"), "myValue");
}

TEST_F(ConfigUnitTest, GetCustomSettingNonExistent)
{
    EXPECT_EQ(Config::getInstance().getCustomSetting("nonexistent", "default"), "default");
}

TEST_F(ConfigUnitTest, GetCustomSettingNoDefault)
{
    EXPECT_EQ(Config::getInstance().getCustomSetting("nonexistent"), "");
}

TEST_F(ConfigUnitTest, OverwriteCustomSetting)
{
    Config::getInstance().setCustomSetting("key", "value1");
    Config::getInstance().setCustomSetting("key", "value2");
    EXPECT_EQ(Config::getInstance().getCustomSetting("key"), "value2");
}

// AppConfig getter/setter tests
TEST_F(ConfigUnitTest, GetConfig)
{
    const AppConfig& config = Config::getInstance().getConfig();
    EXPECT_EQ(config.defaultVolume, 0.5f); // Default value
}

TEST_F(ConfigUnitTest, SetAppConfig)
{
    AppConfig newConfig;
    newConfig.defaultVolume = 0.9f;
    newConfig.theme = "Custom";

    Config::getInstance().setAppConfig(newConfig);

    EXPECT_EQ(Config::getInstance().getConfig().defaultVolume, 0.9f);
    EXPECT_EQ(Config::getInstance().getConfig().theme, "Custom");
}

TEST_F(ConfigUnitTest, GetMutable)
{
    AppConfig& mutable_config = Config::getInstance().getMutable();
    mutable_config.windowWidth = 1920;
    mutable_config.windowHeight = 1080;

    EXPECT_EQ(Config::getInstance().getConfig().windowWidth, 1920);
    EXPECT_EQ(Config::getInstance().getConfig().windowHeight, 1080);
}

// AppConfig JSON serialization tests
TEST_F(ConfigUnitTest, AppConfigToJson)
{
    AppConfig config;
    config.defaultVolume = 0.75f;
    config.theme = "TestTheme";

    nlohmann::json j;
    to_json(j, config);

    EXPECT_EQ(j["defaultVolume"], 0.75f);
    EXPECT_EQ(j["theme"], "TestTheme");
}

TEST_F(ConfigUnitTest, AppConfigFromJson)
{
    nlohmann::json j;
    j["defaultVolume"] = 0.8f;
    j["loopEnabled"] = true;
    j["shuffleEnabled"] = true;
    j["theme"] = "FromJson";
    j["windowWidth"] = 1600;
    j["windowHeight"] = 900;

    AppConfig config;
    from_json(j, config);

    EXPECT_EQ(config.defaultVolume, 0.8f);
    EXPECT_TRUE(config.loopEnabled);
    EXPECT_TRUE(config.shuffleEnabled);
    EXPECT_EQ(config.theme, "FromJson");
    EXPECT_EQ(config.windowWidth, 1600);
    EXPECT_EQ(config.windowHeight, 900);
}

TEST_F(ConfigUnitTest, AppConfigFromJsonPartial)
{
    nlohmann::json j;
    j["defaultVolume"] = 0.6f;
    // Only set one field, others should stay default

    AppConfig config;
    from_json(j, config);

    EXPECT_EQ(config.defaultVolume, 0.6f);
    // Defaults preserved for unset fields
}

// Default values test
TEST_F(ConfigUnitTest, DefaultConfigValues)
{
    AppConfig config;
    EXPECT_EQ(config.defaultVolume, 0.5f);
    EXPECT_FALSE(config.loopEnabled);
    EXPECT_FALSE(config.shuffleEnabled);
    EXPECT_EQ(config.theme, "Dark");
    EXPECT_EQ(config.windowWidth, 1280);
    EXPECT_EQ(config.windowHeight, 720);
}
