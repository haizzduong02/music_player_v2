#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/app/model/MediaFileFactory.h"
#include "../../../inc/app/controller/PlaylistController.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>
#include <algorithm>

FileBrowserView::FileBrowserView(IFileSystem* fileSystem, LibraryController* libController)
    : fileSystem_(fileSystem), libController_(libController), selectedIndex_(-1) {
    
    // Start at current directory or home
    currentPath_ = fileSystem_->exists(".") ? "." : "/";
    refreshCurrentDirectory();
}

void FileBrowserView::setPlaylistController(PlaylistController* controller) {
    playlistController_ = controller;
}

void FileBrowserView::setMode(BrowserMode mode) {
    mode_ = mode;
}

void FileBrowserView::setTargetPlaylist(const std::string& playlistName) {
    targetPlaylistName_ = playlistName;
}

void FileBrowserView::render() {
    if (!isVisible()) {
        return;
    }
    
    ImGui::Begin("File Browser", &visible_);
    
    // Current path and navigation
    ImGui::Text("Path: %s", currentPath_.c_str());
    
    if (ImGui::Button("Up")) {
        navigateUp();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Refresh")) {
        refreshCurrentDirectory();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Home")) {
        navigateTo("/home");
    }
    
    ImGui::Separator();
    
    // Get available content size
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float panelHeight = availableSize.y - 35; // Leave space for buttons
    float leftPanelWidth = availableSize.x * 0.35f;
    
    // LEFT PANEL: Folders only
    if (ImGui::BeginChild("FolderPanel", ImVec2(leftPanelWidth, panelHeight), true)) {
        ImGui::Text("Folders");
        ImGui::Separator();
        
        for (size_t i = 0; i < currentFiles_.size(); ++i) {
            const auto& fileInfo = currentFiles_[i];
            if (!fileInfo.isDirectory) continue;
            
            if (ImGui::Selectable(fileInfo.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    navigateTo(fileInfo.path);
                }
            }
        }
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // RIGHT PANEL: Media files only (filtered)
    if (ImGui::BeginChild("FilePanel", ImVec2(0, panelHeight), true)) {
        ImGui::Text("Media Files");
        ImGui::Separator();
        
        int fileIndex = 0;
        for (size_t i = 0; i < currentFiles_.size(); ++i) {
            const auto& fileInfo = currentFiles_[i];
            if (fileInfo.isDirectory) continue;
            
            // Check if supported format
            std::string ext = fileInfo.name;
            size_t dotPos = ext.find_last_of('.');
            if (dotPos == std::string::npos) continue;
            ext = ext.substr(dotPos);
            if (!MediaFileFactory::isSupportedFormat(ext)) continue;
            
            bool isSelected = (static_cast<int>(i) == selectedIndex_);
            
            if (ImGui::Selectable(fileInfo.name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex_ = static_cast<int>(i);
                
                // Double-click to add to library or playlist
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (mode_ == BrowserMode::PLAYLIST_SELECTION && playlistController_ && !targetPlaylistName_.empty()) {
                        // Add to playlist
                        playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, fileInfo.path);
                        Logger::getInstance().info("Added to playlist '" + targetPlaylistName_ + "': " + fileInfo.name);
                    } else {
                        // Default: Add to library
                        libController_->addMediaFile(fileInfo.path);
                        Logger::getInstance().info("Added to Library: " + fileInfo.name);
                    }
                }
            }
            fileIndex++;
        }
        
        if (fileIndex == 0) {
            ImGui::TextDisabled("No media files in this folder");
        }
    }
    ImGui::EndChild();
    
    // Bottom buttons
    std::string addBtnText = "Add Selected";
    if (mode_ == BrowserMode::PLAYLIST_SELECTION) {
        addBtnText = "Add to Playlist";
    } else {
        addBtnText = "Add to Library";
    }
    
    if (ImGui::Button(addBtnText.c_str())) {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(currentFiles_.size())) {
            const auto& selected = currentFiles_[selectedIndex_];
            if (!selected.isDirectory) {
                if (mode_ == BrowserMode::PLAYLIST_SELECTION && playlistController_ && !targetPlaylistName_.empty()) {
                    playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, selected.path);
                    Logger::getInstance().info("Added to playlist '" + targetPlaylistName_ + "': " + selected.name);
                } else {
                    libController_->addMediaFile(selected.path);
                    Logger::getInstance().info("Added to Library: " + selected.name);
                }
            }
        }
    }
    
    ImGui::SameLine();
    
    // Only show "Add All" if managing library, or implement "Add All to Playlist" similarly
    if (ImGui::Button("Add All in Folder")) {
        if (mode_ == BrowserMode::PLAYLIST_SELECTION && playlistController_ && !targetPlaylistName_.empty()) {
             // Iterate logic to add all supported files to playlist
             for (const auto& file : currentFiles_) {
                 if (!file.isDirectory) {
                     // Check support again just in case
                     std::string ext = file.name;
                     size_t dotPos = ext.find_last_of('.');
                     if (dotPos != std::string::npos && MediaFileFactory::isSupportedFormat(ext.substr(dotPos))) {
                         playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, file.path);
                     }
                 }
             }
             Logger::getInstance().info("Added all files in folder to playlist");
        } else {
            libController_->addMediaFilesFromDirectory(currentPath_, false);
        }
    }
    
    ImGui::End();
}

void FileBrowserView::handleInput() {
    // Handled through ImGui
}

void FileBrowserView::setCurrentDirectory(const std::string& path) {
    navigateTo(path);
}

void FileBrowserView::navigateUp() {
    // Get parent directory
    size_t lastSlash = currentPath_.find_last_of("/\\");
    if (lastSlash != std::string::npos && lastSlash > 0) {
        currentPath_ = currentPath_.substr(0, lastSlash);
        refreshCurrentDirectory();
    }
}

void FileBrowserView::navigateTo(const std::string& path) {
    if (fileSystem_->exists(path) && fileSystem_->isDirectory(path)) {
        currentPath_ = path;
        refreshCurrentDirectory();
    } else {
        Logger::getInstance().warn("Invalid directory: " + path);
    }
}

void FileBrowserView::refreshCurrentDirectory() {
    currentFiles_ = fileSystem_->browse(currentPath_);
    selectedIndex_ = -1;
}
