#ifndef HYBRID_METADATA_READER_H
#define HYBRID_METADATA_READER_H

#include "interfaces/IMetadataReader.h"
#include <memory>

/**
 * @file HybridMetadataReader.h
 * @brief Composite Metadata Reader
 * 
 * Combines TagLib (fast, audio-focused) and Mpv (slower, unified format support).
 * Prioritizes TagLib, falls back to Mpv for video or missing data.
 */
class HybridMetadataReader : public IMetadataReader
{
public:
    HybridMetadataReader(std::unique_ptr<IMetadataReader> primary, 
                         std::unique_ptr<IMetadataReader> secondary);
    ~HybridMetadataReader() override = default;

    MediaMetadata readMetadata(const std::string &filepath) override;
    bool writeMetadata(const std::string &filepath, const MediaMetadata &metadata) override;
    std::map<std::string, std::string> extractTags(const std::string &filepath,
                                                   const std::vector<std::string> &tags) override;
    bool supportsEditing(const std::string &filepath) override;

private:
    std::unique_ptr<IMetadataReader> primary_;   // TagLib
    std::unique_ptr<IMetadataReader> secondary_; // Mpv
};

#endif // HYBRID_METADATA_READER_H
