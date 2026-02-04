#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>

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
    
    // Current path
    ImGui::Text("Path: %s", currentPath_.c_str());
    
    if (ImGui::Button("Up")) {
        navigateUp();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Refresh")) {
        refreshCurrentDirectory();
    }
    
    ImGui::Separator();
    
    // File list
    if (ImGui::BeginChild("FileList", ImVec2(0, -30), true)) {
        for (size_t i = 0; i < currentFiles_.size(); ++i) {
            const auto& fileInfo = currentFiles_[i];
            bool isSelected = (static_cast<int>(i) == selectedIndex_);
            
            // Icon prefix
            std::string icon = fileInfo.isDirectory ? "[DIR] " : "[FILE] ";
            std::string displayText = icon + fileInfo.name;
            
            if (ImGui::Selectable(displayText.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex_ = static_cast<int>(i);
                
                // Double-click to navigate
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (fileInfo.isDirectory) {
                        navigateTo(fileInfo.path);
                    }
                }
            }
        }
    }
    ImGui::EndChild();
    
    // Bottom buttons
    if (ImGui::Button("Add Selected to Library")) {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(currentFiles_.size())) {
            const auto& selected = currentFiles_[selectedIndex_];
            if (!selected.isDirectory) {
                libController_->addMediaFile(selected.path);
            }
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Add Directory to Library")) {
        libController_->addMediaFilesFromDirectory(currentPath_, true);
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
