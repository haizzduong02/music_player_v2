#include "../../../inc/app/model/MediaFileFactory.h"
#include "../../../inc/utils/Logger.h"
#include <algorithm>
#include <cctype>

std::unique_ptr<MediaFile> MediaFileFactory::createMediaFile(
    const std::string& filepath,
    IMetadataReader* metadataReader) {
    
    // Create MediaFile first
    auto mediaFile = std::make_unique<MediaFile>(filepath);
    
    // Read metadata if reader is provided
    if (metadataReader) {
        try {
            MediaMetadata metadata = metadataReader->readMetadata(filepath);
            mediaFile->setMetadata(metadata);
        } catch (const std::exception& e) {
            Logger::getInstance().warn("Failed to read metadata for: " + filepath + " - " + e.what());
        }
    }
    
    return mediaFile;
}

std::unique_ptr<MediaFile> MediaFileFactory::createMediaFileWithMetadata(
    const std::string& filepath,
    const MediaMetadata& metadata) {
    
    return std::make_unique<MediaFile>(filepath, metadata);
}

bool MediaFileFactory::isSupportedFormat(const std::string& extension) {
    std::string ext = extension;
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Add dot if missing
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    auto allFormats = getAllSupportedFormats();
    return std::find(allFormats.begin(), allFormats.end(), ext) != allFormats.end();
}

std::vector<std::string> MediaFileFactory::getSupportedAudioFormats() {
    return {
        ".mp3", ".flac", ".wav", ".m4a", ".aac",
        ".ogg", ".wma", ".opus", ".ape", ".alac"
    };
}

std::vector<std::string> MediaFileFactory::getSupportedVideoFormats() {
    return {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv",
        ".flv", ".webm", ".m4v", ".mpeg", ".mpg"
    };
}

std::vector<std::string> MediaFileFactory::getAllSupportedFormats() {
    auto audio = getSupportedAudioFormats();
    auto video = getSupportedVideoFormats();
    
    // Combine both vectors
    std::vector<std::string> all;
    all.reserve(audio.size() + video.size());
    all.insert(all.end(), audio.begin(), audio.end());
    all.insert(all.end(), video.begin(), video.end());
    
    return all;
}
