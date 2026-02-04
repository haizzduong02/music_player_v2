#include "../../../inc/app/view/HistoryView.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>

HistoryView::HistoryView(HistoryController* controller, History* history)
    : controller_(controller), history_(history), selectedIndex_(-1) {
    
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
    if (!isVisible()) {
        return;
    }
    
    ImGui::Begin("History", &visible_);
    
    auto historyTracks = history_->getAll();
    
    ImGui::Text("Playback History (%zu tracks)", historyTracks.size());
    ImGui::Separator();
    
    // History list (most recent first)
    if (ImGui::BeginChild("HistoryList", ImVec2(0, -30), true)) {
        for (size_t i = 0; i < historyTracks.size(); ++i) {
            const auto& track = historyTracks[i];
            bool isSelected = (static_cast<int>(i) == selectedIndex_);
            
            std::string displayText = track->getDisplayName();
            const auto& meta = track->getMetadata();
            if (!meta.artist.empty()) {
                displayText = meta.artist + " - " + displayText;
            }
            
            if (ImGui::Selectable(displayText.c_str(), isSelected)) {
                selectedIndex_ = static_cast<int>(i);
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove from History")) {
                    controller_->removeFromHistory(i);
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::EndChild();
    
    // Bottom buttons
    if (ImGui::Button("Clear History")) {
        controller_->clearHistory();
        selectedIndex_ = -1;
    }
    
    ImGui::End();
}

void HistoryView::handleInput() {
    // Handled through ImGui
}

void HistoryView::update(void* subject) {
    // History changed - will re-render
    selectedIndex_ = -1;
}
