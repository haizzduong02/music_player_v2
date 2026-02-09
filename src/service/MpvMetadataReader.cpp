#include "service/MpvMetadataReader.h"
#include "utils/Logger.h"
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cmath>

MpvMetadataReader::MpvMetadataReader()
{
    mpv_ = mpv_create();
    if (!mpv_)
    {
        throw std::runtime_error("Failed to create mpv context for metadata reading");
    }

    mpv_set_option_string(mpv_, "vo", "null");
    mpv_set_option_string(mpv_, "ao", "null");
    mpv_set_option_string(mpv_, "ytdl", "no"); // Disable ytdl for speed/safety
    
    if (mpv_initialize(mpv_) < 0)
    {
        throw std::runtime_error("Failed to initialize mpv for metadata reading");
    }
}

MpvMetadataReader::~MpvMetadataReader()
{
    if (mpv_)
    {
        mpv_terminate_destroy(mpv_);
    }
}

MediaMetadata MpvMetadataReader::readMetadata(const std::string &filepath)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaMetadata metadata;

    
    if (!mpv_) return metadata;

    const char *cmd[] = {"loadfile", filepath.c_str(), NULL};
    mpv_command(mpv_, cmd);

    bool done = false;
    bool error = false;
    
    int timeoutMs = 2000;
    while (!done)
    {
        mpv_event *event = mpv_wait_event(mpv_, 0.1); // 100ms wait
        
        if (event->event_id == MPV_EVENT_NONE)
        {
            timeoutMs -= 100;
            if (timeoutMs <= 0) 
            {
                Logger::warn("Timeout waiting for metadata probe: " + filepath);
                done = true;
                error = true;
            }
            continue;
        }

        if (event->event_id == MPV_EVENT_FILE_LOADED)
        {
            done = true;
        }
        else if (event->event_id == MPV_EVENT_END_FILE)
        {
            mpv_event_end_file *endFile = (mpv_event_end_file *)event->data;
            if (endFile->reason == MPV_END_FILE_REASON_ERROR)
            {
                Logger::warn("Failed to load file for metadata: " + filepath);
                error = true;
            }
            done = true;
        }
    }

    if (!error)
    {
        // Extract Duration
        double durationFunc = 0;
        if (mpv_get_property(mpv_, "duration", MPV_FORMAT_DOUBLE, &durationFunc) >= 0)
        {
            metadata.duration = static_cast<int>(std::round(durationFunc));
        }

        // "media-title" is often the most reliable "Title" fallback
        
        char* value = nullptr;
        if (mpv_get_property(mpv_, "media-title", MPV_FORMAT_STRING, &value) >= 0)
        {
            if (value) 
            {
                metadata.title = value;
                mpv_free(value);
            }
        }
        
        // Try specific metadata tags using mpv property "metadata" is harder directly via C API get_property without iterating node map.
        
        auto readTag = [&](const char* key) -> std::string {
            char* val = nullptr;
            std::string prop = std::string("metadata/by-key/") + key;
            if (mpv_get_property(mpv_, prop.c_str(), MPV_FORMAT_STRING, &val) >= 0 && val)
            {
                std::string result = val;
                mpv_free(val);
                return result;
            }
            return "";
        };

        std::string artist = readTag("Artist");
        if (artist.empty()) artist = readTag("artist"); // Case sensitivity varies
        if (!artist.empty()) metadata.artist = artist;

        std::string album = readTag("Album");
        if (album.empty()) album = readTag("album");
        if (!album.empty()) metadata.album = album;
        
        std::string genre = readTag("Genre");
        if (genre.empty()) genre = readTag("genre");
        if (!genre.empty()) metadata.genre = genre;
        
        std::string date = readTag("Date");
        if (date.empty()) date = readTag("date");
        if (date.empty()) date = readTag("Year");
        if (!date.empty()) 
        {
            try { metadata.year = std::stoi(date); } catch (...) {}
        }
    }
    
    const char *stopCmd[] = {"stop", NULL};
    mpv_command(mpv_, stopCmd);

    return metadata;
}

bool MpvMetadataReader::writeMetadata(const std::string &filepath, const MediaMetadata &metadata)
{
    return false;
}

std::map<std::string, std::string> MpvMetadataReader::extractTags(const std::string &filepath,
                                                                  const std::vector<std::string> &tags)
{
    return {}; 
}

bool MpvMetadataReader::supportsEditing(const std::string &filepath)
{
    return false;
}
