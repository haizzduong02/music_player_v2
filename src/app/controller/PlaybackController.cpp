#include "../../../inc/app/controller/PlaybackController.h"
#include "../../../inc/utils/Logger.h"

PlaybackController::PlaybackController(
    IPlaybackEngine* engine,
    PlaybackState* state,
    History* history,
    IHardwareInterface* hardware,
    Playlist* currentPlaylist)
    : state_(state), engine_(engine), history_(history), 
      hardware_(hardware), currentPlaylist_(currentPlaylist) {
}

bool PlaybackController::play(std::shared_ptr<MediaFile> track, bool pushToStack) {
    if (!track || !engine_ || !state_) {
        return false;
    }
    
    // Save current to back stack only if requested
    if (pushToStack && state_->getCurrentTrack()) {
        state_->pushToBackStack();
    }
    
    // Update state first
    state_->setCurrentTrack(track);
    state_->syncQueueIndex(track); // Sync queue index for correct Next behavior
    state_->setStatus(PlaybackStatus::PLAYING);
    
    // Reset position and set duration from metadata
    state_->setPosition(0.0);
    state_->setDuration(static_cast<double>(track->getMetadata().duration));
    
    // Add to history
    if (history_) {
        history_->addTrack(track);
    }
    
    // Play via engine
    return engine_->play(track->getPath());
}

void PlaybackController::pause() {
    if (engine_) {
        engine_->pause();
    }
    if (state_) {
        state_->setStatus(PlaybackStatus::PAUSED);
    }
}

void PlaybackController::resume() {
    if (engine_) {
        engine_->resume();
    }
    if (state_) {
        state_->setStatus(PlaybackStatus::PLAYING);
    }
}

void PlaybackController::stop() {
    if (engine_) {
        engine_->stop();
    }
    if (state_) {
        state_->setStatus(PlaybackStatus::STOPPED);
        state_->setPosition(0.0);
    }
}

bool PlaybackController::next() {
    // 1. Playlist Mode
    if (currentPlaylist_) {
        auto currentTrack = state_->getCurrentTrack();
        int currentIndex = findTrackIndexInPlaylist(currentTrack);
        
        if (currentIndex != -1) {
            auto tracks = currentPlaylist_->getTracks();
            size_t nextIndex = static_cast<size_t>(currentIndex) + 1;
            
            // Loop if enabled
            if (nextIndex >= tracks.size()) {
                if (currentPlaylist_->isLoopEnabled()) {
                    nextIndex = 0;
                } else {
                    return false; // End of playlist
                }
            }
            
            if (nextIndex < tracks.size()) {
                return play(tracks[nextIndex]);
            }
        } else if (!currentPlaylist_->getTracks().empty()) {
            // Start from beginning if current track not in playlist
            return play(currentPlaylist_->getTracks()[0]);
        }
    }
    
    // 2. Queue Mode (Fallback)
    auto nextTrack = state_->getNextTrack();
    if (nextTrack) {
        return play(nextTrack);
    }
    return false;
}

bool PlaybackController::previous() {
    // 1. Playlist Mode
    if (currentPlaylist_) {
        auto currentTrack = state_->getCurrentTrack();
        
        // If playing > 3 seconds, restart track
        if (state_->getPosition() > 3.0) {
            seek(0.0);
            return true;
        }
        
        int currentIndex = findTrackIndexInPlaylist(currentTrack);
        
        if (currentIndex != -1) {
            auto tracks = currentPlaylist_->getTracks();
            int prevIndex = currentIndex - 1;
            
            // Loop or stop
            if (prevIndex < 0) {
                if (currentPlaylist_->isLoopEnabled()) {
                    prevIndex = static_cast<int>(tracks.size()) - 1;
                } else {
                    prevIndex = 0; // Stay at first track or stop? Usually stay
                }
            }
            
            if (prevIndex >= 0 && static_cast<size_t>(prevIndex) < tracks.size()) {
                return play(tracks[static_cast<size_t>(prevIndex)]);
            }
        }
    }
    
    // 2. Library/History Mode (Back Stack)
    auto prevTrack = state_->popFromBackStack();
    if (prevTrack) {
        return play(prevTrack, false);
    }
    return false;
}

int PlaybackController::findTrackIndexInPlaylist(std::shared_ptr<MediaFile> track) {
    if (!currentPlaylist_ || !track) {
        return -1;
    }
    
    auto tracks = currentPlaylist_->getTracks();
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (tracks[i]->getPath() == track->getPath()) {
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

void PlaybackController::seek(double positionSeconds) {
    if (engine_) {
        engine_->seek(positionSeconds);
    }
    if (state_) {
        state_->setPosition(positionSeconds);
    }
}

void PlaybackController::setVolume(float volume) {
    if (state_) {
        state_->setVolume(volume);
    }
    if (engine_) {
        engine_->setVolume(volume);
    }
}

void PlaybackController::setCurrentPlaylist(Playlist* playlist) {
    currentPlaylist_ = playlist;
    if (!playlist) {
        return;
    }
    
    auto tracks = playlist->getTracks();
    state_->setPlayQueue(tracks);
}

void PlaybackController::playContext(const std::vector<std::shared_ptr<MediaFile>>& context, size_t startIndex) {
    if (context.empty() || startIndex >= context.size()) {
        return;
    }
    
    // Set queue
    state_->setPlayQueue(context);
    
    // Set index to the next track effectively (so valid next() works)
    // getNextTrack uses queueIndex_++, so if we are playing index i,
    // the next call to getNextTrack should return i+1.
    // So we should set queueIndex_ to startIndex + 1.
    state_->setQueueIndex(startIndex + 1);
    
    // Play the track
    play(context[startIndex]);
}

void PlaybackController::update(void* subject) {
    // Observer pattern - called when PlaybackState changes
    // Could update UI or respond to state changes here
}

void PlaybackController::updateTime(double deltaTime) {
    if (!state_ || !engine_) return;
    
    // Only update if playing
    if (state_->getStatus() == PlaybackStatus::PLAYING) {
        double currentPos = state_->getPosition();
        double duration = state_->getDuration();
        
        // Update position
        double newPos = currentPos + deltaTime;
        state_->setPosition(newPos);
        
        // Check for completion
        // Use engine status as primary authority if available, time as fallback
        if (engine_->isFinished()) {
            handlePlaybackFinished();
        } 
        // Fallback: if duration is known and we exceeded it significantly (e.g. +1 sec buffer)
        else if (duration > 0 && newPos > duration + 1.0) {
            handlePlaybackFinished();
        }
    }
}

void PlaybackController::handlePlaybackFinished() {
    Logger::getInstance().info("Playback finished");
    next(); // Simple auto-next
}
