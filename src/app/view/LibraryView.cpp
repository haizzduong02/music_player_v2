#include "../../../inc/app/view/LibraryView.h"
#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/app/model/PlaylistManager.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/app/controller/PlaybackController.h"

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
    
    auto files = searchQuery_.empty() ? 
        library_->getAll() : 
        library_->search(searchQuery_);

    renderEditToolbar(files);
    
    ImGui::Text("Library: %zu tracks", files.size());
    ImGui::Separator();
    
    renderTrackListTable(files);
}

void LibraryView::renderSearchBar() {
    static char searchBuffer[256] = "";
    strncpy(searchBuffer, searchQuery_.c_str(), sizeof(searchBuffer));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.45f, 0.45f, 1.0f));
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
        searchQuery_ = searchBuffer;
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

