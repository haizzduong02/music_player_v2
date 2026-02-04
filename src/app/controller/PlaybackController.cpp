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

bool PlaybackController::play(std::shared_ptr<MediaFile> track) {
    if (!track || !engine_ || !state_) {
        return false;
    }
    
    // Save current to back stack
    if (state_->getCurrentTrack()) {
        state_->pushToBackStack();
    }
    
    // Update state first
    state_->setCurrentTrack(track);
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
    auto nextTrack = state_->getNextTrack();
    if (nextTrack) {
        return play(nextTrack);
    }
    return false;
}

bool PlaybackController::previous() {
    auto prevTrack = state_->popFromBackStack();
    if (prevTrack) {
        return play(prevTrack);
    }
    return false;
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
