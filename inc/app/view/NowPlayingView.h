#ifndef NOW_PLAYING_VIEW_H
#define NOW_PLAYING_VIEW_H

#include "app/controller/PlaybackController.h"
#include "app/model/PlaylistManager.h"
#include "app/view/BaseView.h"

// ... (existing comments)

class NowPlayingView : public BaseView
{
    friend class NowPlayingViewTest;

  public:
    NowPlayingView(PlaybackController *controller, PlaybackState *state);
    ~NowPlayingView() override;

    void setPlaylistManager(PlaylistManager *manager)
    {
        playlistManager_ = manager;
    }

    void render() override;
    void handleInput() override;
    void update(void *subject) override;

  private:
    PlaybackController *controller_;
    PlaybackState *state_;
    PlaylistManager *playlistManager_ = nullptr;

    // UI state
    float volumeSlider_;
    bool isDraggingSeek_;
    float seekPosition_;

    // Icons
    unsigned int playTexture_ = 0;
    unsigned int pauseTexture_ = 0;
    unsigned int nextTexture_ = 0;
    unsigned int prevTexture_ = 0;
    unsigned int heartFilledTexture_ = 0;
    unsigned int heartOutlineTexture_ = 0;

    // Album art OpenGL texture
    unsigned int albumArtTexture_ = 0;
    std::string currentTrackPath_; // To detect track changes

    /**
     * @brief Load a texture from file
     */
    unsigned int loadIconTexture(const std::string &path);

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
