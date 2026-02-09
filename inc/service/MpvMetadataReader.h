#ifndef MPV_METADATA_READER_H
#define MPV_METADATA_READER_H

#include "interfaces/IMetadataReader.h"
#include <mpv/client.h>
#include <mutex>

/**
 * @file MpvMetadataReader.h
 * @brief IMetadataReader implementation using libmpv
 * 
 * Extracts metadata (duration, title, etc.) using a standalone mpv handle.
 * Useful for video formats not supported by TagLib.
 */
class MpvMetadataReader : public IMetadataReader
{
public:
    MpvMetadataReader();
    ~MpvMetadataReader() override;

    // IMetadataReader implementation
    MediaMetadata readMetadata(const std::string &filepath) override;
    bool writeMetadata(const std::string &filepath, const MediaMetadata &metadata) override;
    std::map<std::string, std::string> extractTags(const std::string &filepath,
                                                   const std::vector<std::string> &tags) override;
    bool supportsEditing(const std::string &filepath) override;

private:
    mpv_handle *mpv_ = nullptr;
    std::mutex mutex_; // Protect access to mpv handle
};

#endif // MPV_METADATA_READER_H
