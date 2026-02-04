#ifndef NOW_PLAYING_VIEW_H
#define NOW_PLAYING_VIEW_H

#include "BaseView.h"
#include "../controller/PlaybackController.h"
#include "../model/PlaybackState.h"

/**
 * @file NowPlayingView.h
 * @brief Now playing view using ImGui
 * 
 * Displays currently playing track with controls.
 * Observes PlaybackState for automatic updates.
 */

/**
 * @brief Now playing view class
 * 
 * ImGui-based view for current playback.
 * Shows metadata, progress, and playback controls.
 */
class NowPlayingView : public BaseView {
public:
    NowPlayingView(PlaybackController* controller, PlaybackState* state);
    ~NowPlayingView() override;
    
    void render() override;
    void handleInput() override;
    void update(void* subject) override;
    
private:
    PlaybackController* controller_;
    PlaybackState* state_;
    
    // UI state
    float volumeSlider_;
    bool isDraggingSeek_;
    float seekPosition_;
    
    // Album art OpenGL texture
    unsigned int albumArtTexture_ = 0;
    std::string currentTrackPath_;  // To detect track changes
    
    /**
     * @brief Render track metadata
     */
    void renderMetadata();
    
    /**
     * @brief Render album art (placeholder or actual)
     */
    void renderAlbumArt();
    
    /**
     * @brief Render progress bar
     */
    void renderProgressBar();
    
    /**
     * @brief Render playback controls (play/pause/next/prev)
     */
    void renderPlaybackControls();
    
    /**
     * @brief Render volume control
     */
    void renderVolumeControl();
    
    /**
     * @brief Format time for display
     * @param seconds Time in seconds
     * @return Formatted string (MM:SS)
     */
    std::string formatTime(double seconds) const;
};

#endif // NOW_PLAYING_VIEW_H
