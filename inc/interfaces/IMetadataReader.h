#ifndef IMETADATA_READER_H
#define IMETADATA_READER_H

#include <map>
#include <string>
#include <vector>

/**
 * @file IMetadataReader.h
 * @brief Interface for reading and writing media metadata (Dependency Inversion Principle)
 *
 * Abstracts metadata operations to allow different implementations (TagLib, FFmpeg, etc.)
 */

/**
 * @brief Media metadata structure
 */
struct MediaMetadata
{
    std::string title;
    std::string artist;
    std::string album;
    std::string genre;
    int year = 0;
    int track = 0;
    int duration = 0;     // in seconds
    int bitrate = 0;      // in kbps
    int sampleRate = 0;   // in Hz (e.g., 44100, 48000)
    int channels = 0;     // number of audio channels (1=mono, 2=stereo)
    bool hasAlbumArt = false; // true if file has embedded album artwork
    std::string codec;
    std::string comment;

    // Album art data (raw bytes - typically JPEG or PNG)
    std::vector<unsigned char> albumArtData;
    std::string albumArtMimeType; // e.g., "image/jpeg", "image/png"

    // Additional fields can be stored here
    std::map<std::string, std::string> customFields;
};

/**
 * @brief Metadata reader/writer interface
 *
 * Provides methods for extracting and modifying metadata from media files.
 */
class IMetadataReader
{
  public:
    virtual ~IMetadataReader() = default;

    /**
     * @brief Read metadata from a media file
     * @param filepath Path to the media file
     * @return MediaMetadata structure containing all metadata
     */
    virtual MediaMetadata readMetadata(const std::string &filepath) = 0;

    /**
     * @brief Write metadata to a media file
     * @param filepath Path to the media file
     * @param metadata Metadata to write
     * @return true if metadata was written successfully
     */
    virtual bool writeMetadata(const std::string &filepath, const MediaMetadata &metadata) = 0;

    /**
     * @brief Extract specific tags from a media file
     * @param filepath Path to the media file
     * @param tags Vector of tag names to extract (e.g., {"ARTIST", "TITLE"})
     * @return Map of tag names to values
     */
    virtual std::map<std::string, std::string> extractTags(const std::string &filepath,
                                                           const std::vector<std::string> &tags) = 0;

    /**
     * @brief Check if a file format supports metadata editing
     * @param filepath Path to the media file
     * @return true if format supports editing
     */
    virtual bool supportsEditing(const std::string &filepath) = 0;
};

#endif // IMETADATA_READER_H
