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
    PlaybackController* playbackController) {
    return new LibraryView(controller, library, playbackController);
}

IView* ViewFactory::createPlaylistView(
    PlaylistController* controller,
    PlaylistManager* manager) {
    return new PlaylistView(controller, manager);
}

IView* ViewFactory::createNowPlayingView(
    PlaybackController* controller,
    PlaybackState* state) {
    return new NowPlayingView(controller, state);
}

IView* ViewFactory::createHistoryView(
    HistoryController* controller,
    History* history) {
    return new HistoryView(controller, history);
}

IView* ViewFactory::createFileBrowserView(
    IFileSystem* fileSystem,
    LibraryController* libController) {
    return new FileBrowserView(fileSystem, libController);
}
