#include "app/view/PlaylistView.h"
#include "app/view/FileBrowserView.h"
#include "utils/Logger.h"
#include "app/controller/PlaybackController.h"
#include "app/controller/PlaylistTrackListController.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

PlaylistView::PlaylistView(PlaylistController* controller, PlaylistManager* manager, PlaybackController* playbackController)
    : playlistController_(controller), selectedTrackIndex_(-1), showCreateDialog_(false), showRenameDialog_(false) {
    
    // Initialize TrackListView base members
    playbackController_ = playbackController;
    playlistManager_ = manager;
    // listController_ will be set dynamically when a playlist is selected
    
    // Attach as observer to playlistManager_
    if (playlistManager_) {
        playlistManager_->attach(this);
    }
}

PlaylistView::~PlaylistView() {
    // Detach from playlistManager_
    if (playlistManager_) {
        playlistManager_->detach(this);
    }
}

void PlaylistView::render() {
    // Embedded render: no Begin/End
    
    auto playlists = playlistManager_->getAllPlaylists();
    
    // Top panel: Playlist list (Fixed height)
    ImGui::BeginChild("PlaylistList", ImVec2(0, 150), true);
    ImGui::Text("Playlists");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 25);
    if (ImGui::Button("+", ImVec2(25, 0))) {
        showCreateDialog_ = true;
    }
    ImGui::Separator();
    
    for (size_t i = 0; i < playlists.size(); ++i) {
        if (playlists[i]->getName() == "Now Playing") continue;
        
        bool isSelectedPl = (playlists[i]->getName() == selectedPlaylistName_);
        
        ImGui::PushID(static_cast<int>(i));
        
        float sidebarWidth = ImGui::GetContentRegionAvail().x;
        float delBtnWidth = 24.0f;
        
        if (ImGui::Selectable(playlists[i]->getName().c_str(), isSelectedPl, 0, ImVec2(sidebarWidth - delBtnWidth - 5, 0))) {
            selectedPlaylistName_ = playlists[i]->getName();
            selectedPlaylist_ = playlists[i];
            selectedTrackIndex_ = -1;
            selectedPaths_.clear(); 
            isEditMode_ = false;   
            
            // Re-create adapter for the selected playlist
            if (playlistController_ && selectedPlaylist_) {
                static std::shared_ptr<ITrackListController> adapter; 
                adapter = std::make_shared<PlaylistTrackListController>(playlistController_, selectedPlaylist_, playbackController_);
                listController_ = adapter.get();
            }
        }
        
        // Playlist Delete Button
        bool isSystem = (playlists[i]->getName() == "Now Playing" || playlists[i]->getName() == "Favorites");
        ImGui::SameLine(sidebarWidth - delBtnWidth);
        
        if (isSystem) {
             ImGui::BeginDisabled();
        }
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("X", ImVec2(delBtnWidth, 0))) {
            if (playlistController_->deletePlaylist(playlists[i]->getName())) {
                if (isSelectedPl) {
                    selectedPlaylist_ = nullptr;
                    selectedPlaylistName_ = "";
                }
            }
        }
        ImGui::PopStyleColor();
        
        if (isSystem) {
             ImGui::EndDisabled();
        }
        
        ImGui::PopID();
    }
    
    ImGui::EndChild(); // End PlaylistList
    
    // Bottom panel: Playlist tracks (Remaining height)
    ImGui::BeginChild("PlaylistTracks", ImVec2(0, 0), true);
    
    if (selectedPlaylist_) {
        ImGui::Text("%s", selectedPlaylist_->getName().c_str());
        ImGui::Text("Tracks: %zu", selectedPlaylist_->size());
        
        ImGui::SameLine();
        if (ImGui::Button("Shuffle")) {
            playlistController_->shufflePlaylist(selectedPlaylist_->getName());
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Files")) {
            shouldOpenAddPopup_ = true;
        }
        
        ImGui::Separator();
        
        auto tracks = selectedPlaylist_->getTracks();
        renderEditToolbar(tracks);
        
        renderTrackListTable(tracks);
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
            playlistController_->createPlaylist(nameBuffer);
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
    // playlistManager_ changed - will re-render
}


void PlaylistView::renderPopups() {
    // Check if we were browsing and the browser is now closed
    // Check if we were browsing and the browser is now closed
    if (isBrowsingForPlaylist_ && fileBrowserView_ && !fileBrowserView_->isVisible()) {
         // Browser closed, just reset flag. Do NOT reopen Add Popup as it is already open (nested).
         isBrowsingForPlaylist_ = false;
    }

    renderAddSongsPopup();
}

void PlaylistView::renderAddSongsPopup() {
    if (shouldOpenAddPopup_) {
         showAddSongsPopup_ = true;
         searchQuery_ = "";
         trackSelector_.clearSelection();
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
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Add Songs to Playlist", &showAddSongsPopup_)) {
        
        // --- 1. Top Controls: Search & Browse ---
        ImGui::BeginGroup();
        {
            char buffer[256];
            strncpy(buffer, searchQuery_.c_str(), sizeof(buffer));
            buffer[sizeof(buffer) - 1] = 0;
            
            ImGui::SetNextItemWidth(350);
            if (ImGui::InputTextWithHint("##search", "Search Library...", buffer, sizeof(buffer))) {
                searchQuery_ = buffer;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Browse Files...")) {
                if (fileBrowserView_) {
                    fileBrowserView_->setMode(FileBrowserView::BrowserMode::LIBRARY_ADD_AND_RETURN);
                    // Set callback to tick added files
                    fileBrowserView_->setOnFilesAddedCallback([this](const std::vector<std::string>& files) {
                        for (const auto& f : files) {
                            Logger::info("Callback: Auto-selecting " + f);
                            trackSelector_.addSelection(f);
                        }
                        // Note: we don't need to set shouldReopenAddPopup_ here anymore, 
                        // logic in renderPopups handles it when browser closes.
                    });
                    fileBrowserView_->show();
                    
                    // Do NOT close this popup, instead open nested popup for File Browser
                    ImGui::OpenPopup("File Browser");
                    
                    isBrowsingForPlaylist_ = true;
                }
            }
        }
        ImGui::EndGroup();
        
        ImGui::Separator();
        
        // --- 2. Track List Table ---
        ImVec2 available = ImGui::GetContentRegionAvail();
        float listHeight = available.y - 60; // Leave space for buttons, optimize usage
        
        // Retrieve all tracks
        auto* lib = playlistController_->getLibrary();
        if (lib) {
            auto allTracks = lib->getAll();
            
            // Filter and Convert to FileInfo
            std::vector<FileInfo> displayTracks;
            
            std::string queryLower = searchQuery_;
            std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
            
            for (const auto& t : allTracks) {
                std::string title = t->getDisplayName();
                std::string artist = t->getMetadata().artist;
                
                // Search logic
                if (!searchQuery_.empty()) {
                    std::string titleLower = title;
                    std::string artistLower = artist;
                    std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);
                    std::transform(artistLower.begin(), artistLower.end(), artistLower.begin(), ::tolower);
                    
                    if (titleLower.find(queryLower) == std::string::npos && artistLower.find(queryLower) == std::string::npos) {
                        continue;
                    }
                }
                
                FileInfo info;
                info.name = title;
                info.extension = artist; // Mapping Artist to "Type/Extension" column
                info.path = t->getPath();
                info.isDirectory = false;
                info.size = 0;
                
                displayTracks.push_back(info);
            }
            
            // Update selector items
            // Note: This sets items every frame is inefficient if data hasn't changed.
            // Ideally should check dirty flag or only set when search changes.
            // For now, let's optimize slightly by checking if sizes match or similar, 
            // but effectively we might just set it. 
            // Better: update only when popup opens or search changes.
            // But since this is immediate mode, we'll just set it for now.
            // Optimization: Static or member cache? 
            // Let's rely on basic set for now, PagedFileSelector is light.
            
            trackSelector_.setItems(displayTracks);
            trackSelector_.setCustomLabels("Title", "Artist");
            
            // Render
            
            // Actions
            trackSelector_.renderActions();
            
            if (ImGui::BeginChild("TrackList", ImVec2(0, listHeight - 40), true)) {
                trackSelector_.renderList();
            }
            ImGui::EndChild();
            
            trackSelector_.renderPagination();
        }
        
        // --- 3. Bottom Buttons ---
        ImGui::Separator();
        
        // Helper lambda
        auto addTracks = [&](const std::vector<std::string>& paths) {
            if (paths.empty()) return;
            if (!selectedPlaylist_) return;

            int count = 0;
            for (const auto& path : paths) {
                playlistController_->addToPlaylist(selectedPlaylist_->getName(), path);
                count++;
            }
            if (count > 0) Logger::info("Added " + std::to_string(count) + " tracks.");
            
            ImGui::CloseCurrentPopup();
            showAddSongsPopup_ = false;
        };

        if (ImGui::Button("Add Selected", ImVec2(120, 0))) {
            addTracks(trackSelector_.getSelectedPaths());
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add Random 20", ImVec2(120, 0))) {
            trackSelector_.selectRandom(20);
            addTracks(trackSelector_.getSelectedPaths());
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            showAddSongsPopup_ = false;
        }

        // Render File Browser Nested Popup if active
        if (isBrowsingForPlaylist_ && fileBrowserView_) {
            fileBrowserView_->renderPopup();
        }
        
        ImGui::EndPopup();
    }
}
