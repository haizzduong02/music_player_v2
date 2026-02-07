#ifndef IPLAYBACK_ENGINE_H
#define IPLAYBACK_ENGINE_H

#include "ISubject.h"
#include <string>

/**
 * @file IPlaybackEngine.h
 * @brief Interface for media playback engines (Dependency Inversion Principle)
 *
 * Defines the contract for playback functionality. Concrete implementations
 * (e.g., SDL2PlaybackEngine) will implement this interface.
 */

/**
 * @brief Playback status enumeration
 */
enum class PlaybackStatus
{
    STOPPED,
    PLAYING,
    PAUSED
};

/**
 * @brief Playback engine interface
 *
 * Extends ISubject to notify observers (Views, Controllers) of state changes.
 * This follows the Dependency Inversion Principle - high-level modules depend
 * on this abstraction, not on concrete SDL2 implementation.
 */
class IPlaybackEngine : public ISubject
{
  public:
    virtual ~IPlaybackEngine() = default;

    /**
     * @brief Load and play a media file
     * @param filepath Path to the media file
     * @return true if playback started successfully
     */
    virtual bool play(const std::string &filepath) = 0;

    /**
     * @brief Pause the current playback
     */
    virtual void pause() = 0;

    /**
     * @brief Resume playback if paused
     */
    virtual void resume() = 0;

    /**
     * @brief Stop playback and release resources
     */
    virtual void stop() = 0;

    /**
     * @brief Seek to a specific position
     * @param positionSeconds Position in seconds
     */
    virtual void seek(double positionSeconds) = 0;

    /**
     * @brief Set playback volume
     * @param volume Volume level (0.0 to 1.0)
     */
    virtual void setVolume(float volume) = 0;

    /**
     * @brief Get current playback state
     * @return Current state (STOPPED, PLAYING, PAUSED)
     */
    virtual PlaybackStatus getState() const = 0;

    /**
     * @brief Get current playback position
     * @return Position in seconds
     */
    virtual double getCurrentPosition() const = 0;

    /**
     * @brief Get total duration of current media
     * @return Duration in seconds
     */
    virtual double getDuration() const = 0;

    /**
     * @brief Get current volume level
     * @return Volume (0.0 to 1.0)
     */
    virtual float getVolume() const = 0;

    /**
     * @brief Check if playback has finished
     * @return true if playback completed
     */
    virtual bool isFinished() const = 0;

    /**
     * @brief Get the current video frame texture for rendering
     * @return OpenGL texture ID casting to void*, or nullptr if no video
     */
    virtual void *getVideoTexture()
    {
        return nullptr;
    }

    /**
     * @brief Get the dimensions of the current video frame
     * @param width Output width
     * @param height Output height
     */
    virtual void getVideoSize(int &width, int &height)
    {
        width = 0;
        height = 0;
    }

    /**
     * @brief Update video frame (called every frame on main thread)
     */
    virtual void updateVideo()
    {
    }
};

#endif // IPLAYBACK_ENGINE_H
