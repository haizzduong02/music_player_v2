#ifndef IPERSISTENCE_H
#define IPERSISTENCE_H

#include <string>

/**
 * @file IPersistence.h
 * @brief Interface for data persistence (Dependency Inversion Principle)
 * 
 * Abstracts data storage to allow different implementations (JSON, XML, SQLite, etc.)
 */

/**
 * @brief Persistence interface
 * 
 * Provides methods for saving and loading data to/from persistent storage.
 * Template-based to support different data types.
 */
class IPersistence {
public:
    virtual ~IPersistence() = default;
    
    /**
     * @brief Save data to a file
     * @param filepath Path to the file
     * @param data Data string to save (e.g., JSON string)
     * @return true if saved successfully
     */
    virtual bool saveToFile(const std::string& filepath, const std::string& data) = 0;
    
    /**
     * @brief Load data from a file
     * @param filepath Path to the file
     * @param data Output parameter for loaded data
     * @return true if loaded successfully
     */
    virtual bool loadFromFile(const std::string& filepath, std::string& data) = 0;
    
    /**
     * @brief Check if a file exists
     * @param filepath Path to check
     * @return true if file exists
     */
    virtual bool fileExists(const std::string& filepath) = 0;
    
    /**
     * @brief Delete a file
     * @param filepath Path to delete
     * @return true if deleted successfully
     */
    virtual bool deleteFile(const std::string& filepath) = 0;
    
    /**
     * @brief Serialize data to string format
     * @param data Data to serialize
     * @return Serialized string
     */
    virtual std::string serialize(const void* data) = 0;
    
    /**
     * @brief Deserialize string to data
     * @param serialized Serialized string
     * @param data Output parameter for deserialized data
     * @return true if deserialized successfully
     */
    virtual bool deserialize(const std::string& serialized, void* data) = 0;
};

#endif // IPERSISTENCE_H
