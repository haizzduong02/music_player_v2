#include "../../../inc/app/view/HistoryView.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/app/controller/PlaybackController.h"
#include <imgui.h>
#include <string>

HistoryView::HistoryView(HistoryController* controller, History* history, PlaybackController* playbackController)
    : controller_(controller), history_(history), playbackController_(playbackController), selectedIndex_(-1) {
    
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
            
            // Fix ID conflict by appending ##i
            std::string label = displayText + "##hist_" + std::to_string(i);

            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex_ = static_cast<int>(i);

                // Double-click to play
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (playbackController_) {
                        // Clear playlist context (Library mode)
                        playbackController_->setCurrentPlaylist(nullptr);
                        playbackController_->play(track);
                        Logger::getInstance().info("Playing from history: " + displayText);
                    }
                }
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
