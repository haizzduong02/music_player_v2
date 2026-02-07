#ifndef PLAYBACK_CONTROLLER_H
#define PLAYBACK_CONTROLLER_H

#include "app/model/History.h"
#include "app/model/PlaybackState.h"
#include "app/model/Playlist.h"
#include "interfaces/IHardwareInterface.h"
#include "interfaces/IObserver.h"
#include "interfaces/IPlaybackEngine.h"
#include <memory>

/**
 * @file PlaybackController.h
 * @brief Controller for playback operations
 *
 * Handles playback logic and coordinates between PlaybackEngine,
 * PlaybackState, History, and Playlist.
 * Observes hardware events for hardware control integration.
 * Sends metadata to hardware for LCD display.
 */

/**
 * @brief Playback controller class
 *
 * Orchestrates playback operations.
 * Implements IObserver to receive hardware commands.
 * Manages playback state, history, and playlist progression.
 */
class PlaybackController : public IObserver
{
  public:
    /**
     * @brief Constructor with dependency injection
     * @param engine Playback engine interface
     * @param state Playback state model
     * @param history History model
     * @param hardware Hardware interface (can be nullptr if no hardware)
     * @param currentPlaylist Current playlist (can be nullptr)
     */
    PlaybackController(IPlaybackEngine *engine, PlaybackState *state, History *history,
                       IHardwareInterface *hardware = nullptr, Playlist *currentPlaylist = nullptr);

    /**
     * @brief Play a specific track
     * @param track Track to play
     * @return true if playback started
     */
    bool play(std::shared_ptr<MediaFile> track, bool pushToStack = true);

    /**
     * @brief Pause playback
     */
    void pause();

    /**
     * @brief Resume playback
     */
    void resume();

    /**
     * @brief Stop playback
     */
    void stop();

    /**
     * @brief Play next track in playlist
     * @return true if next track played
     */
    bool next();

    /**
     * @brief Play previous track (from back stack)
     * @return true if previous track played
     */
    bool previous();

    /**
     * @brief Seek to position
     * @param positionSeconds Position in seconds
     */
    void seek(double positionSeconds);

    /**
     * @brief Set volume
     * @param volume Volume level (0.0 to 1.0)
     */
    void setVolume(float volume);

    /**
     * @brief Set current playlist for Next/Previous navigation
     * @param playlist Playlist to use
     */
    void setCurrentPlaylist(Playlist *playlist);

    /**
     * @brief Get current playlist
     * @return Current playlist, nullptr if none
     */
    Playlist *getCurrentPlaylist() const
    {
        return currentPlaylist_;
    }

    /**
     * @brief Get playback state
     * @return Current playback state
     */
    PlaybackState *getPlaybackState() const
    {
        return state_;
    }

    /**
     * @brief Toggle repeat mode (NONE -> ALL -> ONE -> NONE)
     */
    void toggleRepeatMode();

    /**
     * @brief Set repeat mode directly
     * @param mode Repeat mode to set
     */
    void setRepeatMode(RepeatMode mode);

    /**
     * @brief Get current repeat mode (Playlist or Global)
     * @return Current RepeatMode
     */
    RepeatMode getRepeatMode() const
    {
        if (currentPlaylist_)
        {
            return currentPlaylist_->getRepeatMode();
        }
        return globalRepeatMode_;
    }

    /**
     * @brief Play a track within a context (queue)
     * @param context List of tracks (queue)
     * @param startIndex Index of track to start playing
     */
    void playContext(const std::vector<std::shared_ptr<MediaFile>> &context, size_t startIndex);

    /**
     * @brief Update from observed subject (Observer pattern)
     * Handles hardware events
     * @param subject Subject that changed
     */
    void update(void *subject) override;

    /**
     * @brief Check if playback has finished (for auto-next)
     * @return true if finished
     */
    bool hasFinished() const;

    /**
     * @brief Handle playback finished event
     * Automatically plays next track or stops based on playlist/loop
     */
    void handlePlaybackFinished();

    /**
     * @brief Update playback time
     * Called from main loop with delta time
     * @param deltaTime Time elapsed since last frame in seconds
     */
    void updateTime(double deltaTime);

    /**
     * @brief Get the underlying playback engine
     * @return Pointer to engine interface
     */
    IPlaybackEngine *getEngine() const
    {
        return engine_;
    }

  private:
    IPlaybackEngine *engine_;
    PlaybackState *state_;
    History *history_;
    IHardwareInterface *hardware_; // For writing LCD display, metadata
    Playlist *currentPlaylist_;
    RepeatMode globalRepeatMode_ = RepeatMode::NONE; // Fallback for Queue/Library mode
    size_t currentTrackIndex_;

    // Throttle state
    double lastPlayTime_ = 0.0;
    std::string lastPlayedPath_;

    /**
     * @brief Add track to history
     * @param track Track to add
     */
    void addToHistory(std::shared_ptr<MediaFile> track);

    /**
     * @brief Find track index in current playlist
     * @param track Track to find
     * @return Index, or -1 if not found
     */
    int findTrackIndexInPlaylist(std::shared_ptr<MediaFile> track);

    /**
     * @brief Send track metadata to hardware LCD
     * @param track Track to display
     */
    void sendMetadataToHardware(std::shared_ptr<MediaFile> track);
};

#endif // PLAYBACK_CONTROLLER_H
