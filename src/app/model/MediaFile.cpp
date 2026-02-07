#include "../../../inc/app/model/MediaFile.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <json.hpp>

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
    std::string cleanName = filename_;
    
    // Remove extension
    size_t lastDot = cleanName.find_last_of('.');
    if (lastDot != std::string::npos) {
        cleanName = cleanName.substr(0, lastDot);
    }
    
    // Clean up common download prefixes (case-insensitive check would be better but simple for now)
    static const std::vector<std::string> prefixesToRemove = {
        "y2mate.com - ",
        "y2mate.is - "
    };
    
    for (const auto& prefix : prefixesToRemove) {
        if (cleanName.length() > prefix.length() && 
            cleanName.compare(0, prefix.length(), prefix) == 0) {
            cleanName = cleanName.substr(prefix.length());
            break;
        }
    }
    
    return cleanName;
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

// JSON Serialization
void to_json(nlohmann::json& j, const MediaFile& m) {
    j = nlohmann::json{
        {"path", m.filepath_},
        {"metadata", {
            {"title", m.metadata_.title},
            {"artist", m.metadata_.artist},
            {"album", m.metadata_.album},
            {"genre", m.metadata_.genre},
            {"year", m.metadata_.year},
            {"track", m.metadata_.track},
            {"duration", m.metadata_.duration} 
        }},
        {"inLibrary", m.inLibrary_}
    };
}

void from_json(const nlohmann::json& j, MediaFile& m) {
    if (j.contains("path")) m.filepath_ = j.at("path").get<std::string>();
    // Re-parse path to get filename/ext
    m.parseFilePath();
    m.determineMediaType();
    
    if (j.contains("inLibrary")) m.inLibrary_ = j.at("inLibrary").get<bool>();
    
    if (j.contains("metadata")) {
        const auto& meta = j.at("metadata");
        if (meta.contains("title")) m.metadata_.title = meta.at("title").get<std::string>();
        if (meta.contains("artist")) m.metadata_.artist = meta.at("artist").get<std::string>();
        if (meta.contains("album")) m.metadata_.album = meta.at("album").get<std::string>();
        if (meta.contains("genre")) m.metadata_.genre = meta.at("genre").get<std::string>();
        if (meta.contains("year")) m.metadata_.year = meta.at("year").get<int>();
        if (meta.contains("track")) m.metadata_.track = meta.at("track").get<int>();
        if (meta.contains("duration")) m.metadata_.duration = meta.at("duration").get<int>();
    }
}
