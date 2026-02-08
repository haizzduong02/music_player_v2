#ifndef FILE_BROWSER_VIEW_H
#define FILE_BROWSER_VIEW_H

#include "app/controller/LibraryController.h"
#include "app/view/BaseView.h"
#include "app/view/components/PagedFileSelector.h"
#include "interfaces/IFileSystem.h"
#include <functional>
#include <set>
#include <string>
#include <vector>

/**
 * @file FileBrowserView.h
 * @brief File browser view using ImGui
 *
 * Allows browsing file system and USB devices.
 * Supports adding files to library and playlists.
 */

/**
 * @brief File browser view class
 *
 * ImGui-based file system browser.
 * Shows directory tree and file list.
 */
class FileBrowserView : public BaseView
{
    friend class FileBrowserViewTest;

  public:
    FileBrowserView(IFileSystem *fileSystem, LibraryController *libController);
    ~FileBrowserView() override = default;

    void render() override;
    void renderPopup(); // Render as a popup/modal
    void handleInput() override;
    void show() override; // Override to clear selection on open

    void setCurrentDirectory(const std::string &path);
    const std::string &getCurrentDirectory() const
    {
        return currentPath_;
    }

    /**
     * @brief Browser operation mode
     */
    enum class BrowserMode
    {
        LIBRARY,               // Default: managing library
        PLAYLIST_SELECTION,    // Selecting files for a playlist
        LIBRARY_ADD_AND_RETURN // Add to library and return selected files (callback)
    };

    void setPlaylistController(class PlaylistController *controller); // Forward decl in cpp or include
    void setMode(BrowserMode mode);
    void setTargetPlaylist(const std::string &playlistName);

    // Callback for when files are added in LIBRARY_ADD_AND_RETURN mode
    void setOnFilesAddedCallback(std::function<void(const std::vector<std::string> &)> callback)
    {
        onFilesAddedCallback_ = callback;
    }

  protected:
    PagedFileSelector fileSelector_;
    int currentTrackCount_ = 0;
    std::string currentPath_;

    void navigateTo(const std::string &path);
    void navigateUp();
    void processFiles(const std::vector<std::string> &paths);

  private:
    IFileSystem *fileSystem_;
    LibraryController *libController_;
    class PlaylistController *playlistController_ = nullptr; // Forward declaration

    // UI state

    // ...

    // UI state
    std::vector<FileInfo> currentFiles_; // Folders for navigation
    // std::vector<FileInfo> currentMediaFiles_; // Recursive media files - Moved to PagedFileSelector
    // std::set<std::string> selectedFiles_;  // For multi-select - Moved to PagedFileSelector


    // Mode state
    BrowserMode mode_ = BrowserMode::LIBRARY;
    std::string targetPlaylistName_;
    std::function<void(const std::vector<std::string> &)> onFilesAddedCallback_;

    /**
     * @brief Render actual content (shared between Window and Popup)
     */
    void renderContent();

    /**
     * @brief Render directory tree (left panel)
     */
    void renderDirectoryTree();

    /**
     * @brief Render file list (right panel)
     */
    void renderFileList();

    /**
     * @brief Render navigation bar (breadcrumbs)
     */
    void renderNavigationBar();

    /**
     * @brief Render action buttons (add to library, add to playlist)
     */
    void renderActionButtons();

    /**
     * @brief Render USB devices section
     */
    void renderUSBDevices();

    /**
     * @brief Navigate to parent directory
     */
    void refreshCurrentDirectory();

    /**
     * @brief Add selected files to library
     */
    void addSelectedToLibrary();

    // Pagination state - Moved to PagedFileSelector
    // int currentPage_ = 0;
    // int itemsPerPage_ = 15;
    // int totalPages_ = 1;
    // char pageInputBuffer_[16] = "1";
};

#endif // FILE_BROWSER_VIEW_H
