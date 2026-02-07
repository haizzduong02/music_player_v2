#ifndef TRACK_LIST_VIEW_H
#define TRACK_LIST_VIEW_H

#include "BaseView.h"
#include "../model/MediaFile.h"
#include "../interfaces/ITrackListController.h"
#include "../controller/PlaybackController.h"
#include "../model/PlaylistManager.h"
#include "imgui.h"
#include "../../utils/Logger.h"
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <cmath>
#include <algorithm>

/**
 * @brief Base class for views displaying a list of tracks with management capabilities.
 */
class TrackListView : public BaseView {
public:
    TrackListView() 
        : isEditMode_(false), 
          listController_(nullptr), 
          playbackController_(nullptr), 
          playlistManager_(nullptr) {}
    
    virtual ~TrackListView() = default;

protected:
    // Shared State
    bool isEditMode_;
    std::string searchQuery_;
    std::set<std::string> selectedPaths_;
    ITrackListController* listController_;
    PlaybackController* playbackController_;
    PlaylistManager* playlistManager_;
    
    // UI Helpers
    void renderEditToolbar(const std::vector<std::shared_ptr<MediaFile>>& tracks) {
        if (isEditMode_) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f)); 
            if (ImGui::Button("Done")) {
                toggleEditMode();
            }
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                removeSelectedTracks();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Select All")) {
                 selectAll(tracks);
            }
        } else {
            if (ImGui::Button("Edit")) {
                toggleEditMode();
            }
        }
    }
    
    void renderTrackListTable(const std::vector<std::shared_ptr<MediaFile>>& tracks) {
        float scrollHeight = ImGui::GetContentRegionAvail().y;
        if (scrollHeight < 100) scrollHeight = 100;
        
        ImGui::BeginChild("TrackListContent", ImVec2(0, scrollHeight), false);
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            auto file = tracks[i];
            if (!file) continue;

            const auto& meta = file->getMetadata();
            std::string title = file->getDisplayName();
            std::string artist = meta.artist.empty() ? "Unknown Artist" : meta.artist;
            std::string album = meta.album.empty() ? "Unknown Album" : meta.album;
            std::string subtitle = artist + " • " + album;
            
            bool isPlaying = false;
            if (playbackController_) {
                auto current = playbackController_->getPlaybackState() ? playbackController_->getPlaybackState()->getCurrentTrack() : nullptr;
                if (current && current->getPath() == file->getPath()) {
                    isPlaying = true;
                }
            }

            ImGui::PushID(static_cast<int>(i));
            
            if (isPlaying) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.5f, 0.5f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.6f, 0.6f, 1.0f));
            }

            float trackItemHeight = 60.0f;
            float buttonSize = 28.0f;
            float btnSpacing = 5.0f;
            float paddingX = 10.0f;
            float paddingY = 8.0f;

            float contentAvailX = ImGui::GetContentRegionAvail().x;
            ImVec2 startPosLocal = ImGui::GetCursorPos();
            ImVec2 startPosScreen = ImGui::GetCursorScreenPos();

            float buttonsAreaWidth = (buttonSize * 2) + btnSpacing + 15.0f;
            float checkboxWidth = 30.0f;
            float contentStartX = paddingX;

            if (isEditMode_) contentStartX += checkboxWidth;

            float textAreaWidth = contentAvailX - buttonsAreaWidth - contentStartX;

            bool clicked = ImGui::Selectable("##track", isPlaying, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(contentAvailX, trackItemHeight));
            ImVec2 endPosLocal = ImGui::GetCursorPos();

            if (isEditMode_) {
                bool selected = isSelected(file->getPath());
                ImGui::SetCursorPos(ImVec2(startPosLocal.x + 5.0f, startPosLocal.y + (trackItemHeight - 20) / 2));
                if (ImGui::Checkbox("##check", &selected)) {
                    toggleSelection(file->getPath());
                }
                if (clicked) toggleSelection(file->getPath());
            }

            // Buttons
            float btn2X = startPosLocal.x + contentAvailX - buttonSize - 10.0f;
            float btn1X = btn2X - buttonSize - btnSpacing;
            float btnY = startPosLocal.y + (trackItemHeight - buttonSize) / 2.0f;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::SetCursorPos(ImVec2(btn1X, btnY));
            std::string addPopupId = "AddToPlaylistPopup##" + std::to_string(i);
            if (ImGui::Button("+", ImVec2(buttonSize, buttonSize))) {
                ImGui::OpenPopup(addPopupId.c_str());
            }

            if (ImGui::BeginPopup(addPopupId.c_str())) {
                ImGui::Text("Add to Playlist");
                ImGui::Separator();
                
                if (playlistManager_) {
                    auto playlists = playlistManager_->getAllPlaylists();
                    
                    // 1. List existing playlists
                    if (ImGui::BeginChild("PlaylistListSub", ImVec2(200, 150), false)) {
                        for (const auto& playlist : playlists) {
                            if (playlist->getName() == "Now Playing") continue;
                            if (ImGui::Selectable(playlist->getName().c_str())) {
                                playlist->addTrack(file);
                                playlist->save();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                    ImGui::EndChild();
                    
                    ImGui::Separator();
                    
                    // 2. Create new playlist and add
                    static char newPlaylistBuffer[128] = "";
                    ImGui::PushItemWidth(160);
                    ImGui::InputTextWithHint("##new_pl", "New Playlist...", newPlaylistBuffer, sizeof(newPlaylistBuffer));
                    ImGui::PopItemWidth();
                    
                    ImGui::SameLine();
                    if (ImGui::Button("+##create_add", ImVec2(30, 0))) {
                        std::string name(newPlaylistBuffer);
                        if (!name.empty()) {
                            auto newPl = playlistManager_->createPlaylist(name);
                            if (newPl) {
                                newPl->addTrack(file);
                                newPl->save();
                                playlistManager_->saveAll(); // Ensure the new playlist itself is saved in the manager
                                newPlaylistBuffer[0] = '\0';
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::SetCursorPos(ImVec2(btn2X, btnY));
            if (ImGui::Button("i", ImVec2(buttonSize, buttonSize))) {
                ImGui::OpenPopup("MetadataPopup");
            }

            if (ImGui::BeginPopup("MetadataPopup")) {
                ImGui::Text("Track Details");
                ImGui::Separator();
                ImGui::Text("File: %s", file->getDisplayName().c_str());
                ImGui::Text("Artist: %s", artist.c_str());
                ImGui::Text("Album: %s", album.c_str());
                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();

            // Text with Marquee
            ImGui::PushClipRect(startPosScreen, ImVec2(startPosScreen.x + textAreaWidth + contentStartX, startPosScreen.y + trackItemHeight), true);
            ImVec2 titlePos = ImVec2(startPosScreen.x + contentStartX, startPosScreen.y + paddingY);
            ImVec2 subtitlePos = ImVec2(startPosScreen.x + contentStartX, startPosScreen.y + paddingY + 24.0f);
            
            auto fonts = ImGui::GetIO().Fonts;
            ImFont* titleFont = (fonts->Fonts.Size > 2) ? fonts->Fonts[2] : fonts->Fonts[0];
            
            ImGui::PushFont(titleFont);
            ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
            float scrollOffsetX = 0.0f;
            if (isHoveredRow(startPosScreen, ImVec2(contentAvailX, trackItemHeight)) && titleSize.x > textAreaWidth) {
                scrollOffsetX = calculateMarqueeOffset(titleSize.x, textAreaWidth, i);
            }
            ImGui::GetWindowDrawList()->AddText(ImVec2(titlePos.x - scrollOffsetX, titlePos.y), ImGui::GetColorU32(ImGuiCol_Text), title.c_str());
            ImGui::PopFont();
            ImGui::GetWindowDrawList()->AddText(subtitlePos, ImGui::GetColorU32(ImGuiCol_TextDisabled), subtitle.c_str());
            ImGui::PopClipRect();

            if (isPlaying) ImGui::PopStyleColor(2);

            ImGui::SetCursorPos(endPosLocal);
            // Submit a dummy item to ensure window boundaries are updated correctly after manual positioning
            ImGui::Dummy(ImVec2(0, 0)); 

            if (clicked && playbackController_ && !isEditMode_) {
                listController_->playTrack(tracks, i);
            }

            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    
    void toggleEditMode() {
        isEditMode_ = !isEditMode_;
        if (!isEditMode_) {
            selectedPaths_.clear();
        }
    }
    
    void selectAll(const std::vector<std::shared_ptr<MediaFile>>& tracks) {
        for (const auto& track : tracks) {
            if (track) selectedPaths_.insert(track->getPath());
        }
    }
    
    bool isSelected(const std::string& path) const {
        return selectedPaths_.find(path) != selectedPaths_.end();
    }
    
    void toggleSelection(const std::string& path) {
        if (isSelected(path)) {
            selectedPaths_.erase(path);
        } else {
            selectedPaths_.insert(path);
        }
    }

    void removeSelectedTracks() {
        if (listController_) {
            listController_->removeTracks(selectedPaths_);
            selectedPaths_.clear();
        }
    }

private:
    bool isHoveredRow(const ImVec2& screenPos, const ImVec2& size) {
        return ImGui::IsMouseHoveringRect(screenPos, ImVec2(screenPos.x + size.x, screenPos.y + size.y));
    }

    float calculateMarqueeOffset(float textWidth, float availWidth, size_t index) {
        ImGuiID itemId = ImGui::GetID((void*)(intptr_t)index);
        float* pHoverTime = ImGui::GetStateStorage()->GetFloatRef(itemId, -1.0f);
        if (*pHoverTime < 0.0f) *pHoverTime = static_cast<float>(ImGui::GetTime());
        
        float driftTime = static_cast<float>(ImGui::GetTime()) - *pHoverTime;
        if (driftTime < 0.5f) return 0.0f;
        
        float maxScroll = textWidth - availWidth + 20.0f;
        float speed = 30.0f;
        float duration = (maxScroll / speed) + 2.0f;
        float cycle = fmodf(driftTime - 0.5f, duration);
        
        return (cycle < maxScroll / speed) ? cycle * speed : maxScroll;
    }
};

#endif // TRACK_LIST_VIEW_H
