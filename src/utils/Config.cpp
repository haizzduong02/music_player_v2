#include "../../inc/utils/Config.h"
#include "../../inc/utils/Logger.h"

bool Config::load() {
    if (!persistence_) {
        Logger::getInstance().error("Config persistence not initialized");
        return false;
    }

    if (!persistence_->fileExists(config_.configPath)) {
        Logger::getInstance().info("No config file found at " + config_.configPath + ", using defaults");
        return save(); // Save defaults
    }

    std::string data;
    if (!persistence_->loadFromFile(config_.configPath, data)) {
        Logger::getInstance().error("Failed to load config file");
        return false;
    }

    if (!persistence_->deserialize(data, &config_)) {
        Logger::getInstance().error("Failed to deserialize config");
        return false;
    }

    Logger::getInstance().info("Configuration loaded successfully");
    return true;
}

bool Config::save() {
    if (!persistence_) {
        Logger::getInstance().error("Config persistence not initialized");
        return false;
    }

    std::string data = persistence_->serialize(&config_);
    if (data.empty()) {
        Logger::getInstance().error("Failed to serialize config");
        return false;
    }

    // Ensure directory exists
    // The persistence layer's saveToFile should handle directory creation or we rely on it here?
    // Looking at JsonPersistence.cpp, it calls ensureDirectoryExists.
    
    if (!persistence_->saveToFile(config_.configPath, data)) {
        Logger::getInstance().error("Failed to save config file");
        return false;
    }

    Logger::getInstance().info("Configuration saved successfully");
    return true;
}
