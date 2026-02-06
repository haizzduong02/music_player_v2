#include "../../../inc/app/view/LibraryView.h"
#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/app/model/PlaylistManager.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/app/controller/PlaybackController.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <algorithm>

LibraryView::LibraryView(LibraryController* controller, Library* library, PlaybackController* playbackController, PlaylistManager* playlistManager)
    : controller_(controller), library_(library), playbackController_(playbackController), playlistManager_(playlistManager), selectedIndex_(-1) {
    
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
    // Embedded render: no Begin/End
    
    // Search bar
    static char searchBuffer[256] = "";
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.45f, 0.45f, 1.0f)); // Custom color from MainWindow
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
        searchQuery_ = searchBuffer;
    }
    ImGui::PopStyleColor();
    
    // Get files to display
    auto files = searchQuery_.empty() ? 
        library_->getAll() : 
        library_->search(searchQuery_);
    
    // Top Controls
    if (ImGui::Button("Add Files")) {
        if (fileBrowserView_) {
            fileBrowserView_->setMode(FileBrowserView::BrowserMode::LIBRARY);
            fileBrowserView_->show();
        }
    }
    
    ImGui::SameLine();
    
    // Edit Mode Toggle
    if (isEditMode_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.0f, 1.0f)); 
        if (ImGui::Button("Done")) {
            isEditMode_ = false;
            selectedTracksForRemoval_.clear();
        }
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        if (ImGui::Button("Remove Selected")) {
            for (const auto& path : selectedTracksForRemoval_) {
                if (controller_) controller_->removeMedia(path);
            }
            selectedTracksForRemoval_.clear();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Select All")) {
             for (const auto& f : files) {
                 selectedTracksForRemoval_.insert(f->getPath());
             }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear All")) { // The "Delete All" requested
             if (library_) library_->clear();
             selectedTracksForRemoval_.clear();
        }
    } else {
        if (ImGui::Button("Edit")) {
            isEditMode_ = true;
            selectedTracksForRemoval_.clear();
        }
    }
    
    ImGui::Text("Library: %zu tracks", files.size());
    ImGui::Separator();
    
    // Calculate remaining height for scroll area
    float scrollHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("TrackListScroll", ImVec2(0, scrollHeight), true);
    
    std::string currentPlayingPath = "";
    if (playbackController_ && playbackController_->getPlaybackState() && playbackController_->getPlaybackState()->getCurrentTrack()) {
        currentPlayingPath = playbackController_->getPlaybackState()->getCurrentTrack()->getPath();
    }
    
    for (size_t i = 0; i < files.size(); ++i) {
        const auto& file = files[i];
        const auto& meta = file->getMetadata();
        bool isPlaying = (file->getPath() == currentPlayingPath);
        
        ImGui::PushID(static_cast<int>(i));
        
        std::string title = file->getDisplayName();
        std::string subtitle = meta.artist;
        if (!meta.album.empty()) {
            if (!subtitle.empty()) subtitle += " - ";
            subtitle += meta.album;
        }

        // Highlight currently playing
        if (isPlaying) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.00f, 0.60f, 0.60f, 1.0f)); // COLOR_ACCENT
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.00f, 0.60f, 0.60f, 1.0f));
        }
        
        // --- Layout Settings ---
        float trackItemHeight = 60.0f; 
        float buttonSize = 28.0f; 
        float btnSpacing = 5.0f;
        float paddingX = 10.0f;
        float paddingY = 8.0f; 
        
        float contentAvailX = ImGui::GetContentRegionAvail().x;
        ImVec2 startPosLocal = ImGui::GetCursorPos();
        ImVec2 startPosScreen = ImGui::GetCursorScreenPos();
        
        float buttonsAreaWidth = (buttonSize * 2) + btnSpacing + 15.0f; 
        // Adjust for checkbox in Edit Mode
        float checkboxWidth = 30.0f;
        float contentStartX = paddingX;
        
        if (isEditMode_) {
             contentStartX += checkboxWidth;
        }

        float textAreaWidth = contentAvailX - buttonsAreaWidth - contentStartX;

        // 1. Render Selectable Background
        // Use full width for selection, but in edit mode, clicking it might toggle checkbox? 
        // Or keep standard behavior but add checkbox overlay.
        bool clicked = ImGui::Selectable("##track", isPlaying, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0, trackItemHeight));
        
        ImVec2 endPosLocal = ImGui::GetCursorPos();
        
        // 1.5 Render Checkbox (Edit Mode)
        if (isEditMode_) {
            bool isSelected = (selectedTracksForRemoval_.find(file->getPath()) != selectedTracksForRemoval_.end());
            ImGui::SetCursorPos(ImVec2(startPosLocal.x + 5.0f, startPosLocal.y + (trackItemHeight - 20) / 2));
            ImGui::PushID((std::string("chk") + std::to_string(i)).c_str());
            if (ImGui::Checkbox("##check", &isSelected)) {
                 if (isSelected) selectedTracksForRemoval_.insert(file->getPath());
                 else selectedTracksForRemoval_.erase(file->getPath());
            }
            ImGui::PopID();
            
            // If the row was clicked in edit mode, toggle selection too (better UX)
            if (clicked) {
                 if (isSelected) selectedTracksForRemoval_.erase(file->getPath());
                 else selectedTracksForRemoval_.insert(file->getPath());
            }
        }
        
        // 3. Render Buttons (Right Aligned)
        float btn2X = startPosLocal.x + contentAvailX - buttonSize - 10.0f;
        float btn1X = btn2X - buttonSize - btnSpacing;
        float btnY = startPosLocal.y + (trackItemHeight - buttonSize) / 2.0f;
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); 
        
        // Button 1 (+)
        ImGui::SetCursorPos(ImVec2(btn1X, btnY));
        std::string addPopupId = "AddToPlaylistPopup##" + std::to_string(i);
        if (ImGui::Button(("+##add" + std::to_string(i)).c_str(), ImVec2(buttonSize, buttonSize))) {
            ImGui::OpenPopup(addPopupId.c_str());
        }
        
        if (ImGui::BeginPopup(addPopupId.c_str())) {
             ImGui::Text("Add to Playlist");
             ImGui::Separator();
             
             // Create New Playlist and Add
             ImGui::Text("Create New:");
             static char newPlaylistName[64] = "";
             ImGui::PushItemWidth(150.0f);
             ImGui::InputText("##newPlaylist", newPlaylistName, sizeof(newPlaylistName));
             ImGui::PopItemWidth();
             ImGui::SameLine();
             if (ImGui::Button("Add") && strlen(newPlaylistName) > 0) {
                 if (playlistManager_) {
                     std::string name(newPlaylistName);
                     if (!playlistManager_->exists(name)) {
                         auto newPl = playlistManager_->createPlaylist(name);
                         if (newPl) {
                             newPl->addTrack(file);
                             newPl->save();
                             Logger::getInstance().info("Created playlist and added track: " + name);
                         }
                         newPlaylistName[0] = '\0'; // Clear buffer
                         ImGui::CloseCurrentPopup();
                     } else {
                         // Maybe show error? For now just log
                         Logger::getInstance().warn("Playlist already exists: " + name);
                     }
                 }
             }
             
             ImGui::Separator();
             ImGui::Text("Existing:");
             if (playlistManager_) {
                 auto playlists = playlistManager_->getAllPlaylists();
                 bool found = false;
                 // Sort logic if needed, or just iterate
                 for (const auto& playlist : playlists) {
                     if (playlist->getName() == "Now Playing") continue;
                     found = true;
                     if (ImGui::Selectable(playlist->getName().c_str())) {
                         playlist->addTrack(file);
                         playlist->save();
                         ImGui::CloseCurrentPopup();
                     }
                 }
                 if (!found) ImGui::TextDisabled("No custom playlists");
             } else {
                 ImGui::TextDisabled("Unavailable");
             }
             ImGui::EndPopup();
        }
        
        // Button 2 (i)
        ImGui::SetCursorPos(ImVec2(btn2X, btnY));
        std::string metaPopupId = "MetadataPopup##" + std::to_string(i);
        if (ImGui::Button(("i" + std::string("##meta") + std::to_string(i)).c_str(), ImVec2(buttonSize, buttonSize))) {
            ImGui::OpenPopup(metaPopupId.c_str());
        }
        
        if (ImGui::BeginPopup(metaPopupId.c_str())) {
            ImGui::Text("Track Details");
            ImGui::Separator();
            ImGui::Text("File: %s", file->getDisplayName().c_str());
            ImGui::Text("Codec: %s", meta.codec.c_str());
            ImGui::Text("Bitrate: %d kbps", meta.bitrate);
            ImGui::Text("Sample Rate: %d Hz", meta.sampleRate);
            ImGui::Text("Channels: %d", meta.channels);
            if (meta.year > 0) ImGui::Text("Year: %d", meta.year);
            if (!meta.genre.empty()) ImGui::Text("Genre: %s", meta.genre.c_str());
            ImGui::EndPopup();
        }
        
        ImGui::PopStyleVar();
        
        // 4. Render Text (Using DrawList)
        ImGui::PushClipRect(startPosScreen, ImVec2(startPosScreen.x + textAreaWidth + contentStartX, startPosScreen.y + trackItemHeight), true);
        
        ImVec2 titlePos = ImVec2(startPosScreen.x + contentStartX, startPosScreen.y + paddingY);
        ImVec2 subtitlePos = ImVec2(startPosScreen.x + contentStartX, startPosScreen.y + paddingY + 24.0f);
        
        ImU32 titleColor = ImGui::GetColorU32(ImGuiCol_Text); // Always use Text color (White) for high contrast
        ImU32 subtitleColor = ImGui::GetColorU32(ImGuiCol_TextDisabled); // Using TextDisabled for dim color
        
        auto fonts = ImGui::GetIO().Fonts;
        ImFont* titleFont = (fonts->Fonts.Size > 2) ? fonts->Fonts[2] : ((fonts->Fonts.Size > 1) ? fonts->Fonts[1] : fonts->Fonts[0]);
        
        // --- Calculate Text Size & Animation ---
        ImGui::PushFont(titleFont);
        ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
        ImGui::PopFont(); // Pop to calculate/logic without affecting state yet
        
        float scrollOffsetX = 0.0f;
        float availTitleWidth = textAreaWidth; // Approx
        
        // Marquee Logic using StateStorage
        // Marquee Logic using StateStorage
        ImGuiID itemId = ImGui::GetID((void*)(intptr_t)i);
        float* pHoverTime = ImGui::GetStateStorage()->GetFloatRef(itemId, -1.0f);
        
        // Fix: IsItemHovered checks the last button. Use MouseHoveringRect for the row.
        bool isHovered = ImGui::IsMouseHoveringRect(startPosScreen, ImVec2(startPosScreen.x + contentAvailX, startPosScreen.y + trackItemHeight));
        
        if (isHovered && titleSize.x > availTitleWidth) {
            if (*pHoverTime < 0.0f) {
                *pHoverTime = static_cast<float>(ImGui::GetTime());
            }
            
            float driftTime = static_cast<float>(ImGui::GetTime()) - *pHoverTime;
            float initialDelay = 0.5f;
            
            if (driftTime > initialDelay) {
                float scrollTime = driftTime - initialDelay;
                float scrollSpeed = 30.0f; // pixels per second
                float maxScroll = titleSize.x - availTitleWidth + 20.0f; // Extra padding
                
                // One-way scroll with restart
                // Improved logic: Scroll -> Pause -> Snap
                float totalDuration = (maxScroll / scrollSpeed) + 2.0f; // +2s pause
                float currentCycle = fmodf(scrollTime, totalDuration);
                
                float scrollDuration = maxScroll / scrollSpeed;
                
                if (currentCycle < scrollDuration) {
                    scrollOffsetX = currentCycle * scrollSpeed;
                } else {
                    scrollOffsetX = maxScroll; // Hold at end
                }
            }
        } else {
            *pHoverTime = -1.0f; // Reset
        }
        
        // Render Title
        ImGui::PushFont(titleFont);
        ImVec2 drawnTitlePos = ImVec2(titlePos.x - scrollOffsetX, titlePos.y);
        ImGui::GetWindowDrawList()->AddText(drawnTitlePos, titleColor, title.c_str());
        ImGui::PopFont();
        
        // Render Subtitle (No Marquee for now, or use similar logic if needed, but user asked for Title)
        ImGui::GetWindowDrawList()->AddText(subtitlePos, subtitleColor, subtitle.c_str());
        
        ImGui::PopClipRect();
        
        // Restore header color
        if (isPlaying) { ImGui::PopStyleColor(2); }
        
        // Restore Cursor Position
        ImGui::SetCursorPos(endPosLocal);
        ImGui::Dummy(ImVec2(0,0));
        
        // Single click to play (only if not in edit mode)
        if (clicked && playbackController_ && !isEditMode_) {
             // Use playContext to set queue without setting a specific playlist object
             // This enables "Library Mode" where Previous uses History and Next uses Queue w/ Global Loop
             playbackController_->setCurrentPlaylist(nullptr);
             
             // Create a vector of shared_ptr from the raw pointer vector of the library if needed
             // But 'files' is already vector<shared_ptr<MediaFile>>
             playbackController_->playContext(files, i);
             
             selectedIndex_ = static_cast<int>(i);
        }
        
        ImGui::PopID();
    }
    ImGui::EndChild();
    

}

void LibraryView::handleInput() {
    // Input handling is done through ImGui callbacks in render()
}

void LibraryView::update(void* subject) {
    (void)subject;
    selectedIndex_ = -1;  // Clear selection
}
