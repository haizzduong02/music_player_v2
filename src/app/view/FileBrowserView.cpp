#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/app/model/MediaFileFactory.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>
#include <algorithm>

FileBrowserView::FileBrowserView(IFileSystem* fileSystem, LibraryController* libController)
    : fileSystem_(fileSystem), libController_(libController), selectedIndex_(-1) {
    
    // Start at current directory or home
    currentPath_ = fileSystem_->exists(".") ? "." : "/";
    refreshCurrentDirectory();
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
                
                // Double-click to add to library
                if (ImGui::IsMouseDoubleClicked(0)) {
                    libController_->addMediaFile(fileInfo.path);
                    Logger::getInstance().info("Added: " + fileInfo.name);
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
    if (ImGui::Button("Add Selected")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(currentFiles_.size())) {
            const auto& selected = currentFiles_[selectedIndex_];
            if (!selected.isDirectory) {
                libController_->addMediaFile(selected.path);
            }
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Add All in Folder")) {
        libController_->addMediaFilesFromDirectory(currentPath_, false);
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
