#include "../../../inc/app/view/ViewFactory.h"
#include "../../../inc/app/view/MainWindow.h"
#include "../../../inc/app/view/LibraryView.h"
#include "../../../inc/app/view/PlaylistView.h"
#include "../../../inc/app/view/NowPlayingView.h"
#include "../../../inc/app/view/HistoryView.h"
#include "../../../inc/app/view/FileBrowserView.h"

// ViewFactory implementation

IView* ViewFactory::createMainWindow() {
    return new MainWindow();
}

IView* ViewFactory::createLibraryView(
    LibraryController* controller,
    Library* library,
    PlaybackController* playbackController,
    class PlaylistManager* playlistManager) {
    return new LibraryView(controller, library, playbackController, playlistManager);
}

IView* ViewFactory::createPlaylistView(
    PlaylistController* controller,
    PlaylistManager* manager,
    PlaybackController* playbackController) {
    return new PlaylistView(controller, manager, playbackController);
}

IView* ViewFactory::createNowPlayingView(
    PlaybackController* controller,
    PlaybackState* state) {
    return new NowPlayingView(controller, state);
}

IView* ViewFactory::createHistoryView(
    HistoryController* controller,
    History* history,
    PlaybackController* playbackController,
    PlaylistManager* playlistManager) {
    return new HistoryView(controller, history, playbackController, playlistManager);
}

IView* ViewFactory::createFileBrowserView(
    IFileSystem* fileSystem,
    LibraryController* libController) {
    return new FileBrowserView(fileSystem, libController);
}
