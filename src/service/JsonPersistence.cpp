#include "../../inc/service/JsonPersistence.h"
#include "../../inc/utils/Logger.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool JsonPersistence::saveToFile(const std::string& filepath, const std::string& data) {
    try {
        // Ensure parent directory exists
        fs::path path(filepath);
        if (path.has_parent_path()) {
            ensureDirectoryExists(path.parent_path().string());
        }
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            Logger::getInstance().error("Failed to open file for writing: " + filepath);
            return false;
        }
        
        file << data;
        file.close();
        
        Logger::getInstance().info("Saved data to: " + filepath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to save to file '" + filepath + "': " + e.what());
        return false;
    }
}

bool JsonPersistence::loadFromFile(const std::string& filepath, std::string& data) {
    try {
        if (!fs::exists(filepath)) {
            Logger::getInstance().warn("File does not exist: " + filepath);
            return false;
        }
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::getInstance().error("Failed to open file for reading: " + filepath);
            return false;
        }
        
        // Read entire file into string
        data.assign(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
        
        file.close();
        
        Logger::getInstance().info("Loaded data from: " + filepath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to load from file '" + filepath + "': " + e.what());
        return false;
    }
}

bool JsonPersistence::fileExists(const std::string& filepath) {
    return fs::exists(filepath);
}

bool JsonPersistence::deleteFile(const std::string& filepath) {
    try {
        if (!fs::exists(filepath)) {
            return false;
        }
        
        fs::remove(filepath);
        Logger::getInstance().info("Deleted file: " + filepath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to delete file '" + filepath + "': " + e.what());
        return false;
    }
}

std::string JsonPersistence::serialize(const void* data) {
    // TODO: Implement with nlohmann/json or similar JSON library
    // For now, return empty string
    Logger::getInstance().warn("JsonPersistence::serialize() not fully implemented");
    return "{}";
}

bool JsonPersistence::deserialize(const std::string& serialized, void* data) {
    // TODO: Implement with nlohmann/json or similar JSON library
    // For now, return false
    Logger::getInstance().warn("JsonPersistence::deserialize() not fully implemented");
    return false;
}

bool JsonPersistence::ensureDirectoryExists(const std::string& dirPath) {
    try {
        if (fs::exists(dirPath)) {
            return fs::is_directory(dirPath);
        }
        
        return fs::create_directories(dirPath);
        
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to create directory '" + dirPath + "': " + e.what());
        return false;
    }
}

bool JsonPersistence::isValidJson(const std::string& jsonStr) {
    // Basic JSON validation - just check for braces/brackets
    if (jsonStr.empty()) {
        return false;
    }
    
    char first = jsonStr.front();
    char last = jsonStr.back();
    
    return (first == '{' && last == '}') || (first == '[' && last == ']');
}
