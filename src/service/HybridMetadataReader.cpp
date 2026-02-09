#include "service/HybridMetadataReader.h"
#include "utils/Logger.h"

HybridMetadataReader::HybridMetadataReader(std::unique_ptr<IMetadataReader> primary, 
                                           std::unique_ptr<IMetadataReader> secondary)
    : primary_(std::move(primary)), secondary_(std::move(secondary))
{
}

MediaMetadata HybridMetadataReader::readMetadata(const std::string &filepath)
{
    MediaMetadata metadata = primary_->readMetadata(filepath);

    // Heuristic: If duration is 0, primary failed or file is unsupported (like video).
    // Or if extension is a known video format (could check extension, but duration 0 is a good signal).
    if (metadata.duration == 0)
    {
        Logger::info("Primary metadata reader incomplete for " + filepath + ", falling back to secondary.");
        MediaMetadata mpvMetadata = secondary_->readMetadata(filepath);
        
        // Merge - prioritize mpv if TagLib failed
        if (mpvMetadata.duration > 0)
        {
            metadata.duration = mpvMetadata.duration;
        }
        
        // Use mpv strings if TagLib strings are empty
        if (metadata.title.empty() && !mpvMetadata.title.empty())
            metadata.title = mpvMetadata.title;
            
        if (metadata.artist.empty() && !mpvMetadata.artist.empty())
            metadata.artist = mpvMetadata.artist;
            
        if (metadata.album.empty() && !mpvMetadata.album.empty())
            metadata.album = mpvMetadata.album;
            
        // Currently MpvMetadataReader doesn't extract album art, so keep TagLib's (if any)
    }

    return metadata;
}

bool HybridMetadataReader::writeMetadata(const std::string &filepath, const MediaMetadata &metadata)
{
    if (primary_->supportsEditing(filepath))
    {
        return primary_->writeMetadata(filepath, metadata);
    }
    return false;
}

std::map<std::string, std::string> HybridMetadataReader::extractTags(const std::string &filepath,
                                                                     const std::vector<std::string> &tags)
{
    auto result = primary_->extractTags(filepath, tags);
    if (result.empty())
    {
        return secondary_->extractTags(filepath, tags);
    }
    return result;
}

bool HybridMetadataReader::supportsEditing(const std::string &filepath)
{
    return primary_->supportsEditing(filepath) || secondary_->supportsEditing(filepath);
}
