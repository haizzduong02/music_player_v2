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
    if (!isVisible()) {
        return;
    }
    
    ImGui::Begin("Playlists", &visible_);
    
    auto playlists = manager_->getAllPlaylists();
    
    // Left panel: Playlist list
    ImGui::BeginChild("PlaylistList", ImVec2(200, 0), true);
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
    
    ImGui::SameLine();
    
    // Right panel: Playlist tracks
    ImGui::BeginChild("PlaylistTracks", ImVec2(0, 0), true);
    
    if (selectedPlaylist_) {
        ImGui::Text("%s", selectedPlaylist_->getName().c_str());
        ImGui::Text("Tracks: %zu", selectedPlaylist_->size());
        ImGui::Separator();
        
        auto tracks = selectedPlaylist_->getTracks();
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            bool isSelected = (static_cast<int>(i) == selectedTrackIndex_);
            
            std::string displayText = tracks[i]->getDisplayName();

            // Fix ID conflict by appending ##i
            std::string label = displayText + "##pl_" + std::to_string(i);
            
            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedTrackIndex_ = static_cast<int>(i);

                // Double-click to play
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (playbackController_) {
                        // Set context to this playlist
                        playbackController_->setCurrentPlaylist(selectedPlaylist_.get());
                        playbackController_->play(tracks[i]);
                        Logger::getInstance().info("Playing from playlist: " + displayText);
                    }
                }
            }
            
            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove")) {
                    controller_->removeFromPlaylist(selectedPlaylist_->getName(), i);
                }
                ImGui::EndPopup();
            }
        }
        
        ImGui::Separator();
        
        // Playlist controls
        if (ImGui::Button("Add Files")) {
            if (fileBrowserView_) {
                fileBrowserView_->setMode(FileBrowserView::BrowserMode::PLAYLIST_SELECTION);
                fileBrowserView_->setTargetPlaylist(selectedPlaylist_->getName());
                fileBrowserView_->show();
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Shuffle")) {
            controller_->shufflePlaylist(selectedPlaylist_->getName());
        }
        
        ImGui::SameLine();
        
        bool loopEnabled = selectedPlaylist_->isLoopEnabled();
        if (ImGui::Checkbox("Loop", &loopEnabled)) {
            controller_->setPlaylistLoop(selectedPlaylist_->getName(), loopEnabled);
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Clear")) {
            selectedPlaylist_->clear();
        }
    } else {
        ImGui::Text("Select a playlist");
    }
    
    ImGui::EndChild();
    
    ImGui::End();
    
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
    // Playlist manager changed - will re-render
}
