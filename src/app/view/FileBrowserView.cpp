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

void FileBrowserView::show() {
    selectedFiles_.clear();
    BaseView::show();
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
    if (!isVisible()) return;

    // If in popup mode, we don't render as a window (handled by parent)
    if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN) return;
    
    // Set larger window size
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(700, 500), ImVec2(1400, 900));
    
    if (ImGui::Begin("File Browser", &visible_)) {
        renderContent();
    }
    ImGui::End();
}

void FileBrowserView::renderPopup() {
    // Popup logic
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Appearing);
    
    bool open = true;
    if (ImGui::BeginPopupModal("File Browser", &open)) {
        renderContent();
        
        ImGui::EndPopup();
    }
    
    if (!open) {
        visible_ = false;
    }
}

void FileBrowserView::renderContent() {
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
    
    // USB Devices - Placeholder for new functionality
    // renderUSBDevices(); 
    
    ImGui::Separator();
    
    // Get available content size
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float footerHeight = 40.0f; // Space for action buttons
    float contentHeight = availableSize.y - footerHeight;
    
    // Left panel width
    float leftPanelWidth = availableSize.x * 0.35f; // Original calculation
    
    // LEFT PANEL: Folders only
    if (ImGui::BeginChild("FolderPanel", ImVec2(leftPanelWidth, contentHeight), true)) {
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
        // renderDirectoryTree(); // Placeholder to get context for new functionality
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // RIGHT PANEL: Media files only (filtered)
    if (ImGui::BeginChild("FilePanel", ImVec2(0, contentHeight), true)) {
        ImGui::Text("Media Files");
        ImGui::Separator();
        
        // Table: [Select] [Name] [Extension]
        if (ImGui::BeginTable("FilesTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("##Select", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_NoHide); 
            ImGui::TableHeadersRow();
            
            int fileIndex = 0;
            for (size_t i = 0; i < currentMediaFiles_.size(); ++i) {
                const auto& fileInfo = currentMediaFiles_[i];
                // No need to check isDirectory as currentMediaFiles_ only has files
                
                std::string filename = fileInfo.name;
                std::string ext = fileInfo.extension;
                
                // Pure extension for display (remove dot)
                std::string displayExt = (ext.length() > 0) ? ext.substr(1) : "";
                
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

                ImGui::TableNextColumn();
                // 4. Path (Display relative path if possible, or full path)
                // If path starts with currentPath_, strip it to show relative
                std::string displayPath = fileInfo.path;
                // Simple check to make it relative to current view if desired, 
                // but user asked for "URL", so full path or at least clear path is good.
                // Let's force it to be slightly distinct color maybe?
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", displayPath.c_str());
                
                fileIndex++;
            }
            
             if (fileIndex == 0) {
                 // ImGui::TableNextRow(); ImGui::TableNextColumn();
                 // ImGui::TextDisabled("No media files");
             }
             
            ImGui::EndTable();
        }
        
        if (currentMediaFiles_.empty()) {
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
             // Iterate logic to add all supported files to playlist
             for (const auto& file : currentMediaFiles_) {
                 // No need to check directory or extension again, list is already filtered
                 playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, file.path);
             }
             Logger::getInstance().info("Added all files in folder to playlist");
        } else {
            libController_->addMediaFilesFromDirectory(currentPath_, false);
        }
    }
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
    // 1. Get directories for navigation (Left Panel)
    auto allFiles = fileSystem_->browse(currentPath_);
    currentFiles_.clear();
    for (const auto& file : allFiles) {
        if (file.isDirectory) {
            currentFiles_.push_back(file);
        }
    }
    
    // 2. Get recursive media files (Right Panel)
    currentMediaFiles_.clear();
    
    // Get all supported extensions
    std::vector<std::string> extensions = MediaFileFactory::getAllSupportedFormats();
    
    // Scan recursively
    std::vector<std::string> mediaPaths = fileSystem_->getMediaFiles(currentPath_, extensions);
    
    // Convert to FileInfo
    for (const auto& path : mediaPaths) {
        FileInfo info;
        info.path = path;
        
        // Extract filename and extension manually since we don't have fs::path here easily 
        // without including filesystem, but let's assume standard path format
        size_t lastSlash = path.find_last_of("/\\");
        info.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        size_t lastDot = info.name.find_last_of('.');
        info.extension = (lastDot != std::string::npos) ? info.name.substr(lastDot) : "";
        info.isDirectory = false;
        info.size = 0; // We don't query size for performance, or we could if needed
        
        currentMediaFiles_.push_back(info);
    }
    
    // selectedFiles_.clear(); // Removed to persist selection across navigation
}
