#ifndef MEDIA_FILE_FACTORY_H
#define MEDIA_FILE_FACTORY_H

#include "MediaFile.h"
#include "../../interfaces/IMetadataReader.h"
#include <memory>
#include <string>

/**
 * @file MediaFileFactory.h
 * @brief Factory for creating MediaFile objects
 * 
 * Creates appropriate MediaFile instances based on file extension.
 */

/**
 * @brief Media file factory class
 * 
 * Implements Factory Method pattern to create MediaFile objects.
 * Can be extended to create different types of media objects.
 */
class MediaFileFactory {
public:
    /**
     * @brief Create a MediaFile from a file path
     * @param filepath Path to the media file
     * @param metadataReader Metadata reader to extract tags
     * @return Unique pointer to MediaFile
     */
    static std::unique_ptr<MediaFile> createMediaFile(
        const std::string& filepath,
        IMetadataReader* metadataReader = nullptr);
    
    /**
     * @brief Create a MediaFile with pre-loaded metadata
     * @param filepath Path to the media file
     * @param metadata Pre-loaded metadata
     * @return Unique pointer to MediaFile
     */
    static std::unique_ptr<MediaFile> createMediaFileWithMetadata(
        const std::string& filepath,
        const MediaMetadata& metadata);
    
    /**
     * @brief Check if a file extension is supported
     * @param extension File extension (e.g., ".mp3")
     * @return true if supported
     */
    static bool isSupportedFormat(const std::string& extension);
    
    /**
     * @brief Get list of supported audio formats
     * @return Vector of extensions
     */
    static std::vector<std::string> getSupportedAudioFormats();
    
    /**
     * @brief Get list of supported video formats
     * @return Vector of extensions
     */
    static std::vector<std::string> getSupportedVideoFormats();
    
    /**
     * @brief Get all supported formats
     * @return Vector of all supported extensions
     */
    static std::vector<std::string> getAllSupportedFormats();
    
private:
    MediaFileFactory() = delete;  // Static class, no instantiation
};

#endif // MEDIA_FILE_FACTORY_H
