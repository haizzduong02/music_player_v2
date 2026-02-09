#include "app/controller/PlaybackController.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include <chrono>

PlaybackController::PlaybackController(IPlaybackEngine *engine, PlaybackState *state, History *history,
                                       IHardwareInterface *hardware, Playlist *currentPlaylist)
    : engine_(engine), state_(state), history_(history), hardware_(hardware), currentPlaylist_(currentPlaylist)
{
    if (hardware_)
    {
        hardware_->attach(this);
    }
}

PlaybackController::~PlaybackController()
{
    if (hardware_)
    {
        hardware_->detach(this);
    }
}

bool PlaybackController::play(std::shared_ptr<MediaFile> track, bool pushToStack)
{
    if (!track || !engine_ || !state_)
    {
        return false;
    }

    // Throttle playback requests for the same track within 500ms
    auto now = std::chrono::steady_clock::now();
    double currentTime = std::chrono::duration<double>(now.time_since_epoch()).count();

    if (track->getPath() == lastPlayedPath_ && (currentTime - lastPlayTime_ < 0.5))
    {
        Logger::debug("Throttling playback request for: " + track->getPath());
        return true; // Already playing or starting
    }

    lastPlayTime_ = currentTime;
    lastPlayedPath_ = track->getPath();

    // Save current to back stack only if requested
    if (pushToStack && state_->getCurrentTrack())
    {
        state_->pushToBackStack();
    }

    // Update state atomically
    state_->setPlayback(track, PlaybackStatus::PLAYING);
    state_->syncQueueIndex(track); // Sync queue index for correct Next behavior

    // Add to history
    if (history_)
    {
        history_->addTrack(track);
    }

    // Play via engine
    return engine_->play(track->getPath());
}

void PlaybackController::pause()
{
    if (engine_)
    {
        engine_->pause();
    }
    if (state_)
    {
        state_->setStatus(PlaybackStatus::PAUSED);
    }
}

void PlaybackController::resume()
{
    if (engine_)
    {
        engine_->resume();
    }
    if (state_)
    {
        state_->setStatus(PlaybackStatus::PLAYING);
    }
}

void PlaybackController::stop()
{
    if (engine_)
    {
        engine_->stop();
    }
    if (state_)
    {
        state_->setStatus(PlaybackStatus::STOPPED);
        state_->setPosition(0.0);
    }
}

bool PlaybackController::next()
{
    // 1. Playlist Mode
    if (currentPlaylist_)
    {
        auto currentTrack = state_->getCurrentTrack();
        int currentIndex = findTrackIndexInPlaylist(currentTrack);

        if (currentIndex != -1)
        {
            auto tracks = currentPlaylist_->getTracks();
            size_t nextIndex = static_cast<size_t>(currentIndex) + 1;

            // Loop if enabled
            if (nextIndex >= tracks.size())
            {
                if (currentPlaylist_->getRepeatMode() == RepeatMode::ALL)
                {
                    nextIndex = 0;
                }
                else
                {
                    return false; // End of playlist
                }
            }

            if (nextIndex < tracks.size())
            {
                return play(tracks[nextIndex]);
            }
        }
        else if (!currentPlaylist_->getTracks().empty())
        {
            // Start from beginning if current track not in playlist
            return play(currentPlaylist_->getTracks()[0]);
        }
    }

    // 2. Queue Mode (Fallback)
    auto nextTrack = state_->getNextTrack();

    // Check for Loop ALL in Queue mode
    if (!nextTrack && !currentPlaylist_ && globalRepeatMode_ == RepeatMode::ALL)
    {
        // Loop back to start of queue
        state_->setQueueIndex(0);
        nextTrack = state_->getNextTrack();
    }

    if (nextTrack)
    {
        return play(nextTrack);
    }
    return false;
}

bool PlaybackController::previous()
{
    // 1. Playlist Mode
    if (currentPlaylist_)
    {
        auto currentTrack = state_->getCurrentTrack();

        // If playing > 3 seconds, restart track
        if (state_->getPosition() > 3.0)
        {
            seek(0.0);
            return true;
        }

        int currentIndex = findTrackIndexInPlaylist(currentTrack);

        if (currentIndex != -1)
        {
            auto tracks = currentPlaylist_->getTracks();
            int prevIndex = currentIndex - 1;

            // Loop or stop
            if (prevIndex < 0)
            {
                if (currentPlaylist_->isLoopEnabled())
                {
                    prevIndex = static_cast<int>(tracks.size()) - 1;
                }
                else
                {
                    prevIndex = 0; // Stay at first track or stop? Usually stay
                }
            }

            if (prevIndex >= 0 && static_cast<size_t>(prevIndex) < tracks.size())
            {
                return play(tracks[static_cast<size_t>(prevIndex)]);
            }
        }
    }

    // 2. Library/History Mode (Back Stack)
    auto prevTrack = state_->popFromBackStack();
    if (prevTrack)
    {
        return play(prevTrack, false);
    }
    return false;
}

int PlaybackController::findTrackIndexInPlaylist(std::shared_ptr<MediaFile> track)
{
    if (!currentPlaylist_ || !track)
    {
        return -1;
    }

    auto tracks = currentPlaylist_->getTracks();
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        if (tracks[i]->getPath() == track->getPath())
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void PlaybackController::seek(double positionSeconds)
{
    if (engine_)
    {
        engine_->seek(positionSeconds);
    }
    if (state_)
    {
        state_->setPosition(positionSeconds);
    }
}

void PlaybackController::setVolume(float volume)
{
    if (state_)
    {
        state_->setVolume(volume);
    }
    if (engine_)
    {
        engine_->setVolume(volume);
    }

    // Persist volume change
    Config::getInstance().getConfig().customVolume = volume;
}

void PlaybackController::setCurrentPlaylist(Playlist *playlist)
{
    currentPlaylist_ = playlist;
    if (!playlist)
    {
        return;
    }

    auto tracks = playlist->getTracks();
    state_->setPlayQueue(tracks);
}

void PlaybackController::playContext(const std::vector<std::shared_ptr<MediaFile>> &context, size_t startIndex)
{
    if (context.empty() || startIndex >= context.size())
    {
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

void PlaybackController::update(void *subject)
{
    // Handle Hardware Events
    if (hardware_ && subject == hardware_)
    {
        auto *hw = static_cast<IHardwareInterface *>(subject);
        HardwareEvent event = hw->getLastEvent();
        
        Logger::info("Hardware Event Received: " + std::to_string((int)event.command));

        switch (event.command)
        {
        case HardwareCommand::NEXT:
            next();
            break;
        case HardwareCommand::PREVIOUS:
            previous();
            break;
        case HardwareCommand::PLAY:
            resume();
            break;
        case HardwareCommand::PAUSE:
            pause();
            break;
        case HardwareCommand::ADC_UPDATE:
             setVolume(event.value);
             break;
        case HardwareCommand::BUTTON_PRESS:
             // button 1: next, 2: prev, 3: play/pause
             if (event.value == 1.0f) next();
             else if (event.value == 2.0f) previous();
             else if (event.value == 3.0f) {
                 if (state_->getStatus() == PlaybackStatus::PLAYING) pause();
                 else resume();
             }
             break;
        default:
            break;
        }
        return;
    }

    // Observer pattern - called when PlaybackEngine changes state
    if (!engine_) return;

    if (engine_->getState() == PlaybackStatus::ERROR)
    {
        Logger::error("PlaybackController received ERROR state from engine");
        
        // Stop to clear state (and ensure we don't interfere with next track if callback triggers it)
        stop();

        // Report failure
        if (onTrackLoadFailedCallback_ && !lastPlayedPath_.empty())
        {
            Logger::error("Reporting track load failure: " + lastPlayedPath_);
            onTrackLoadFailedCallback_(lastPlayedPath_);
        }
    }
}

void PlaybackController::updateTime(double deltaTime)
{
    if (!state_ || !engine_)
        return;

    // Only update if playing
    if (state_->getStatus() == PlaybackStatus::PLAYING)
    {
        double currentPos = state_->getPosition();
        double duration = state_->getDuration();

        // Update position
        double newPos = currentPos + deltaTime;
        state_->setPosition(newPos);

        // Check for completion
        // Use engine status as primary authority if available, time as fallback
        if (engine_->isFinished())
        {
            handlePlaybackFinished();
        }
        // Fallback: if duration is known and we exceeded it significantly (e.g. +1 sec buffer)
        else if (duration > 0 && newPos > duration + 1.0)
        {
            handlePlaybackFinished();
        }
    }
}

void PlaybackController::handlePlaybackFinished()
{
    Logger::info("Playback finished");

    RepeatMode mode = currentPlaylist_ ? currentPlaylist_->getRepeatMode() : globalRepeatMode_;

    Logger::info("Current RepeatMode: " + std::to_string(static_cast<int>(mode)));

    // Check RepeatMode::ONE
    if (mode == RepeatMode::ONE)
    {
        if (state_ && state_->getCurrentTrack())
        {
            Logger::info("RepeatMode::ONE active, repeating track: " + state_->getCurrentTrack()->getDisplayName());
            play(state_->getCurrentTrack());
            return;
        }
    }

    Logger::info("Calling next() (Auto-advance)");
    next(); // Simple auto-next
}

void PlaybackController::toggleRepeatMode()
{
    // Determine current mode
    RepeatMode current = currentPlaylist_ ? currentPlaylist_->getRepeatMode() : globalRepeatMode_;
    RepeatMode nextMode = RepeatMode::NONE;

    // Cycle: NONE -> ONE -> ALL -> NONE
    switch (current)
    {
    case RepeatMode::NONE:
        nextMode = RepeatMode::ONE;
        break;
    case RepeatMode::ONE:
        nextMode = RepeatMode::ALL;
        break;
    case RepeatMode::ALL:
        nextMode = RepeatMode::NONE;
        break;
    }

    if (currentPlaylist_)
    {
        currentPlaylist_->setRepeatMode(nextMode);
    }
    else
    {
        globalRepeatMode_ = nextMode;
    }

    std::string modeStr = "NONE";
    if (nextMode == RepeatMode::ALL)
        modeStr = "ALL";
    else if (nextMode == RepeatMode::ONE)
        modeStr = "ONE";

    Logger::info("Repeat mode set to: " + modeStr);
}

void PlaybackController::setRepeatMode(RepeatMode mode)
{
    if (currentPlaylist_)
    {
        currentPlaylist_->setRepeatMode(mode);
    }
    else
    {
        globalRepeatMode_ = mode;
    }
}
