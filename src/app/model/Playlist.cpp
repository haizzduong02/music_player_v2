#include "app/model/Playlist.h"
#include "utils/Logger.h"
#include <algorithm>
#include <random>

Playlist::Playlist(const std::string &name) : name_(name), repeatMode_(RepeatMode::NONE)
{
}

bool Playlist::addTrack(std::shared_ptr<MediaFile> track)
{
    if (!track)
    {
        return false;
    }

    // Prevent duplicates
    if (contains(track->getPath()))
    {
        Logger::warn("Track already exists in playlist '" + name_ + "': " + track->getPath());
        return false;
    }

    std::lock_guard<std::mutex> lock(dataMutex_);
    tracks_.push_back(track);

    Logger::info("Added track to playlist '" + name_ + "': " + track->getPath());
    Subject::notify();
    return true;
}

bool Playlist::insertTrack(std::shared_ptr<MediaFile> track, size_t position)
{
    if (!track)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(dataMutex_);

    if (position > tracks_.size())
    {
        return false;
    }

    tracks_.insert(tracks_.begin() + position, track);
    Subject::notify();
    return true;
}

bool Playlist::removeTrack(size_t index)
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    if (index >= tracks_.size())
    {
        return false;
    }

    tracks_.erase(tracks_.begin() + index);
    Logger::info("Removed track at index " + std::to_string(index) + " from playlist '" + name_ + "'");
    Subject::notify();
    return true;
}

bool Playlist::removeTrackByPath(const std::string &filepath)
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    auto it = std::find_if(tracks_.begin(), tracks_.end(),
                           [&filepath](const std::shared_ptr<MediaFile> &track)
                           { return track && track->getPath() == filepath; });

    if (it != tracks_.end())
    {
        tracks_.erase(it);
        Logger::info("Removed track from playlist '" + name_ + "': " + filepath);
        Subject::notify();
        return true;
    }

    return false;
}

std::shared_ptr<MediaFile> Playlist::getTrack(size_t index) const
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    if (index >= tracks_.size())
    {
        return nullptr;
    }

    return tracks_[index];
}

void Playlist::clear()
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    tracks_.clear();

    Logger::info("Cleared playlist '" + name_ + "'");
    Subject::notify();
}

void Playlist::shuffle()
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    if (tracks_.size() <= 1)
    {
        return; // Nothing to shuffle
    }

    // Fisher-Yates shuffle algorithm
    std::random_device rd;
    std::mt19937 gen(rd());

    for (size_t i = tracks_.size() - 1; i > 0; --i)
    {
        std::uniform_int_distribution<size_t> dis(0, i);
        size_t j = dis(gen);
        std::swap(tracks_[i], tracks_[j]);
    }

    Logger::info("Shuffled playlist '" + name_ + "'");
    Subject::notify();
}

void Playlist::rename(const std::string &newName)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    name_ = newName;
    Subject::notify();
}

// ... (other includes)

void to_json(nlohmann::json &j, const Playlist &p)
{
    // Only save tracks, name is implied by filename usually, but valid to save it too
    std::vector<nlohmann::json> tracksJson;
    for (const auto &track : p.tracks_)
    {
        if (track)
        {
            nlohmann::json t;
            to_json(t, *track);
            tracksJson.push_back(t);
        }
    }

    j = nlohmann::json{{"name", p.name_}, {"tracks", tracksJson}};
}

void from_json(const nlohmann::json &j, Playlist &p)
{
    if (j.contains("name"))
        p.name_ = j.at("name").get<std::string>();
    // Tracks handled in load() mostly, but if used for full deserialization:
    if (j.contains("tracks"))
    {
        p.tracks_.clear();
        for (const auto &item : j["tracks"])
        {
            auto track = std::make_shared<MediaFile>("");
            item.get_to(*track);
            p.tracks_.push_back(track);
        }
    }
}

bool Playlist::contains(const std::string &filepath) const
{
    std::lock_guard<std::mutex> lock(dataMutex_);

    for (const auto &track : tracks_)
    {
        if (track && track->getPath() == filepath)
        {
            return true;
        }
    }

    return false;
}
