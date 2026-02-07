#include "../../../inc/app/view/HistoryView.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/app/controller/PlaybackController.h"
#include <imgui.h>
#include <string>

HistoryView::HistoryView(HistoryController* controller, History* history, PlaybackController* playbackController, PlaylistManager* playlistManager)
    : historyController_(controller), history_(history), selectedIndex_(-1) {
    
    // Initialize TrackListView base members
    listController_ = controller;
    playbackController_ = playbackController;
    playlistManager_ = playlistManager;
    
    // Attach as observer to history
    if (history_) {
        history_->attach(this);
    }
}

HistoryView::~HistoryView() {
    // Detach from history
    if (history_) {
        history_->detach(this);
    }
}

void HistoryView::render() {
    // Embedded render: no Begin/End
    
    auto historyTracks = history_->getAll();
    
    ImGui::Text("Playback History (%zu tracks)", historyTracks.size());
    ImGui::Separator();
    
    renderEditToolbar(historyTracks);
    
    renderTrackListTable(historyTracks);
}

void HistoryView::handleInput() {
    // Handled through ImGui
}

void HistoryView::update(void* subject) {
    (void)subject;
    // History changed - will re-render
    selectedIndex_ = -1;
}

