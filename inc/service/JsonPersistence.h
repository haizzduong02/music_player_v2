#ifndef JSON_PERSISTENCE_H
#define JSON_PERSISTENCE_H

#include "interfaces/IPersistence.h"
#include <string>

/**
 * @file JsonPersistence.h
 * @brief Concrete implementation of IPersistence using JSON
 * 
 * Provides data persistence using JSON format.
 * Can use nlohmann/json or similar JSON library.
 */

/**
 * @brief JSON persistence class
 * 
 * Concrete implementation of IPersistence.
 * Serializes/deserializes data to/from JSON format.
 */
class JsonPersistence : public IPersistence {
public:
    /**
     * @brief Constructor
     */
    JsonPersistence() = default;
    
    /**
     * @brief Destructor
     */
    ~JsonPersistence() override = default;
    
    // IPersistence implementation
    bool saveToFile(const std::string& filepath, const std::string& data) override;
    
    bool loadFromFile(const std::string& filepath, std::string& data) override;
    
    bool fileExists(const std::string& filepath) override;
    
    bool deleteFile(const std::string& filepath) override;
    
    std::string serialize(const void* data) override;
    
    bool deserialize(const std::string& serialized, void* data) override;
    
    /**
     * @brief Create directory if it doesn't exist
     * @param dirPath Directory path
     * @return true if created or already exists
     */
    bool ensureDirectoryExists(const std::string& dirPath);
    
private:
    /**
     * @brief Validate JSON string
     * @param jsonStr JSON string
     * @return true if valid JSON
     */
    bool isValidJson(const std::string& jsonStr);
};

#endif // JSON_PERSISTENCE_H
