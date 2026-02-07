#ifndef MEDIA_FILE_H
#define MEDIA_FILE_H

#include "interfaces/IMetadataReader.h"
#include <string>
#include <json.hpp>
#include <memory>

/**
 * @file MediaFile.h
 * @brief Media file representation
 * 
 * Represents a single media file with its metadata and state.
 */

/**
 * @brief Media type enumeration
 */
enum class MediaType {
    AUDIO,
    VIDEO,
    IMAGE,
    UNKNOWN
};

/**
 * @brief Media file class
 * 
 * Encapsulates all information about a media file including
 * path, metadata, and library membership status.
 */
class MediaFile {
public:
    /**
     * @brief Constructor
     * @param filepath Full path to the media file
     */
    explicit MediaFile(const std::string& filepath);
    
    /**
     * @brief Constructor with metadata
     * @param filepath Full path to the media file
     * @param metadata Metadata for the file
     */
    MediaFile(const std::string& filepath, const MediaMetadata& metadata);
    
    // Getters
    const std::string& getPath() const { return filepath_; }
    const std::string& getFileName() const { return filename_; }
    const std::string& getExtension() const { return extension_; }
    const MediaMetadata& getMetadata() const { return metadata_; }
    MediaType getType() const { return type_; }
    bool isInLibrary() const { return inLibrary_; }
    
    // Setters
    void setMetadata(const MediaMetadata& metadata) { metadata_ = metadata; }
    void setInLibrary(bool inLibrary) { inLibrary_ = inLibrary; }
    
    /**
     * @brief Get display name (title if available, filename otherwise)
     * @return Display name
     */
    std::string getDisplayName() const;
    
    /**
     * @brief Check if file exists on disk
     * @return true if file exists
     */
    bool exists() const;
    
    /**
     * @brief Get file size in bytes
     * @return File size
     */
    size_t getFileSize() const;
    
    // JSON serialization support
    friend void to_json(nlohmann::json& j, const MediaFile& m);
    friend void from_json(const nlohmann::json& j, MediaFile& m);
    
private:
    std::string filepath_;
    std::string filename_;
    std::string extension_;
    MediaMetadata metadata_;
    MediaType type_;
    bool inLibrary_;
    
    /**
     * @brief Parse filepath to extract filename and extension
     */
    void parseFilePath();
    
    /**
     * @brief Determine media type from extension
     */
    void determineMediaType();
};

#endif // MEDIA_FILE_H
