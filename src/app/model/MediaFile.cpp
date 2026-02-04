#include "../../../inc/app/model/MediaFile.h"
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

MediaFile::MediaFile(const std::string& filepath)
    : filepath_(filepath), inLibrary_(false) {
    parseFilePath();
    determineMediaType();
}

MediaFile::MediaFile(const std::string& filepath, const MediaMetadata& metadata)
    : filepath_(filepath), metadata_(metadata), inLibrary_(false) {
    parseFilePath();
    determineMediaType();
}

std::string MediaFile::getDisplayName() const {
    // Return title if available and not empty
    if (!metadata_.title.empty()) {
        return metadata_.title;
    }
    // Otherwise return filename without extension
    size_t lastDot = filename_.find_last_of('.');
    if (lastDot != std::string::npos) {
        return filename_.substr(0, lastDot);
    }
    return filename_;
}

bool MediaFile::exists() const {
    return fs::exists(filepath_);
}

size_t MediaFile::getFileSize() const {
    try {
        return fs::file_size(filepath_);
    } catch (const fs::filesystem_error&) {
        return 0;
    }
}

void MediaFile::parseFilePath() {
    fs::path path(filepath_);
    filename_ = path.filename().string();
    extension_ = path.extension().string();
    
    // Convert extension to lowercase
    std::transform(extension_.begin(), extension_.end(), extension_.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}

void MediaFile::determineMediaType() {
    // Audio formats
    static const std::vector<std::string> audioExts = {
        ".mp3", ".flac", ".wav", ".m4a", ".aac", ".ogg", ".wma", ".opus"
    };
    
    // Video formats
    static const std::vector<std::string> videoExts = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv", ".webm"
    };
    
    // Image formats
    static const std::vector<std::string> imageExts = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp"
    };
    
    if (std::find(audioExts.begin(), audioExts.end(), extension_) != audioExts.end()) {
        type_ = MediaType::AUDIO;
    } else if (std::find(videoExts.begin(), videoExts.end(), extension_) != videoExts.end()) {
        type_ = MediaType::VIDEO;
    } else if (std::find(imageExts.begin(), imageExts.end(), extension_) != imageExts.end()) {
        type_ = MediaType::IMAGE;
    } else {
        type_ = MediaType::UNKNOWN;
    }
}
