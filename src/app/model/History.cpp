#include "../../../inc/app/model/History.h"
#include "../../../inc/utils/Logger.h"
#include <algorithm>

History::History(size_t maxSize, IPersistence* persistence)
    : maxSize_(maxSize), persistence_(persistence) {
}

void History::addTrack(std::shared_ptr<MediaFile> track) {
    if (!track) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Check if track already exists
    int existingIndex = findTrackIndex(track->getPath());
    
    if (existingIndex >= 0) {
        // Move existing track to top (most recent)
        history_.erase(history_.begin() + existingIndex);
    }
    
    // Add to front (most recent)
    history_.insert(history_.begin(), track);
    
    // Trim to max size
    trimToMaxSize();
    
    Logger::getInstance().debug("Added to history: " + track->getPath());
    Subject::notify();
}

bool History::removeTrack(size_t index) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (index >= history_.size()) {
        return false;
    }
    
    history_.erase(history_.begin() + index);
    Subject::notify();
    return true;
}

bool History::removeTrackByPath(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    int index = findTrackIndex(filepath);
    if (index < 0) {
        return false;
    }
    
    history_.erase(history_.begin() + index);
    Subject::notify();
    return true;
}

void History::clear() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    history_.clear();
    
    Logger::getInstance().info("History cleared");
    Subject::notify();
}

std::vector<std::shared_ptr<MediaFile>> History::getRecent(size_t count) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    size_t actualCount = std::min(count, history_.size());
    return std::vector<std::shared_ptr<MediaFile>>(
        history_.begin(),
        history_.begin() + actualCount
    );
}

std::shared_ptr<MediaFile> History::getTrack(size_t index) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (index >= history_.size()) {
        return nullptr;
    }
    
    return history_[index];
}

void History::setMaxSize(size_t maxSize) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    maxSize_ = maxSize;
    trimToMaxSize();
    Subject::notify();
}

bool History::save() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for History");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // TODO: Implement persistence save
        // persistence_->save("history", history_);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to save history: " + std::string(e.what()));
        return false;
    }
}

bool History::load() {
    if (!persistence_) {
        Logger::getInstance().warn("No persistence layer configured for History");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    try {
        // TODO: Implement persistence load
        // history_ = persistence_->load<MediaFile>("history");
        trimToMaxSize();
        Subject::notify();
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to load history: " + std::string(e.what()));
        return false;
    }
}

int History::findTrackIndex(const std::string& filepath) const {
    for (size_t i = 0; i < history_.size(); ++i) {
        if (history_[i]->getPath() == filepath) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void History::trimToMaxSize() {
    if (history_.size() > maxSize_) {
        history_.resize(maxSize_);
    }
}
