#include "../../../inc/app/model/Playlist.h"
#include "../../../inc/utils/Logger.h"
#include <algorithm>
#include <random>

Playlist::Playlist(const std::string& name, IPersistence* persistence)
    : name_(name), repeatMode_(RepeatMode::NONE), persistence_(persistence) {
}

bool Playlist::addTrack(std::shared_ptr<MediaFile> track) {
    if (!track) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    tracks_.push_back(track);
    
    Logger::getInstance().info("Added track to playlist '" + name_ + "': " + track->getPath());
    Subject::notify();
    return true;
}

bool Playlist::insertTrack(std::shared_ptr<MediaFile> track, size_t position) {
    if (!track) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (position > tracks_.size()) {
        return false;
    }
    
    tracks_.insert(tracks_.begin() + position, track);
    Subject::notify();
    return true;
}

bool Playlist::removeTrack(size_t index) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (index >= tracks_.size()) {
        return false;
    }
    
    tracks_.erase(tracks_.begin() + index);
    Logger::getInstance().info("Removed track at index " + std::to_string(index) + " from playlist '" + name_ + "'");
    Subject::notify();
    return true;
}

bool Playlist::removeTrackByPath(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&filepath](const std::shared_ptr<MediaFile>& track) {
            return track && track->getPath() == filepath;
        });
    
    if (it != tracks_.end()) {
        tracks_.erase(it);
        Logger::getInstance().info("Removed track from playlist '" + name_ + "': " + filepath);
        Subject::notify();
        return true;
    }
    
    return false;
}



std::shared_ptr<MediaFile> Playlist::getTrack(size_t index) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (index >= tracks_.size()) {
        return nullptr;
    }
    
    return tracks_[index];
}

void Playlist::clear() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    tracks_.clear();
    
    Logger::getInstance().info("Cleared playlist '" + name_ + "'");
    Subject::notify();
}

void Playlist::shuffle() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (tracks_.size() <= 1) {
        return;  // Nothing to shuffle
    }
    
    // Fisher-Yates shuffle algorithm
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (size_t i = tracks_.size() - 1; i > 0; --i) {
        std::uniform_int_distribution<size_t> dis(0, i);
        size_t j = dis(gen);
        std::swap(tracks_[i], tracks_[j]);
    }
    
    Logger::getInstance().info("Shuffled playlist '" + name_ + "'");
    Subject::notify();
}

void Playlist::rename(const std::string& newName) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    name_ = newName;
    Subject::notify();
}

bool Playlist::save() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for Playlist");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // TODO: Implement persistence save
        // persistence_->save("playlist_" + name_, tracks_);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to save playlist '" + name_ + "': " + std::string(e.what()));
        return false;
    }
}

bool Playlist::load() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for Playlist");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // TODO: Implement persistence load
        // tracks_ = persistence_->load<MediaFile>("playlist_" + name_);
        Subject::notify();
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to load playlist '" + name_ + "': " + std::string(e.what()));
        return false;
    }
}

bool Playlist::contains(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    for (const auto& track : tracks_) {
        if (track && track->getPath() == filepath) {
            return true;
        }
    }
    
    return false;
}

