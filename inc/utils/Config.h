#ifndef CONFIG_H
#define CONFIG_H

#include "../interfaces/IPersistence.h"
#include <string>
#include <map>
#include <vector>

/**
 * @file Config.h
 * @brief Application configuration management
 * 
 * Manages application settings and persists them to disk.
 */

/**
 * @brief Configuration structure
 */
struct AppConfig {
    // Audio settings
    float defaultVolume = 0.5f;
    bool loopEnabled = false;
    bool shuffleEnabled = false;
    
    // UI settings
    std::string theme = "Dark";
    int windowWidth = 1280;
    int windowHeight = 720;
    
    // File paths
    std::string libraryPath = "./data/library.json";
    std::string playlistDir = "./data/playlists/";
    std::string historyPath = "./data/history.json";
    std::string configPath = "./data/config.json";
    std::string logPath = "./logs/app.log";
    
    // Hardware settings
    std::string serialPort = "/dev/ttyUSB0";
    int baudRate = 115200;
    bool hardwareEnabled = true;
    
    // Playback settings
    int maxHistorySize = 50;
    
    // Supported formats
    std::vector<std::string> supportedAudioFormats = {".mp3", ".wav", ".flac", ".ogg", ".m4a"};
    std::vector<std::string> supportedVideoFormats = {".mp4", ".avi", ".mkv", ".mov"};
    
    // Additional settings
    std::map<std::string, std::string> customSettings;
};

/**
 * @brief Configuration manager class
 * 
 * Singleton for managing application configuration.
 * Depends on IPersistence for loading/saving.
 */
class Config {
public:
    /**
     * @brief Get the config instance
     * @return Reference to the singleton
     */
    static Config& getInstance() {
        static Config instance;
        return instance;
    }
    
    // Delete copy constructor and assignment
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    /**
     * @brief Initialize with persistence layer
     * @param persistence Persistence implementation for saving/loading
     */
    void init(IPersistence* persistence) {
        persistence_ = persistence;
    }
    
    /**
     * @brief Load configuration from disk
     * @return true if loaded successfully
     */
    bool load();
    
    /**
     * @brief Save configuration to disk
     * @return true if saved successfully
     */
    bool save();
    
    /**
     * @brief Get the application configuration
     * @return Reference to config structure
     */
    AppConfig& getConfig() {
        return config_;
    }
    
    /**
     * @brief Get const reference to config
     * @return Const reference to config structure
     */
    const AppConfig& getConfig() const {
        return config_;
    }
    
    /**
     * @brief Set a custom setting
     * @param key Setting key
     * @param value Setting value
     */
    void setCustomSetting(const std::string& key, const std::string& value) {
        config_.customSettings[key] = value;
    }
    
    /**
     * @brief Get a custom setting
     * @param key Setting key
     * @param defaultValue Default value if key not found
     * @return Setting value
     */
    std::string getCustomSetting(const std::string& key, const std::string& defaultValue = "") const {
        auto it = config_.customSettings.find(key);
        return (it != config_.customSettings.end()) ? it->second : defaultValue;
    }
    
private:
    Config() : persistence_(nullptr) {}
    ~Config() = default;
    
    AppConfig config_;
    IPersistence* persistence_;
};

#endif // CONFIG_H
