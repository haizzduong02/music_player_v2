#ifndef IVIEW_FACTORY_H
#define IVIEW_FACTORY_H

#include "IView.h"
#include <memory>

/**
 * @file IViewFactory.h
 * @brief Factory interface for creating views (Factory Method Pattern)
 * 
 * Defines the contract for view factories.
 * Allows dependency injection and testability.
 */

// Forward declarations
class LibraryController;
class PlaylistController;
class PlaybackController;
class HistoryController;
class Library;
class PlaylistManager;
class PlaybackState;
class History;
class IFileSystem;

/**
 * @brief View factory interface
 * 
 * Creates view instances with injected dependencies.
 * Implements Factory Method pattern.
 */
class IViewFactory {
public:
    virtual ~IViewFactory() = default;
    
    /**
     * @brief Create main window view
     * @return Pointer to main window
     */
    virtual IView* createMainWindow() = 0;
    
    /**
     * @brief Create library view
     * @param controller Library controller
     * @param library Library model to observe
     * @return Pointer to library view
     */
    virtual IView* createLibraryView(
        LibraryController* controller,
        Library* library,
        PlaybackController* playbackController,
        class PlaylistManager* playlistManager) = 0;
    
    /**
     * @brief Create playlist view
     * @param controller Playlist controller
     * @param manager Playlist manager to observe
     * @return Pointer to playlist view
     */
    virtual IView* createPlaylistView(
        PlaylistController* controller,
        PlaylistManager* manager,
        PlaybackController* playbackController) = 0;
    
    /**
     * @brief Create now playing view
     * @param controller Playback controller
     * @param state Playback state to observe
     * @return Pointer to now playing view
     */
    virtual IView* createNowPlayingView(
        PlaybackController* controller,
        PlaybackState* state) = 0;
    
    /**
     * @brief Create history view
     * @param controller History controller
     * @param history History model to observe
     * @return Pointer to history view
     */
    virtual IView* createHistoryView(
        HistoryController* controller,
        History* history,
        PlaybackController* playbackController) = 0;
    
    /**
     * @brief Create file browser view
     * @param fileSystem File system interface
     * @param libController Library controller
     * @return Pointer to file browser view
     */
    virtual IView* createFileBrowserView(
        IFileSystem* fileSystem,
        LibraryController* libController) = 0;
};

#endif // IVIEW_FACTORY_H
