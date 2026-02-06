#include "../../../inc/app/view/PlaylistView.h"
#include "../../../inc/app/view/FileBrowserView.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/app/controller/PlaybackController.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

PlaylistView::PlaylistView(PlaylistController* controller, PlaylistManager* manager, PlaybackController* playbackController)
    : controller_(controller), manager_(manager), playbackController_(playbackController), selectedTrackIndex_(-1), showCreateDialog_(false), showRenameDialog_(false) {
    
    // Attach as observer to manager
    if (manager_) {
        manager_->attach(this);
    }
}

PlaylistView::~PlaylistView() {
    // Detach from manager
    if (manager_) {
        manager_->detach(this);
    }
}

void PlaylistView::render() {
    // Embedded render: no Begin/End
    
    auto playlists = manager_->getAllPlaylists();
    
    // Top panel: Playlist list (Fixed height)
    ImGui::BeginChild("PlaylistList", ImVec2(0, 150), true);
    ImGui::Text("Playlists");
    ImGui::Separator();
    
    for (size_t i = 0; i < playlists.size(); ++i) {
        if (playlists[i]->getName() == "Now Playing") continue;
        
        bool isSelected = (playlists[i]->getName() == selectedPlaylistName_);
        
        if (ImGui::Selectable(playlists[i]->getName().c_str(), isSelected)) {
            selectedPlaylistName_ = playlists[i]->getName();
            selectedPlaylist_ = playlists[i];
            selectedTrackIndex_ = -1;
        }
    }
    
    ImGui::Separator();
    
    // New playlist button
    if (ImGui::Button("New Playlist")) {
        showCreateDialog_ = true;
    }
    
    ImGui::EndChild();
    
    // Bottom panel: Playlist tracks (Remaining height)
    ImGui::BeginChild("PlaylistTracks", ImVec2(0, 0), true);
    
    if (selectedPlaylist_) {
        ImGui::Text("%s", selectedPlaylist_->getName().c_str());
        ImGui::Text("Tracks: %zu", selectedPlaylist_->size());
        
        ImGui::SameLine();
        if (ImGui::Button("Shuffle")) {
            controller_->shufflePlaylist(selectedPlaylist_->getName());
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Files")) {
            shouldOpenAddPopup_ = true;
        }
        
        ImGui::Separator();
        
        auto tracks = selectedPlaylist_->getTracks();
        
        std::string currentPlayingPath = "";
        if (playbackController_ && playbackController_->getPlaybackState() && playbackController_->getPlaybackState()->getCurrentTrack()) {
            currentPlayingPath = playbackController_->getPlaybackState()->getCurrentTrack()->getPath();
        }
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            const auto& track = tracks[i];
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
            
            float textAreaWidth = contentAvailX - paddingX;

            // 1. Render Selectable
            bool clicked = ImGui::Selectable("##pltrack", isPlaying, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, trackItemHeight));
            
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
            
            // Use Index i for ID
            ImGuiID itemId = ImGui::GetID((void*)(intptr_t)i);
            float* pHoverTime = ImGui::GetStateStorage()->GetFloatRef(itemId, -1.0f);
            
            // Check hover on selectable (Item 1)
            // But we already rendered Dummy and SetCursorPos...
            // ImGui::IsItemHovered() here checks the Dummy at line 137? No, Check previous Item (Selectable at 122).
            // Wait, we used Selectable at line 122, then GetCursorPos...
            // We should use clicked state from line 122, but for Hover, we can check IsItemHovered() if we are still in same frame context?
            // Actually `clicked` is bool. Selectable is the Item.
            // Let's rely on `ImGui::IsItemHovered()` (Selectable is likely the last item or we can check rect).
            // The Selectable was drawn, then Dummy? No, Selectable is line 122.
            // We are at line 130 approx.
            // Let's ensure we are checking the row.
            // Actually, we can just check if mouse is in the rect.
            
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
            
            ImGui::SetCursorPos(endPosLocal);
            ImGui::Dummy(ImVec2(0,0));
            
            if (clicked) {
                if (playbackController_) {
                    playbackController_->setCurrentPlaylist(selectedPlaylist_.get());
                    playbackController_->play(tracks[i]);
                    Logger::getInstance().info("Playing from playlist: " + title);
                }
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove")) {
                    controller_->removeFromPlaylist(selectedPlaylist_->getName(), i);
                }
                ImGui::EndPopup();
            }
            
            ImGui::PopID();
        }
    } else {
        ImGui::Text("Select a playlist");
    }
    
    ImGui::EndChild();
    

    
    // New playlist dialog
    if (showCreateDialog_) {
        ImGui::OpenPopup("New Playlist");
        showCreateDialog_ = false;
    }
    
    if (ImGui::BeginPopupModal("New Playlist", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char nameBuffer[256] = "";
        ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));
        
        if (ImGui::Button("Create")) {
            controller_->createPlaylist(nameBuffer);
            nameBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel")) {
            nameBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void PlaylistView::handleInput() {
    // Handled through ImGui
}

void PlaylistView::update(void* subject) {
    (void)subject;
    // Playlist manager changed - will re-render
}

void PlaylistView::renderPopups() {
    // Check if we were browsing and the browser is now closed
    if (isBrowsingForPlaylist_ && fileBrowserView_ && !fileBrowserView_->isVisible()) {
         shouldReopenAddPopup_ = true;
         isBrowsingForPlaylist_ = false;
    }

    renderAddSongsPopup();
}

void PlaylistView::renderAddSongsPopup() {
    if (shouldOpenAddPopup_) {
         showAddSongsPopup_ = true;
         songSearchQuery_ = "";
         selectedTracksForAdd_.clear();
         ImGui::OpenPopup("Add Songs to Playlist");
         shouldOpenAddPopup_ = false;
    }

    if (shouldReopenAddPopup_) {
        showAddSongsPopup_ = true;
        ImGui::OpenPopup("Add Songs to Playlist");
        shouldReopenAddPopup_ = false;
    }

    if (!showAddSongsPopup_) return;

    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Add Songs to Playlist", &showAddSongsPopup_)) {
        
        // --- 1. Top Controls: Search & Browse ---
        ImGui::BeginGroup();
        {
            char buffer[256];
            strncpy(buffer, songSearchQuery_.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = 0;
            
            ImGui::SetNextItemWidth(350);
            if (ImGui::InputTextWithHint("##search", "Search Library...", buffer, sizeof(buffer))) {
                songSearchQuery_ = buffer;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Browse Files...")) {
                if (fileBrowserView_) {
                    fileBrowserView_->setMode(FileBrowserView::BrowserMode::LIBRARY_ADD_AND_RETURN);
                    // Set callback to tick added files
                    fileBrowserView_->setOnFilesAddedCallback([this](const std::vector<std::string>& files) {
                        for (const auto& f : files) {
                            Logger::getInstance().info("Callback: Auto-selecting " + f);
                            selectedTracksForAdd_.insert(f);
                        }
                        // Note: we don't need to set shouldReopenAddPopup_ here anymore, 
                        // logic in renderPopups handles it when browser closes.
                    });
                    fileBrowserView_->show();
                    
                    // Hide this popup so browser can be seen/used
                    showAddSongsPopup_ = false;
                    ImGui::CloseCurrentPopup();
                    
                    isBrowsingForPlaylist_ = true;
                }
            }
        }
        ImGui::EndGroup();
        
        ImGui::Separator();
        
        // --- 2. Track List Table ---
        ImVec2 available = ImGui::GetContentRegionAvail();
        float listHeight = available.y - 40; // Leave space for buttons
        
        if (ImGui::BeginChild("TrackList", ImVec2(0, listHeight), true)) {
            // Retrieve all tracks
            auto* lib = controller_->getLibrary();
            if (lib) {
                auto allTracks = lib->getAll();
                
                // Filter
                std::vector<std::shared_ptr<MediaFile>> displayTracks;
                if (songSearchQuery_.empty()) {
                    displayTracks = allTracks;
                } else {
                    std::string queryLower = songSearchQuery_;
                    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
                    for (const auto& t : allTracks) {
                        std::string title = t->getDisplayName();
                        std::string artist = t->getMetadata().artist;
                        std::transform(title.begin(), title.end(), title.begin(), ::tolower);
                        std::transform(artist.begin(), artist.end(), artist.begin(), ::tolower);
                        
                        if (title.find(queryLower) != std::string::npos || artist.find(queryLower) != std::string::npos) {
                            displayTracks.push_back(t);
                        }
                    }
                }
                
                // Render Table
                if (ImGui::BeginTable("AddTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Select", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                    ImGui::TableSetupColumn("Title");
                    ImGui::TableSetupColumn("Artist");
                    ImGui::TableHeadersRow();
                    
                    for (const auto& track : displayTracks) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        
                        // Checkbox
                        bool isSelected = selectedTracksForAdd_.find(track->getPath()) != selectedTracksForAdd_.end();
                        ImGui::PushID(track->getPath().c_str());
                        if (ImGui::Checkbox("##chk", &isSelected)) {
                            if (isSelected) selectedTracksForAdd_.insert(track->getPath());
                            else selectedTracksForAdd_.erase(track->getPath());
                        }
                        ImGui::PopID();
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", track->getDisplayName().c_str());
                        
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", track->getMetadata().artist.c_str());
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::EndChild();
        
        // --- 3. Bottom Buttons ---
        ImGui::Separator();
        
        if (ImGui::Button("Add Selected", ImVec2(120, 0))) {
            if (selectedPlaylist_) {
                for (const auto& path : selectedTracksForAdd_) {
                    controller_->addToPlaylist(selectedPlaylist_->getName(), path);
                }
                // Save is handled within addToPlaylist usually or manual calling might be needed
                // Assuming controller works as expected. 
                // Currently addToPlaylist just adds to model. We might need to ensure persistence. 
                // PlaylistManager::save(playlist) is usually called by controller or manager.
            }
            ImGui::CloseCurrentPopup();
            showAddSongsPopup_ = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            showAddSongsPopup_ = false;
        }
        
        ImGui::EndPopup();
    }
}
