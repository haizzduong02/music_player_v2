#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "app/view/BaseView.h"
#include "app/view/LibraryView.h"
#include "app/view/PlaylistView.h"
#include "app/view/NowPlayingView.h"
#include "app/view/HistoryView.h"
#include "app/view/FileBrowserView.h"
#include "app/model/PlaybackState.h"
#include <vector>
#include <memory>
#include <GL/gl.h>

/**
 * @file MainWindow.h
 * @brief Main application window with unified layout
 * 
 * New unified layout with tab navigation, large album art,
 * and integrated playback controls.
 */

/**
 * @brief Screen enum for tab navigation
 */
enum class Screen {
    HISTORY,
    PLAYLIST,
    LIBRARY
};

/**
 * @brief Main window class with unified layout
 * 
 * Single window containing:
 * - Tab buttons (History, Playlist, Library) at top-left
 * - Album art/video display in center-left
 * - Track list on right side
 * - Playback controls at bottom
 */
class MainWindow : public BaseView {
public:
    MainWindow();
    ~MainWindow() override = default;
    
    void render() override;
    void handleInput() override;
    
    // View setters
    void setLibraryView(LibraryView* view) { libraryView_ = view; }
    void setPlaylistView(PlaylistView* view) { playlistView_ = view; }
    void setNowPlayingView(NowPlayingView* view) { nowPlayingView_ = view; }
    void setHistoryView(HistoryView* view) { historyView_ = view; }
    void setFileBrowserView(FileBrowserView* view) { fileBrowserView_ = view; }
    
    // Getters
    LibraryView* getLibraryView() const { return libraryView_; }
    FileBrowserView* getFileBrowserView() const { return fileBrowserView_; }
    PlaylistView* getPlaylistView() const { return playlistView_; }
    NowPlayingView* getNowPlayingView() const { return nowPlayingView_; }
    
    // Screen navigation
    void switchScreen(Screen screen);
    Screen getCurrentScreen() const { return currentScreen_; }
    
    // Set playback references for controls
    void setPlaybackController(PlaybackController* controller) { playbackController_ = controller; }
    void setPlaybackState(PlaybackState* state) { playbackState_ = state; }
    
private:
    // Views
    LibraryView* libraryView_ = nullptr;
    PlaylistView* playlistView_ = nullptr;
    NowPlayingView* nowPlayingView_ = nullptr;
    HistoryView* historyView_ = nullptr;
    FileBrowserView* fileBrowserView_ = nullptr;
    
    // State
    Screen currentScreen_ = Screen::LIBRARY;
    
    // Playback references
    PlaybackController* playbackController_ = nullptr;
    PlaybackState* playbackState_ = nullptr;
    
    // Album art texture
    GLuint albumArtTexture_ = 0;
    std::string currentTrackPath_;
    
    // Render helpers
    void renderTabBar();
    void renderAlbumArt();

    void renderPlaybackControls();
    void renderNowPlayingInfo();
    
    // Helper to scroll to currently playing track
    void scrollToCurrentTrack();
};

#endif // MAIN_WINDOW_H
