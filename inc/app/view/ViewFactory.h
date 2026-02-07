#ifndef VIEW_FACTORY_H
#define VIEW_FACTORY_H

#include "interfaces/IViewFactory.h"
#include "app/view/MainWindow.h"
#include "app/view/LibraryView.h"
#include "app/view/PlaylistView.h"
#include "app/view/NowPlayingView.h"
#include "app/view/HistoryView.h"
#include "app/view/FileBrowserView.h"

/**
 * @file ViewFactory.h
 * @brief Concrete implementation of IViewFactory (Factory Method Pattern)
 * 
 * Creates view instances with proper dependency injection.
 */

/**
 * @brief View factory class
 * 
 * Implements Factory Method pattern to create ImGui views.
 * Handles dependency injection for all views.
 */
class ViewFactory : public IViewFactory {
public:
    /**
     * @brief Constructor
     */
    ViewFactory() = default;
    
    /**
     * @brief Destructor
     */
    ~ViewFactory() override = default;
    
    /**
     * @brief Create main window view
     * @return Pointer to main window
     */
    IView* createMainWindow() override;
    
    /**
     * @brief Create library view
     * @param controller Library controller
     * @param library Library model to observe
     * @return Pointer to library view
     */
    IView* createLibraryView(
        LibraryController* controller,
        Library* library,
        PlaybackController* playbackController,
        class PlaylistManager* playlistManager) override;
    
    /**
     * @brief Create playlist view
     * @param controller Playlist controller
     * @param manager Playlist manager to observe
     * @return Pointer to playlist view
     */
    IView* createPlaylistView(
        PlaylistController* controller,
        PlaylistManager* manager,
        PlaybackController* playbackController) override;
    
    /**
     * @brief Create now playing view
     * @param controller Playback controller
     * @param state Playback state to observe
     * @return Pointer to now playing view
     */
    IView* createNowPlayingView(
        PlaybackController* controller,
        PlaybackState* state) override;
    
    /**
     * @brief Create history view
     * @param controller History controller
     * @param history History model to observe
     * @return Pointer to history view
     */
    IView* createHistoryView(
        HistoryController* controller,
        History* history,
        PlaybackController* playbackController,
        PlaylistManager* playlistManager) override;
    
    /**
     * @brief Create file browser view
     * @param fileSystem File system interface
     * @param libController Library controller
     * @return Pointer to file browser view
     */
    IView* createFileBrowserView(
        IFileSystem* fileSystem,
        LibraryController* libController) override;
};

#endif // VIEW_FACTORY_H
