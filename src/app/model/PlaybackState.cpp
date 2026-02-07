#include "../../../inc/app/model/PlaybackState.h"
#include "../../../inc/utils/Logger.h"
#include <algorithm>

PlaybackState::PlaybackState()
    : currentTrack_(nullptr),
      status_(PlaybackStatus::STOPPED),
      volume_(0.7f),  // Default 70% volume
      position_(0.0),
      duration_(0.0),
      queueIndex_(0) {
}

void PlaybackState::setPlayback(std::shared_ptr<MediaFile> track, PlaybackStatus status) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    currentTrack_ = track;
    status_ = status;
    position_ = 0.0;
    
    if (track) {
        duration_ = track->getMetadata().duration;
        Logger::getInstance().info("Playback set to: " + track->getPath());
    } else {
        duration_ = 0.0;
    }
    
    Subject::notify();
}

void PlaybackState::setStatus(PlaybackStatus status) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    status_ = status;
    Subject::notify();
}

void PlaybackState::setVolume(float volume) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    // Clamp volume to [0.0, 1.0]
    volume_ = std::max(0.0f, std::min(1.0f, volume));
    Subject::notify();
}

void PlaybackState::setPosition(double position) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    position_ = std::max(0.0, std::min(duration_, position));
    // Don't notify for position updates (too frequent)
}

void PlaybackState::setDuration(double duration) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    duration_ = duration;
    Subject::notify();
}

void PlaybackState::pushToBackStack() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (currentTrack_) {
        backStack_.push(currentTrack_);
    }
}

std::shared_ptr<MediaFile> PlaybackState::popFromBackStack() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (backStack_.empty()) {
        return nullptr;
    }
    
    auto track = backStack_.top();
    backStack_.pop();
    return track;
}

void PlaybackState::clearBackStack() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    while (!backStack_.empty()) {
        backStack_.pop();
    }
}

void PlaybackState::setPlayQueue(const std::vector<std::shared_ptr<MediaFile>>& queue) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    playQueue_ = queue;
    queueIndex_ = 0;
    Subject::notify();
}

void PlaybackState::setQueueIndex(size_t index) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (index <= playQueue_.size()) {
        queueIndex_ = index;
    }
}

void PlaybackState::syncQueueIndex(std::shared_ptr<MediaFile> track) {
    if (!track) return;
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (playQueue_.empty()) return;
    
    // Find track in queue
    for (size_t i = 0; i < playQueue_.size(); ++i) {
        if (playQueue_[i]->getPath() == track->getPath()) {
            // Set index to the NEXT track (i + 1)
            queueIndex_ = i + 1;
            return;
        }
    }
    // If not found, do nothing (maintain current queue position)
}

std::shared_ptr<MediaFile> PlaybackState::getNextTrack() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    if (playQueue_.empty() || queueIndex_ >= playQueue_.size()) {
        return nullptr;
    }
    
    return playQueue_[queueIndex_++];
}

bool PlaybackState::hasNextTrack() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return queueIndex_ < playQueue_.size();
}

void PlaybackState::clearPlayQueue() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    playQueue_.clear();
    queueIndex_ = 0;
    Subject::notify();
}


