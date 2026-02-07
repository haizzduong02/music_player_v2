#include "service/JsonPersistence.h"
#include "utils/Logger.h"
#include "utils/Config.h" // For AppConfig definition
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
// using json = nlohmann::json; // Removed for stubbing

bool JsonPersistence::saveToFile(const std::string& filepath, const std::string& data) {
    try {
        // Ensure parent directory exists
        fs::path path(filepath);
        if (path.has_parent_path()) {
            ensureDirectoryExists(path.parent_path().string());
        }
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            Logger::error("Failed to open file for writing: " + filepath);
            return false;
        }
        
        file << data;
        file.close();
        
        Logger::info("Saved data to: " + filepath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to save to file '" + filepath + "': " + e.what());
        return false;
    }
}

bool JsonPersistence::loadFromFile(const std::string& filepath, std::string& data) {
    try {
        if (!fs::exists(filepath)) {
            Logger::warn("File does not exist: " + filepath);
            return false;
        }
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::error("Failed to open file for reading: " + filepath);
            return false;
        }
        
        // Read entire file into string
        data.assign(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
        
        file.close();
        
        Logger::info("Loaded data from: " + filepath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to load from file '" + filepath + "': " + e.what());
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
        Logger::info("Deleted file: " + filepath);
        return true;
        
    } catch (const std::exception& e) {
        Logger::error("Failed to delete file '" + filepath + "': " + e.what());
        return false;
    }
}

std::string JsonPersistence::serialize(const void* data) {
    if (!data) return "{}";
    
    // Stubbed implementation - return empty JSON object
    Logger::warn("JsonPersistence::serialize() stubbed - JSON library missing");
    return "{}";
}

bool JsonPersistence::deserialize(const std::string& serialized, void* data) {
    if (serialized.empty() || !data) return false;
    
    // Stubbed implementation
    Logger::warn("JsonPersistence::deserialize() stubbed - JSON library missing");
    return false; 
}

bool JsonPersistence::ensureDirectoryExists(const std::string& dirPath) {
    try {
        if (fs::exists(dirPath)) {
            return fs::is_directory(dirPath);
        }
        
        return fs::create_directories(dirPath);
        
    } catch (const std::exception& e) {
        Logger::error("Failed to create directory '" + dirPath + "': " + e.what());
        return false;
    }
}

bool JsonPersistence::isValidJson(const std::string& jsonStr) {
    if (jsonStr.empty()) return false;
    // Simple basic check for stub
    char first = jsonStr.front();
    char last = jsonStr.back();
    return (first == '{' && last == '}') || (first == '[' && last == ']');
}
