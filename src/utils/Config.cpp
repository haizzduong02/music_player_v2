#include "utils/Config.h"
#include "utils/Logger.h"
#include <json.hpp>
bool Config::load()
{
    if (!persistence_)
    {
        Logger::error("Config persistence not initialized");
        return false;
    }

    if (!persistence_->fileExists(config_.configPath))
    {
        Logger::info("No config file found at " + config_.configPath + ", using defaults");
        return save(); // Save defaults
    }

    std::string data;
    if (!persistence_->loadFromFile(config_.configPath, data))
    {
        Logger::error("Failed to load config file");
        return false;
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(data);
        config_ = j.get<AppConfig>();
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to parse config: " + std::string(e.what()));
        return false;
    }

    Logger::info("Configuration loaded successfully");
    return true;
}

bool Config::save()
{
    if (!persistence_)
    {
        Logger::error("Config persistence not initialized");
        return false;
    }

    try
    {
        nlohmann::json j = config_;
        std::string data = j.dump(4);

        if (!persistence_->saveToFile(config_.configPath, data))
        {
            Logger::error("Failed to save config file");
            return false;
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to serialize config: " + std::string(e.what()));
        return false;
    }

    // ... existing methods ...

    Logger::info("Configuration saved successfully");
    return true;
}

void to_json(nlohmann::json &j, const AppConfig &c)
{
    j = nlohmann::json{{"defaultVolume", c.defaultVolume},
                       {"loopEnabled", c.loopEnabled},
                       {"shuffleEnabled", c.shuffleEnabled},
                       {"theme", c.theme},
                       {"windowWidth", c.windowWidth},
                       {"windowHeight", c.windowHeight},
                       {"libraryPath", c.libraryPath},
                       {"playlistDir", c.playlistDir},
                       {"historyPath", c.historyPath},
                       {"configPath", c.configPath},
                       {"logPath", c.logPath},
                       {"serialPort", c.serialPort},
                       {"baudRate", c.baudRate},
                       {"hardwareEnabled", c.hardwareEnabled},
                       {"maxHistorySize", c.maxHistorySize},
                       {"supportedAudioFormats", c.supportedAudioFormats},
                       {"supportedVideoFormats", c.supportedVideoFormats},
                       {"customSettings", c.customSettings}};
}

void from_json(const nlohmann::json &j, AppConfig &c)
{
    if (j.contains("defaultVolume"))
        c.defaultVolume = j.at("defaultVolume").get<float>();
    if (j.contains("loopEnabled"))
        c.loopEnabled = j.at("loopEnabled").get<bool>();
    if (j.contains("shuffleEnabled"))
        c.shuffleEnabled = j.at("shuffleEnabled").get<bool>();
    if (j.contains("theme"))
        c.theme = j.at("theme").get<std::string>();
    if (j.contains("windowWidth"))
        c.windowWidth = j.at("windowWidth").get<int>();
    if (j.contains("windowHeight"))
        c.windowHeight = j.at("windowHeight").get<int>();
    if (j.contains("libraryPath"))
        c.libraryPath = j.at("libraryPath").get<std::string>();
    if (j.contains("playlistDir"))
        c.playlistDir = j.at("playlistDir").get<std::string>();
    if (j.contains("historyPath"))
        c.historyPath = j.at("historyPath").get<std::string>();
    if (j.contains("configPath"))
        c.configPath = j.at("configPath").get<std::string>();
    if (j.contains("logPath"))
        c.logPath = j.at("logPath").get<std::string>();
    if (j.contains("serialPort"))
        c.serialPort = j.at("serialPort").get<std::string>();
    if (j.contains("baudRate"))
        c.baudRate = j.at("baudRate").get<int>();
    if (j.contains("hardwareEnabled"))
        c.hardwareEnabled = j.at("hardwareEnabled").get<bool>();
    if (j.contains("maxHistorySize"))
        c.maxHistorySize = j.at("maxHistorySize").get<int>();
    if (j.contains("supportedAudioFormats"))
        c.supportedAudioFormats = j.at("supportedAudioFormats").get<std::vector<std::string>>();
    if (j.contains("supportedVideoFormats"))
        c.supportedVideoFormats = j.at("supportedVideoFormats").get<std::vector<std::string>>();
    if (j.contains("customSettings"))
        c.customSettings = j.at("customSettings").get<std::map<std::string, std::string>>();
}
