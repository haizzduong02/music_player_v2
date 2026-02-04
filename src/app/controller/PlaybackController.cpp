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
    auto current = state_->getCurrentTrack();
    if (current && engine_) {
        engine_->play(current->getPath());
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
