#ifndef PLAYBACK_STATE_H
#define PLAYBACK_STATE_H

#include "app/model/MediaFile.h"
#include "utils/Subject.h"
#include "interfaces/IPlaybackEngine.h"
#include <memory>
#include <stack>

/**
 * @file PlaybackState.h
 * @brief Playback state model
 * 
 * Maintains the current playback state and notifies observers of changes.
 * Acts as a state machine for playback.
 */

/**
 * @brief Playback state class
 * 
 * Tracks current playback status including:
 * - Current track
 * - Play/Pause/Stop state
 * - Volume
 * - Position/Duration
 * - Previous track stack
 * 
 * Notifies observers when any state changes.
 */
class PlaybackState : public Subject {
public:
    /**
     * @brief Constructor
     */
    PlaybackState();
    
    /**
     * @brief Set current track
     * @param track Current track
     */
    void setPlayback(std::shared_ptr<MediaFile> track, PlaybackStatus status);
    
    std::shared_ptr<MediaFile> getCurrentTrack() const { return currentTrack_; }
    
    /**
     * @brief Set playback status
     * @param status New status
     */
    void setStatus(PlaybackStatus status);
    
    PlaybackStatus getStatus() const { return status_; }
    
    /**
     * @brief Set volume
     * @param volume Volume level (0.0 to 1.0)
     */
    void setVolume(float volume);
    
    float getVolume() const { return volume_; }
    
    /**
     * @brief Set current position
     * @param position Position in seconds
     */
    void setPosition(double position);
    
    double getPosition() const { return position_; }
    
    /**
     * @brief Set duration
     * @param duration Duration in seconds
     */
    void setDuration(double duration);
    
    /**
     * @brief Get duration
     * @return Duration in seconds
     */
    double getDuration() const {
        return duration_;
    }
    
    bool isPlaying() const { return status_ == PlaybackStatus::PLAYING; }
    bool isPaused() const { return status_ == PlaybackStatus::PAUSED; }
    bool isStopped() const { return status_ == PlaybackStatus::STOPPED; }
    
    /**
     * @brief Push current track to back stack (for Previous functionality)
     */
    void pushToBackStack();
    
    /**
     * @brief Pop from back stack
     * @return Previous track, nullptr if stack empty
     */
    std::shared_ptr<MediaFile> popFromBackStack();
    
    bool isBackStackEmpty() const { return backStack_.empty(); }
    
    /**
     * @brief Clear back stack
     */
    void clearBackStack();

    /**
     * @brief Remove specific track from back stack
     * @param path Filepath to remove
     */
    void removeTrackFromBackStack(const std::string& path);
    
    /**
     * @brief Set play queue for Next functionality
     * @param queue Queue of tracks to play
     */
    void setPlayQueue(const std::vector<std::shared_ptr<MediaFile>>& queue);
    
    /**
     * @brief Set current queue index
     * @param index New index
     */
    void setQueueIndex(size_t index);

    /**
     * @brief Sync queue index with the given track
     * Find the track in the play queue and set queue index to the next one
     * @param track Track to sync with
     */
    void syncQueueIndex(std::shared_ptr<MediaFile> track);
    
    /**
     * @brief Get next track from queue
     * @return Next track, nullptr if queue empty
     */
    std::shared_ptr<MediaFile> getNextTrack();
    
    /**
     * @brief Check if has next track
     * @return true if queue has more tracks
     */
    bool hasNextTrack() const;
    
    /**
     * @brief Clear play queue
     */
    void clearPlayQueue();
    
    /**
     * @brief Reset state to initial values
     */
    void reset();
    
private:
    std::shared_ptr<MediaFile> currentTrack_;
    PlaybackStatus status_;  // Changed from PlaybackState to avoid name collision
    float volume_;
    double position_;
    double duration_;
    std::stack<std::shared_ptr<MediaFile>> backStack_;  // For Previous functionality
    std::vector<std::shared_ptr<MediaFile>> playQueue_;  // For Next functionality
    size_t queueIndex_;  // Current position in play queue
    mutable std::mutex dataMutex_;  ///< Thread-safety for state access
};

#endif // PLAYBACK_STATE_H
