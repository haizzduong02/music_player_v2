#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/app/model/MediaFileFactory.h"
#include "../../../inc/app/controller/PlaylistController.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>
#include <algorithm>

FileBrowserView::FileBrowserView(IFileSystem* fileSystem, LibraryController* libController)
    : fileSystem_(fileSystem), libController_(libController) {
    
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
    
    // Set larger window size
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(700, 500), ImVec2(1400, 900));
    
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
        
        // Table: [Select] [Name] [Extension]
        if (ImGui::BeginTable("FilesTable", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("##Select", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableHeadersRow();
            
            int fileIndex = 0;
            for (size_t i = 0; i < currentFiles_.size(); ++i) {
                const auto& fileInfo = currentFiles_[i];
                if (fileInfo.isDirectory) continue;
                
                // Check if supported format
                std::string filename = fileInfo.name;
                std::string ext = "";
                size_t dotPos = filename.find_last_of('.');
                if (dotPos == std::string::npos) continue;
                ext = filename.substr(dotPos); // keep dot for check
                
                if (!MediaFileFactory::isSupportedFormat(ext)) continue;
                
                // Pure extension for display (remove dot)
                std::string displayExt = (ext.length() > 0) ? ext.substr(1) : "";
                // Name without extension (optional, user requested fields: Name, Extension)
                // Let's keep full name in Name col or strip it? usually Name includes ext.
                // User asked for "file ... fields: name, extension". 
                // I will show name (full) and extension in separate col.
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                // 1. Checkbox
                bool isSelected = (selectedFiles_.find(fileInfo.path) != selectedFiles_.end());
                ImGui::PushID((int)i);
                if (ImGui::Checkbox("##check", &isSelected)) {
                    if (isSelected) {
                        selectedFiles_.insert(fileInfo.path);
                    } else {
                        selectedFiles_.erase(fileInfo.path);
                    }
                }
                ImGui::PopID();
                
                ImGui::TableNextColumn();
                // 2. Name
                ImGui::Text("%s", fileInfo.name.c_str());
                
                ImGui::TableNextColumn();
                // 3. Extension
                ImGui::Text("%s", displayExt.c_str());
                
                fileIndex++;
            }
            
             if (fileIndex == 0) {
                 // ImGui::TableNextRow(); ImGui::TableNextColumn();
                 // ImGui::TextDisabled("No media files");
             }
             
            ImGui::EndTable();
        }
        
        if (currentFiles_.empty()) {
            ImGui::TextDisabled("Empty folder");
        }
    }
    ImGui::EndChild();
    
    // Bottom buttons
    std::string addBtnText = "Add Selected";
    if (mode_ == BrowserMode::PLAYLIST_SELECTION) {
        addBtnText = "Add to Playlist";
    } else if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN) {
        addBtnText = "Add & Return";
    } else {
        addBtnText = "Add to Library";
    }
    
    if (ImGui::Button(addBtnText.c_str())) {
        // Add all selected files
        int addedCount = 0;
        std::vector<std::string> addedPaths; 
        
        for (const auto& path : selectedFiles_) {
             if (mode_ == BrowserMode::PLAYLIST_SELECTION && playlistController_ && !targetPlaylistName_.empty()) {
                 playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, path);
             } else {
                 // Add to library (both for LIBRARY and LIBRARY_ADD_AND_RETURN)
                 libController_->addMediaFile(path);
                 addedPaths.push_back(path);
             }
             addedCount++;
        }
        
        if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN) {
            if (onFilesAddedCallback_) {
                onFilesAddedCallback_(addedPaths);
            }
            // Hide after adding
            visible_ = false;
        }
        
        if (addedCount > 0) {
            Logger::getInstance().info("Added " + std::to_string(addedCount) + " files.");
            // Optional: Clear selection after add?
            // selectedFiles_.clear(); 
        } else {
            Logger::getInstance().warn("No files selected.");
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
    selectedFiles_.clear();
}
