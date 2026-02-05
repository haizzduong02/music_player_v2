#include "../../inc/service/TagLibMetadataReader.h"
#include "../../inc/utils/Logger.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <algorithm>

MediaMetadata TagLibMetadataReader::readMetadata(const std::string& filepath) {
    MediaMetadata metadata;

    // Determine codec from file extension first (fallback if TagLib fails)
    std::string ext = getExtension(filepath);
    if (ext == ".mp3") metadata.codec = "MP3";
    else if (ext == ".flac") metadata.codec = "FLAC";
    else if (ext == ".wav") metadata.codec = "WAV";
    else if (ext == ".m4a") metadata.codec = "AAC";
    else if (ext == ".ogg") metadata.codec = "Vorbis";
    else metadata.codec = "Unknown";
    
    TagLib::FileRef file(filepath.c_str());
    
    if (file.isNull() || !file.tag()) {
        Logger::getInstance().warn("Failed to read detailed metadata from: " + filepath);
        return metadata;
    }
    
    TagLib::Tag* tag = file.tag();
    
    // Read basic tags
    metadata.title = tag->title().to8Bit(true);
    metadata.artist = tag->artist().to8Bit(true);
    metadata.album = tag->album().to8Bit(true);
    metadata.genre = tag->genre().to8Bit(true);
    metadata.year = tag->year();
    metadata.track = tag->track();
    metadata.comment = tag->comment().to8Bit(true);
    
    // Read audio properties
    if (file.audioProperties()) {
        TagLib::AudioProperties* props = file.audioProperties();
        metadata.duration = props->lengthInSeconds();
        metadata.bitrate = props->bitrate();
        metadata.sampleRate = props->sampleRate();
        metadata.channels = props->channels();
    }
    
    // Audio properties read above
    
    // Codec already determined at start of function
    
    // Extract album art
    metadata.hasAlbumArt = false;
    
    // Try MP3 (ID3v2)
    if (ext == ".mp3") {
        TagLib::MPEG::File mpegFile(filepath.c_str());
        if (mpegFile.isValid() && mpegFile.ID3v2Tag()) {
            TagLib::ID3v2::Tag* id3v2 = mpegFile.ID3v2Tag();
            auto frames = id3v2->frameList("APIC");
            if (!frames.isEmpty()) {
                auto* pictureFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
                if (pictureFrame) {
                    auto pictureData = pictureFrame->picture();
                    metadata.albumArtData.assign(pictureData.begin(), pictureData.end());
                    metadata.albumArtMimeType = pictureFrame->mimeType().to8Bit(true);
                    metadata.hasAlbumArt = true;
                }
            }
        }
    }
    // Try FLAC
    else if (ext == ".flac") {
        TagLib::FLAC::File flacFile(filepath.c_str());
        if (flacFile.isValid()) {
            auto pictures = flacFile.pictureList();
            if (!pictures.isEmpty()) {
                auto* picture = pictures.front();
                auto pictureData = picture->data();
                metadata.albumArtData.assign(pictureData.begin(), pictureData.end());
                metadata.albumArtMimeType = picture->mimeType().to8Bit(true);
                metadata.hasAlbumArt = true;
            }
        }
    }
    
    return metadata;
}

bool TagLibMetadataReader::writeMetadata(const std::string& filepath, const MediaMetadata& metadata) {
    if (!supportsEditing(filepath)) {
        Logger::getInstance().warn("Format does not support editing: " + filepath);
        return false;
    }
    
    TagLib::FileRef file(filepath.c_str());
    
    if (file.isNull() || !file.tag()) {
        Logger::getInstance().error("Failed to open file for writing: " + filepath);
        return false;
    }
    
    TagLib::Tag* tag = file.tag();
    
    // Write basic tags
    tag->setTitle(TagLib::String(metadata.title, TagLib::String::UTF8));
    tag->setArtist(TagLib::String(metadata.artist, TagLib::String::UTF8));
    tag->setAlbum(TagLib::String(metadata.album, TagLib::String::UTF8));
    tag->setGenre(TagLib::String(metadata.genre, TagLib::String::UTF8));
    tag->setYear(metadata.year);
    tag->setTrack(metadata.track);
    tag->setComment(TagLib::String(metadata.comment, TagLib::String::UTF8));
    
    bool success = file.save();
    
    if (success) {
        Logger::getInstance().info("Metadata saved for: " + filepath);
    } else {
        Logger::getInstance().error("Failed to save metadata for: " + filepath);
    }
    
    return success;
}

std::map<std::string, std::string> TagLibMetadataReader::extractTags(
    const std::string& filepath,
    const std::vector<std::string>& tags) {
    
    std::map<std::string, std::string> result;
    
    TagLib::FileRef file(filepath.c_str());
    if (file.isNull() || !file.tag()) {
        return result;
    }
    
    TagLib::Tag* tag = file.tag();
    
    for (const auto& tagName : tags) {
        std::string lowerTag = tagName;
        std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
        
        if (lowerTag == "title") {
            result[tagName] = tag->title().to8Bit(true);
        } else if (lowerTag == "artist") {
            result[tagName] = tag->artist().to8Bit(true);
        } else if (lowerTag == "album") {
            result[tagName] = tag->album().to8Bit(true);
        } else if (lowerTag == "genre") {
            result[tagName] = tag->genre().to8Bit(true);
        } else if (lowerTag == "year") {
            result[tagName] = std::to_string(tag->year());
        } else if (lowerTag == "track") {
            result[tagName] = std::to_string(tag->track());
        }
    }
    
    return result;
}

bool TagLibMetadataReader::supportsEditing(const std::string& filepath) {
    std::string ext = getExtension(filepath);
    return isFormatSupported(ext);
}

std::string TagLibMetadataReader::getExtension(const std::string& filepath) {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    
    std::string ext = filepath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool TagLibMetadataReader::isFormatSupported(const std::string& extension) {
    static const std::vector<std::string> supportedFormats = {
        ".mp3", ".flac", ".ogg", ".m4a", ".wav", ".wma", ".ape"
    };
    
    return std::find(supportedFormats.begin(), supportedFormats.end(), extension) 
           != supportedFormats.end();
}
