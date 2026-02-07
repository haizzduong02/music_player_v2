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
    // Embedded render: no Begin/End
    
    auto historyTracks = history_->getAll();
    
    ImGui::Text("Playback History (%zu tracks)", historyTracks.size());
    
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) {
        controller_->clearHistory();
        selectedIndex_ = -1;
        selectedPaths_.clear();
    }
    
    ImGui::SameLine();
    // Edit Mode Toggle
    if (isEditMode_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f));
        if (ImGui::Button("Done")) {
            toggleEditMode();
        }
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (ImGui::Button("Remove Selected")) {
            removeSelectedTracks();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
            selectAll(historyTracks);
        }
    } else {
        if (ImGui::Button("Edit")) {
            toggleEditMode();
        }
    }
    
    ImGui::Separator();
    
    float scrollHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("HistoryList", ImVec2(0, scrollHeight), true);
    
    std::string currentPlayingPath = "";
    if (playbackController_ && playbackController_->getPlaybackState() && playbackController_->getPlaybackState()->getCurrentTrack()) {
        currentPlayingPath = playbackController_->getPlaybackState()->getCurrentTrack()->getPath();
    }
    
    for (size_t i = 0; i < historyTracks.size(); ++i) {
        const auto& track = historyTracks[i];
        const auto& meta = track->getMetadata();
        bool isPlaying = (track->getPath() == currentPlayingPath);
        
        ImGui::PushID(static_cast<int>(i));
        
        std::string title = track->getDisplayName();
        std::string subtitle = meta.artist;
        if (!meta.album.empty()) {
            if (!subtitle.empty()) subtitle += " - ";
            subtitle += meta.album;
        }

        if (isPlaying) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.00f, 0.60f, 0.60f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.00f, 0.60f, 0.60f, 1.0f));
        }
        
        // --- Layout Settings ---
        float trackItemHeight = 60.0f; 
        float paddingX = 10.0f;
        float paddingY = 8.0f; 
        
        float contentAvailX = ImGui::GetContentRegionAvail().x;
        
        ImVec2 startPosScreen = ImGui::GetCursorScreenPos();
        
        // Adjust for checkbox in Edit Mode
        float checkboxWidth = 30.0f;
        float contentStartX = paddingX;
        if (isEditMode_) contentStartX += checkboxWidth;
        
        float textAreaWidth = contentAvailX - contentStartX;

        // 1. Render Selectable
        bool clicked = ImGui::Selectable("##hist", isPlaying, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0, trackItemHeight));

        // Render Checkbox 
        if (isEditMode_) {
            bool selected = isSelected(track->getPath());
            ImGui::SetCursorPos(ImVec2(startPosScreen.x + 5.0f - ImGui::GetWindowPos().x, startPosScreen.y + (trackItemHeight - 20) / 2 - ImGui::GetWindowPos().y));
            // Apply similar relative positioning logic as PlaylistView
            ImVec2 localPos = ImVec2(startPosScreen.x - ImGui::GetWindowPos().x + ImGui::GetScrollX() + 5.0f, 
                                     startPosScreen.y - ImGui::GetWindowPos().y + ImGui::GetScrollY() + (trackItemHeight - 20) / 2);
            ImGui::SetCursorPos(localPos);

            ImGui::PushID((std::string("chk") + std::to_string(i)).c_str());
            if (ImGui::Checkbox("##check", &selected)) {
                 toggleSelection(track->getPath());
            }
            ImGui::PopID();
            
            if (clicked) toggleSelection(track->getPath());
        }
        
        // Context menu - MUST FOLLOW Selectable immediately
        if (ImGui::BeginPopupContextItem("hist_track_ctx")) {
            if (ImGui::MenuItem("Remove from History")) {
                controller_->removeFromHistory(i);
            }
            ImGui::EndPopup();
        }
        
        ImVec2 endPosLocal = ImGui::GetCursorPos();
        
        // 2. Render Text (Using DrawList)
        ImGui::PushClipRect(startPosScreen, ImVec2(startPosScreen.x + textAreaWidth, startPosScreen.y + trackItemHeight), true);
        
        ImVec2 titlePos = ImVec2(startPosScreen.x + paddingX, startPosScreen.y + paddingY);
        ImVec2 subtitlePos = ImVec2(startPosScreen.x + paddingX, startPosScreen.y + paddingY + 24.0f);
        
        ImU32 titleColor = ImGui::GetColorU32(ImGuiCol_Text);
        ImU32 subtitleColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        
        auto fonts = ImGui::GetIO().Fonts;
        ImFont* titleFont = (fonts->Fonts.Size > 2) ? fonts->Fonts[2] : ((fonts->Fonts.Size > 1) ? fonts->Fonts[1] : fonts->Fonts[0]);
        
        // --- Marquee Logic ---
        ImGui::PushFont(titleFont);
        ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
        ImGui::PopFont();
        
        float scrollOffsetX = 0.0f;
        float availTitleWidth = textAreaWidth;
        
        ImGuiID itemId = ImGui::GetID((void*)(intptr_t)i);
        float* pHoverTime = ImGui::GetStateStorage()->GetFloatRef(itemId, -1.0f);
        
        // Check hover
        bool isHovered = ImGui::IsMouseHoveringRect(startPosScreen, ImVec2(startPosScreen.x + contentAvailX, startPosScreen.y + trackItemHeight));
        
        if (isHovered && titleSize.x > availTitleWidth) {
            if (*pHoverTime < 0.0f) *pHoverTime = static_cast<float>(ImGui::GetTime());
            
            float driftTime = static_cast<float>(ImGui::GetTime()) - *pHoverTime;
            float initialDelay = 0.5f;
            
            if (driftTime > initialDelay) {
                float scrollTime = driftTime - initialDelay;
                float scrollSpeed = 30.0f;
                float maxScroll = titleSize.x - availTitleWidth + 20.0f;
                
                float totalDuration = (maxScroll / scrollSpeed) + 2.0f;
                float currentCycle = fmodf(scrollTime, totalDuration);
                
                if (currentCycle < (maxScroll / scrollSpeed)) {
                    scrollOffsetX = currentCycle * scrollSpeed;
                } else {
                    scrollOffsetX = maxScroll;
                }
            }
        } else {
            *pHoverTime = -1.0f;
        }
        
        ImGui::PushFont(titleFont);
        ImVec2 drawnTitlePos = ImVec2(titlePos.x - scrollOffsetX, titlePos.y);
        ImGui::GetWindowDrawList()->AddText(drawnTitlePos, titleColor, title.c_str());
        ImGui::PopFont();
        
        ImGui::GetWindowDrawList()->AddText(subtitlePos, subtitleColor, subtitle.c_str());
        
        ImGui::PopClipRect();
        
        if (isPlaying) { ImGui::PopStyleColor(2); }
        
        // Restore Cursor
        ImGui::SetCursorPos(endPosLocal);
        ImGui::Dummy(ImVec2(0,0));
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
             if (playbackController_) {
                 playbackController_->setCurrentPlaylist(nullptr);
                 playbackController_->play(track);
             }
        }
        
        // Context menu removed from here (moved up)
        
        ImGui::PopID();
    }
    ImGui::EndChild();
    
}

void HistoryView::handleInput() {
    // Handled through ImGui
}

void HistoryView::update(void* subject) {
    (void)subject;
    // History changed - will re-render
    selectedIndex_ = -1;
}

void HistoryView::removeSelectedTracks() {
    for (const auto& path : selectedPaths_) {
        controller_->removeFromHistoryByPath(path);
    }
    selectedPaths_.clear();
}
