#include "../../../inc/app/view/LibraryView.h"
#include "../../../inc/utils/Logger.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <algorithm>

LibraryView::LibraryView(LibraryController* controller, Library* library)
    : controller_(controller), library_(library), selectedIndex_(-1) {
    
    // Attach as observer to library
    if (library_) {
        library_->attach(this);
    }
}

LibraryView::~LibraryView() {
    // Detach from library
    if (library_) {
        library_->detach(this);
    }
}

void LibraryView::render() {
    if (!isVisible()) {
        return;
    }
    
    ImGui::Begin("Library", &visible_);
    
    // Search bar
    static char searchBuffer[256] = "";
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
        searchQuery_ = searchBuffer;
    }
    
    // Get files to display
    auto files = searchQuery_.empty() ? 
        library_->getAll() : 
        library_->search(searchQuery_);
    
    // Display file count
    ImGui::Text("Files: %zu", files.size());
    ImGui::Separator();
    
    // File list
    if (ImGui::BeginChild("FileList", ImVec2(0, -30), true)) {
        for (size_t i = 0; i < files.size(); ++i) {
            const auto& file = files[i];
            const auto& meta = file->getMetadata();
            
            bool isSelected = (static_cast<int>(i) == selectedIndex_);
            
            // Display: "Artist - Title" or filename
            std::string displayText = file->getDisplayName();
            if (!meta.artist.empty()) {
                displayText = meta.artist + " - " + displayText;
            }
            
            if (ImGui::Selectable(displayText.c_str(), isSelected)) {
                selectedIndex_ = static_cast<int>(i);
            }
            
            // Right-click context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove from Library")) {
                    controller_->removeMedia(file->getPath());
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::EndChild();
    
    // Bottom buttons
    if (ImGui::Button("Add File")) {
        // TODO: Open file dialog
        Logger::getInstance().info("Add File button clicked");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Add Directory")) {
        // TODO: Open directory dialog
        Logger::getInstance().info("Add Directory button clicked");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear Library")) {
        // TODO: Implement clearLibrary in controller
        selectedIndex_ = -1;
    }
    
    ImGui::End();
}

void LibraryView::handleInput() {
    // Input handling is done through ImGui callbacks in render()
}

void LibraryView::update(void* subject) {
    selectedIndex_ = -1;  // Clear selection
}
