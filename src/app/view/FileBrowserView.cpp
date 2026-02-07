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
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(800, 600), ImVec2(1600, 1000));
    
    if (ImGui::Begin("File Browser", &visible_)) {
        renderContent();
    }
    ImGui::End();
}

void FileBrowserView::renderPopup() {
    // Popup logic
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_Appearing);
    
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
    // Current path info at the very top (optional but good for context)
    ImGui::TextDisabled("Location: %s", currentPath_.c_str());
    ImGui::Separator();
    
    // Get available content size
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float footerHeight = 10.0f; // Minimal footer now
    float contentHeight = availableSize.y - footerHeight;
    
    // Left panel width
    float leftPanelWidth = availableSize.x * 0.30f; 
    
    // LEFT PANEL: Folders only + Navigation
    if (ImGui::BeginChild("FolderPanel", ImVec2(leftPanelWidth, contentHeight), true)) {
        // Navigation buttons at the top of the tree
        float navBtnWidth = (ImGui::GetContentRegionAvail().x - 10.0f) / 3.0f;
        
        if (ImGui::Button("Up", ImVec2(navBtnWidth, 0))) navigateUp();
        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(navBtnWidth, 0))) refreshCurrentDirectory();
        ImGui::SameLine();
        
        const char* homeEnv = std::getenv("HOME");
        std::string homePath = homeEnv ? std::string(homeEnv) : "/home";
        if (ImGui::Button("Home", ImVec2(navBtnWidth, 0))) navigateTo(homePath);
        
        ImGui::Separator();
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
    ImGui::BeginGroup();
    
    // RIGHT PANEL: Media files + Actions
    if (ImGui::BeginChild("FilePanel", ImVec2(0, contentHeight - 40), true)) {
        // Actions at the top of the file list
        std::string addBtnText = "Add Selected";
        if (mode_ == BrowserMode::PLAYLIST_SELECTION) addBtnText = "Add to Playlist";
        else if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN) addBtnText = "Add & Return";
        else addBtnText = "Add to Library";

        if (ImGui::Button(addBtnText.c_str())) {
            int addedCount = 0;
            std::vector<std::string> addedPaths; 
            for (const auto& path : selectedFiles_) {
                 if (mode_ == BrowserMode::PLAYLIST_SELECTION && playlistController_ && !targetPlaylistName_.empty()) {
                     playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, path);
                 } else {
                     libController_->addMediaFile(path);
                     addedPaths.push_back(path);
                 }
                 addedCount++;
            }
            if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN && onFilesAddedCallback_) onFilesAddedCallback_(addedPaths);
            if (addedCount > 0) Logger::getInstance().info("Added " + std::to_string(addedCount) + " files.");
            if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN) visible_ = false;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            for (const auto& file : currentMediaFiles_) {
                selectedFiles_.insert(file.path);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            selectedFiles_.clear();
        }

        ImGui::Separator();
        
        // Table: [Select] [Name] [Extension]
        if (ImGui::BeginTable("FilesTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("##Select", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_NoHide); 
            ImGui::TableHeadersRow();
            
            size_t startIndex = static_cast<size_t>(currentPage_) * itemsPerPage_;
            size_t endIndex = std::min(startIndex + static_cast<size_t>(itemsPerPage_), currentMediaFiles_.size());
            
            for (size_t i = startIndex; i < endIndex; ++i) {
                const auto& fileInfo = currentMediaFiles_[i];
                std::string displayExt = (fileInfo.extension.length() > 0) ? fileInfo.extension.substr(1) : "";
                
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                bool isSelected = (selectedFiles_.find(fileInfo.path) != selectedFiles_.end());
                ImGui::PushID((int)i);
                if (ImGui::Checkbox("##check", &isSelected)) {
                    if (isSelected) selectedFiles_.insert(fileInfo.path);
                    else selectedFiles_.erase(fileInfo.path);
                }
                ImGui::PopID();
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", fileInfo.name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", displayExt.c_str());

                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", fileInfo.path.c_str());
            }
            ImGui::EndTable();
        }
        
        if (currentMediaFiles_.empty()) ImGui::TextDisabled("Empty folder");
    }
    ImGui::EndChild();

    renderPaginationControls();
    ImGui::EndGroup();
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
    
    // 2. Get recursive media files with max depth (Right Panel)
    currentMediaFiles_.clear();
    
    // Get all supported extensions
    std::vector<std::string> extensions = MediaFileFactory::getAllSupportedFormats();
    
    // Scan recursively with max depth of 3
    std::vector<std::string> mediaPaths = fileSystem_->getMediaFiles(currentPath_, extensions, 3);
    
    // Convert to FileInfo
    for (const auto& path : mediaPaths) {
        FileInfo info;
        info.path = path;
        
        // Extract filename and extension manually
        size_t lastSlash = path.find_last_of("/\\");
        info.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        size_t lastDot = info.name.find_last_of('.');
        info.extension = (lastDot != std::string::npos) ? info.name.substr(lastDot) : "";
        info.isDirectory = false;
        info.size = 0; 
        
        currentMediaFiles_.push_back(info);
    }
    
    updatePagination();
}

void FileBrowserView::updatePagination() {
    if (itemsPerPage_ <= 0) itemsPerPage_ = 15;
    totalPages_ = (int)((currentMediaFiles_.size() + itemsPerPage_ - 1) / itemsPerPage_);
    if (totalPages_ == 0) totalPages_ = 1; 
    
    // Safety clamp
    if (currentPage_ >= totalPages_) currentPage_ = totalPages_ - 1;
    if (currentPage_ < 0) currentPage_ = 0;
    
    snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
}

void FileBrowserView::renderPaginationControls() {
    float buttonWidth = 60.0f;
    float textWidth = 100.0f;
    float totalWidth = buttonWidth * 3 + textWidth + 80.0f; 
    
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + ImGui::GetCursorPosX());
    
    if (ImGui::Button("Prev", ImVec2(buttonWidth, 0))) {
        if (currentPage_ > 0) {
             currentPage_--;
             snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
        }
    }
    
    ImGui::SameLine();
    
    // Page Input
    ImGui::PushItemWidth(50.0f);
    if (ImGui::InputText("##PageInput", pageInputBuffer_, sizeof(pageInputBuffer_), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
        int page = std::atoi(pageInputBuffer_);
        if (page < 1) page = 1;
        if (page > totalPages_) page = totalPages_;
        currentPage_ = page - 1;
        snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    ImGui::Text("of %d", totalPages_);
    
    ImGui::SameLine();
    if (ImGui::Button("Go", ImVec2(buttonWidth, 0))) {
        int page = std::atoi(pageInputBuffer_);
        if (page < 1) page = 1;
        if (page > totalPages_) page = totalPages_;
        currentPage_ = page - 1;
        snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Next", ImVec2(buttonWidth, 0))) {
        if (currentPage_ < totalPages_ - 1) {
             currentPage_++;
             snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
        }
    }
}
