#ifndef FILE_BROWSER_VIEW_H
#define FILE_BROWSER_VIEW_H

#include "BaseView.h"
#include "../controller/LibraryController.h"
#include "../../interfaces/IFileSystem.h"
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
class FileBrowserView : public BaseView {
public:
    FileBrowserView(IFileSystem* fileSystem, LibraryController* libController);
    ~FileBrowserView() override = default;
    
    void render() override;
    void handleInput() override;
    
    void setCurrentDirectory(const std::string& path);
    const std::string& getCurrentDirectory() const { return currentPath_; }
    
private:
    IFileSystem* fileSystem_;
    LibraryController* libController_;
    
    // UI state
    std::string currentPath_;
    std::vector<FileInfo> currentFiles_;
    int selectedIndex_;
    std::vector<std::string> selectedFiles_;  // For multi-select
    
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
    void navigateUp();
    
    /**
     * @brief Navigate to directory
     * @param path Directory to navigate to
     */
    void navigateTo(const std::string& path);
    
    /**
     * @brief Refresh current directory
     */
    void refreshCurrentDirectory();
    
    /**
     * @brief Add selected files to library
     */
    void addSelectedToLibrary();
};

#endif // FILE_BROWSER_VIEW_H
