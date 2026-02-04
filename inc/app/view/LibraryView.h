#ifndef LIBRARY_VIEW_H
#define LIBRARY_VIEW_H

#include "BaseView.h"
#include "../controller/LibraryController.h"
#include "../model/Library.h"
#include <string>

class PlaybackController;
class FileBrowserView;

/**
 * @file LibraryView.h
 * @brief Library view using ImGui
 * 
 * Displays library contents in a table.
 * Observes Library model for automatic updates.
 */

/**
 * @brief Library view class
 * 
 * ImGui-based view for the library.
 * Automatically updates when library changes (Observer pattern).
 */
class LibraryView : public BaseView {
public:
    LibraryView(LibraryController* controller, Library* library, PlaybackController* playbackController);
    ~LibraryView() override;
    
    void render() override;
    void handleInput() override;
    void update(void* subject) override;
    
    void setFileBrowserView(FileBrowserView* browserView) { fileBrowserView_ = browserView; }
    
private:
    LibraryController* controller_;
    Library* library_;
    PlaybackController* playbackController_;
    FileBrowserView* fileBrowserView_ = nullptr;
    
    // UI state
    std::string searchQuery_;
    std::vector<std::shared_ptr<MediaFile>> displayedFiles_;
    int selectedIndex_;
    
    // Popup state for Add File/Directory
    bool showAddFilePopup_ = false;
    bool showAddDirPopup_ = false;
    char filePathBuffer_[512] = "";
    char dirPathBuffer_[512] = "";
    
    /**
     * @brief Render search bar
     */
    void renderSearchBar();
    
    /**
     * @brief Render library table
     */
    void renderLibraryTable();
    
    /**
     * @brief Render context menu for selected item
     */
    void renderContextMenu();
    
    /**
     * @brief Handle search
     */
    void performSearch();
    
    /**
     * @brief Refresh displayed files
     */
    void refreshDisplay();
};

#endif // LIBRARY_VIEW_H
