#include "app/view/LibraryView.h"
#include "app/view/FileBrowserView.h"
#include "app/model/PlaylistManager.h"
#include "utils/Logger.h"
#include "app/controller/PlaybackController.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <algorithm>

LibraryView::LibraryView(LibraryController* controller, Library* library, PlaybackController* playbackController, class PlaylistManager* playlistManager)
    : libraryController_(controller),
      library_(library), 
      selectedIndex_(-1) {
    
    // Initialize TrackListView base members
    listController_ = controller;
    playbackController_ = playbackController;
    playlistManager_ = playlistManager;
    
    // Attach as observer to library
    if (library_) {
        library_->attach(this);
        refreshDisplay(); // Initial population
    }
}

LibraryView::~LibraryView() {
    // Detach from library
    if (library_) {
        library_->detach(this);
    }
}

void LibraryView::render() {
    renderSearchBar();
    
    // Top Controls
    if (ImGui::Button("Add Files")) {
        if (fileBrowserView_) {
            fileBrowserView_->setMode(FileBrowserView::BrowserMode::LIBRARY);
            fileBrowserView_->show();
        }
    }
    
    ImGui::SameLine();
    
    // Use cached files list
    const auto& files = displayedFiles_;

    ImGui::Text("Library: %zu tracks", files.size());
    ImGui::Separator();
    
    renderEditToolbar(files);
    
    renderTrackListTable(files);
}

void LibraryView::renderSearchBar() {
    static char searchBuffer[256] = "";
    strncpy(searchBuffer, searchQuery_.c_str(), sizeof(searchBuffer));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.45f, 0.45f, 1.0f));
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
        searchQuery_ = searchBuffer;
        refreshDisplay();
    }
    ImGui::PopStyleColor();
}

void LibraryView::handleInput() {
    // Handled through ImGui
}

void LibraryView::update(void* subject) {
    (void)subject;
    selectedIndex_ = -1;  // Clear selection
    refreshDisplay();
}

void LibraryView::refreshDisplay() {
    // This view currently calculates display tracks on the fly in render()
    // but we can use this to cache or force a refresh if needed.
    if (library_) {
        displayedFiles_ = searchQuery_.empty() ? library_->getAll() : library_->search(searchQuery_);
    }
}

