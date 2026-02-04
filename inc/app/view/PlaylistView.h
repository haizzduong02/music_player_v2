#ifndef PLAYLIST_VIEW_H
#define PLAYLIST_VIEW_H

#include "BaseView.h"
#include "../controller/PlaylistController.h"
#include "../controller/PlaybackController.h"
#include "../model/PlaylistManager.h"
#include <string>

class FileBrowserView;

/**
 * @file PlaylistView.h
 * @brief Playlist view using ImGui
 * 
 * Displays playlists and their contents.
 * Observes PlaylistManager for automatic updates.
 */

/**
 * @brief Playlist view class
 * 
 * ImGui-based view for playlists.
 * Shows list of playlists and selected playlist content.
 */
class PlaylistView : public BaseView {
public:
    PlaylistView(PlaylistController* controller, PlaylistManager* manager, PlaybackController* playbackController);
    ~PlaylistView() override;
    
    void render() override;
    void handleInput() override;
    void update(void* subject) override;
    
    void setFileBrowserView(FileBrowserView* browserView) { fileBrowserView_ = browserView; }
    
    PlaylistManager* getManager() const { return manager_; }
    
private:
    PlaylistController* controller_;
    PlaylistManager* manager_;
    PlaybackController* playbackController_;
    FileBrowserView* fileBrowserView_ = nullptr;
    
    // UI state
    std::string selectedPlaylistName_;
    std::shared_ptr<Playlist> selectedPlaylist_;
    int selectedTrackIndex_;
    std::string newPlaylistName_;
    bool showCreateDialog_;
    bool showRenameDialog_;
    std::string renameBuffer_;
    
    /**
     * @brief Render playlist list (left panel)
     */
    void renderPlaylistList();
    
    /**
     * @brief Render playlist content (right panel)
     */
    void renderPlaylistContent();
    
    /**
     * @brief Render playlist controls (shuffle, loop, etc.)
     */
    void renderPlaylistControls();
    
    /**
     * @brief Render create playlist dialog
     */
    void renderCreateDialog();
    
    /**
     * @brief Render rename playlist dialog
     */
    void renderRenameDialog();
    
    /**
     * @brief Render context menu
     */
    void renderContextMenu();
    
    /**
     * @brief Select a playlist
     * @param name Playlist name
     */
    void selectPlaylist(const std::string& name);
};

#endif // PLAYLIST_VIEW_H
