#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "BaseView.h"
#include "LibraryView.h"
#include "PlaylistView.h"
#include "NowPlayingView.h"
#include "HistoryView.h"
#include "FileBrowserView.h"
#include <vector>
#include <memory>

/**
 * @file MainWindow.h
 * @brief Main application window
 * 
 * Container for all child views.
 * Manages ImGui main window and menu bar.
 */

/**
 * @brief Main window class
 * 
 * Root view that contains all other views.
 * Manages window layout and navigation.
 */
class MainWindow : public BaseView {
public:
    /**
     * @brief Constructor
     */
    MainWindow();
    
    /**
     * @brief Destructor
     */
    ~MainWindow() override = default;
    
    /**
     * @brief Render main window and all child views
     */
    void render() override;
    
    /**
     * @brief Handle input
     */
    void handleInput() override;
    
    /**
     * @brief Add a child view
     * @param view View to add
     */
    void addView(IView* view);
    
    /**
     * @brief Remove a child view
     * @param view View to remove
     */
    void removeView(IView* view);
    
    /**
     * @brief Set library view
     * @param view Library view
     */
    void setLibraryView(LibraryView* view) { libraryView_ = view; }
    
    /**
     * @brief Set playlist view
     * @param view Playlist view
     */
    void setPlaylistView(PlaylistView* view) { playlistView_ = view; }
    
    /**
     * @brief Set now playing view
     * @param view Now playing view
     */
    void setNowPlayingView(NowPlayingView* view) { nowPlayingView_ = view; }
    
    /**
     * @brief Set history view
     * @param view History view
     */
    void setHistoryView(HistoryView* view) { historyView_ = view; }
    
    /**
     * @brief Set file browser view
     * @param view File browser view
     */
    void setFileBrowserView(FileBrowserView* view) { fileBrowserView_ = view; }
    
    /**
     * @brief Get library view
     * @return Library view pointer
     */
    LibraryView* getLibraryView() const { return libraryView_; }
    
    /**
     * @brief Get file browser view
     * @return File browser view pointer
     */
    FileBrowserView* getFileBrowserView() const { return fileBrowserView_; }
    
    /**
     * @brief Get playlist view
     * @return Playlist view pointer
     */
    PlaylistView* getPlaylistView() const { return playlistView_; }
    
private:
    std::vector<IView*> childViews_;
    
    // Main views (owned elsewhere, just referenced here)
    LibraryView* libraryView_;
    PlaylistView* playlistView_;
    NowPlayingView* nowPlayingView_;
    HistoryView* historyView_;
    FileBrowserView* fileBrowserView_;
    
    /**
     * @brief Render menu bar
     */
    void renderMenuBar();
    
    /**
     * @brief Render docking layout
     */
    void setupDockspace();
};

#endif // MAIN_WINDOW_H
