#include "../../../inc/app/view/MainWindow.h"
#include "../../../inc/utils/Logger.h"
#include "../../../inc/app/controller/PlaybackController.h"
#include <sstream>
#include <cstring>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

#include <stb_image.h>

#ifdef USE_SDL2
#include <SDL2/SDL_opengl.h>
#endif

// Color scheme
// Color scheme - Swapped (Black Window, Teal Panels)
static const ImVec4 COLOR_BG_BLACK = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);      // #000000 (Pure Black)
static const ImVec4 COLOR_BG_TEAL = ImVec4(0.11f, 0.40f, 0.35f, 1.0f);       // #1C6758 (Original Teal)
static const ImVec4 COLOR_ACCENT = ImVec4(0.94f, 0.35f, 0.49f, 1.0f);        // #F05A7E
static const ImVec4 COLOR_TEXT = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);          
static const ImVec4 COLOR_TEXT_DIM = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);      

MainWindow::MainWindow() {
    Logger::getInstance().info("MainWindow created");
}

void MainWindow::render() {
    if (!isVisible()) {
        return;
    }
    
#ifdef USE_IMGUI
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    
    // Set up fullscreen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | 
                                   ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse; // Fix mouse scrolling
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BG_BLACK); // Main background black
    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT);
    
    if (ImGui::Begin("MainWindow", nullptr, windowFlags)) { // Begin MainWindow
        // Tab bar at top
        renderTabBar();
        
        ImGui::Separator();
        
        // Calculate layout sizes - Album art takes 75%, track list takes 25%
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float availableHeight = ImGui::GetContentRegionAvail().y - 120.0f; // Reserve for controls
        float albumPanelWidth = availableWidth * 0.75f;  // Increased to 75%
        float trackListWidth = availableWidth * 0.24f;   // Reduced to 24% (keep some margin)
        
        // Main content area - Force no scrollbar
        // Main content area - Force no scrollbar
        ImGui::BeginChild("ContentArea", ImVec2(0, availableHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            // Left side: Track list (25% of width) - TEAL Background
            ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
            ImGui::BeginChild("LeftPanel", ImVec2(trackListWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            renderTrackList();
            ImGui::EndChild();
            ImGui::PopStyleColor(); // Pop ChildBg
            
            ImGui::SameLine();
            
            // Right side: Album art (75% of width) - Keep Black
            ImGui::BeginChild("RightPanel", ImVec2(albumPanelWidth, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            renderAlbumArt();
            ImGui::EndChild();
        }
        ImGui::EndChild();
        
        // Bottom: Playback controls
        renderPlaybackControls();
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2);
    
    // Render file browser if open
    if (fileBrowserView_) fileBrowserView_->render();
#endif
}

void MainWindow::renderTabBar() {
#ifdef USE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_Button, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.5f, 1.0f)); // Lighter teal
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, COLOR_ACCENT);
    
    float buttonWidth = 100.0f;
    float buttonHeight = 30.0f;
    
    // History button
    bool isHistoryActive = (currentScreen_ == Screen::HISTORY);
    if (isHistoryActive) ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ACCENT);
    if (ImGui::Button("History", ImVec2(buttonWidth, buttonHeight))) {
        switchScreen(Screen::HISTORY);
    }
    if (isHistoryActive) ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Playlist button
    bool isPlaylistActive = (currentScreen_ == Screen::PLAYLIST);
    if (isPlaylistActive) ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ACCENT);
    if (ImGui::Button("Playlist", ImVec2(buttonWidth, buttonHeight))) {
        switchScreen(Screen::PLAYLIST);
    }
    if (isPlaylistActive) ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Library button
    bool isLibraryActive = (currentScreen_ == Screen::LIBRARY);
    if (isLibraryActive) ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ACCENT);
    if (ImGui::Button("Library", ImVec2(buttonWidth, buttonHeight))) {
        switchScreen(Screen::LIBRARY);
    }
    if (isLibraryActive) ImGui::PopStyleColor();
    
    // Now Playing info on the right
    if (playbackState_ && playbackState_->getCurrentTrack()) {
        auto track = playbackState_->getCurrentTrack();
        std::string nowPlayingText = "Now Playing: " + track->getDisplayName();
        float textWidth = ImGui::CalcTextSize(nowPlayingText.c_str()).x;
        ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - 20);
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ACCENT);
        ImGui::Text("%s", nowPlayingText.c_str());
        ImGui::PopStyleColor();
    }
    
    ImGui::PopStyleColor(3);
#endif
}

void MainWindow::renderNowPlayingInfo() {
#ifdef USE_IMGUI
    if (playbackState_ && playbackState_->getCurrentTrack()) {
        auto track = playbackState_->getCurrentTrack();
        const auto& meta = track->getMetadata();
        
        // Title (large)
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 0 ? ImGui::GetIO().Fonts->Fonts[0] : nullptr);
        ImGui::TextWrapped("%s", track->getDisplayName().c_str());
        ImGui::PopFont();
        
        // Artist - Album (smaller, dimmed)
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT_DIM);
        std::string subtitle;
        if (!meta.artist.empty()) subtitle = meta.artist;
        if (!meta.album.empty()) {
            if (!subtitle.empty()) subtitle += " - ";
            subtitle += meta.album;
        }
        if (!subtitle.empty()) {
            ImGui::TextWrapped("%s", subtitle.c_str());
        }
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
    } else {
        ImGui::Text("No track playing");
        ImGui::Spacing();
    }
#endif
}

void MainWindow::renderAlbumArt() {
#ifdef USE_IMGUI
    // Use 85% of available width for album art/video, no hard cap
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float albumSize = availableWidth * 0.85f;
    // if (albumSize > 500) albumSize = 500; // Max size cap removed per user request
    
    // Check for video texture first
    void* videoTexture = nullptr;
    if (playbackController_ && playbackController_->getEngine()) {
        videoTexture = playbackController_->getEngine()->getVideoTexture();
    }

    if (videoTexture) {
        // Render Video
        float aspectRatio = 16.0f / 9.0f; // Default fallback
        
        int vW = 0, vH = 0;
        playbackController_->getEngine()->getVideoSize(vW, vH);
        if (vW > 0 && vH > 0) {
            aspectRatio = (float)vW / (float)vH;
        }
        
        float videoHeight = albumSize / aspectRatio;
        
        // Constrain height to avoid scrolling if video is too tall
        float maxHeight = ImGui::GetContentRegionAvail().y - 50.0f; // Leave space for info
        if (videoHeight > maxHeight && maxHeight > 50) {
            videoHeight = maxHeight;
            albumSize = videoHeight * aspectRatio; // Adjust width to maintain aspect ratio
        }
        
        // Center video
        float startX = (availableWidth - albumSize) / 2;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
        
        ImGui::Image((ImTextureID)videoTexture, ImVec2(albumSize, videoHeight));
    } else if (playbackState_ && playbackState_->getCurrentTrack()) {
        auto track = playbackState_->getCurrentTrack();
        const auto& meta = track->getMetadata();
        
        // Check if track changed
        if (currentTrackPath_ != track->getPath()) {
            currentTrackPath_ = track->getPath();
            
            // Cleanup old texture
            if (albumArtTexture_) {
                glDeleteTextures(1, &albumArtTexture_);
                albumArtTexture_ = 0;
            }
            
            // Load new album art
            if (meta.hasAlbumArt && !meta.albumArtData.empty()) {
                int width, height, channels;
                unsigned char* imageData = stbi_load_from_memory(
                    meta.albumArtData.data(),
                    static_cast<int>(meta.albumArtData.size()),
                    &width, &height, &channels, 4);
                
                if (imageData) {
                    glGenTextures(1, &albumArtTexture_);
                    glBindTexture(GL_TEXTURE_2D, albumArtTexture_);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
                    stbi_image_free(imageData);
                }
            }
        }
        
        // Display album art
        float startX = (availableWidth - albumSize) / 2;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

        if (albumArtTexture_) {
            ImGui::Image((ImTextureID)(intptr_t)albumArtTexture_, ImVec2(albumSize, albumSize));
        } else {
            // Placeholder
            ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
            ImGui::BeginChild("AlbumArtPlaceholder", ImVec2(albumSize, albumSize), true);
            ImGui::SetCursorPos(ImVec2(albumSize/2 - 30, albumSize/2 - 10));
            ImGui::Text("[No Art]");
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    } else {
        // No track - placeholder
        float startX = (availableWidth - albumSize) / 2;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
        ImGui::BeginChild("AlbumArtPlaceholder", ImVec2(albumSize, albumSize), true);
        ImGui::SetCursorPos(ImVec2(albumSize/2 - 30, albumSize/2 - 10));
        ImGui::Text("[No Art]");
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
#endif
}

void MainWindow::renderTrackList() {
#ifdef USE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_Header, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COLOR_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, COLOR_ACCENT);
    
    std::string currentPlayingPath = "";
    if (playbackState_ && playbackState_->getCurrentTrack()) {
        currentPlayingPath = playbackState_->getCurrentTrack()->getPath();
    }
    
    switch (currentScreen_) {
        case Screen::LIBRARY:
            if (libraryView_ && libraryView_->getLibrary()) {
                auto files = libraryView_->getLibrary()->getAll();
                
                // Search bar
                static char searchBuffer[256] = "";
                ImGui::PushStyleColor(ImGuiCol_FrameBg, COLOR_BG_TEAL);
                if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
                    // Filter handled below
                }
                ImGui::PopStyleColor();
                
                std::string searchQuery(searchBuffer);
                if (!searchQuery.empty()) {
                    files = libraryView_->getLibrary()->search(searchQuery);
                }
                
                // Buttons FIRST (outside scroll)
                if (ImGui::Button("Add Files")) {
                    if (fileBrowserView_) {
                        fileBrowserView_->setMode(FileBrowserView::BrowserMode::LIBRARY);
                        fileBrowserView_->show();
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear Library")) {
                    if (libraryView_ && libraryView_->getLibrary()) {
                        libraryView_->getLibrary()->clear();
                    }
                }
                
                ImGui::SameLine();
                
                // Loop button (Moved to Library Header)
                if (playbackController_ && playbackController_->getCurrentPlaylist()) {
                    auto currentPlaylist = playbackController_->getCurrentPlaylist();
                    RepeatMode mode = currentPlaylist->getRepeatMode();
                    bool isAll = (mode == RepeatMode::ALL);
                    
                    // Visual feedback
                    if (isAll) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));  // Green when ALL
                    } else if (mode == RepeatMode::ONE) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.1f, 1.0f));  // Orange when ONE (indication)
                    }
                    
                    const char* label = isAll ? "Loop: ALL" : (mode == RepeatMode::ONE ? "Loop: ONE" : "Loop: OFF");
                    
                    if (ImGui::Button(label)) { 
                         // Toggle ALL <-> NONE (Override ONE if set)
                         if (isAll)
                             playbackController_->setRepeatMode(RepeatMode::NONE);
                         else
                             playbackController_->setRepeatMode(RepeatMode::ALL);
                    }
                    
                    if (isAll || mode == RepeatMode::ONE) {
                        ImGui::PopStyleColor();
                    }
                }
                
                ImGui::Text("Library: %zu tracks", files.size());
                ImGui::Separator();
                
                // Calculate remaining height for scroll area
                float scrollHeight = ImGui::GetContentRegionAvail().y;
                ImGui::BeginChild("TrackListScroll", ImVec2(0, scrollHeight), true);
                
                for (size_t i = 0; i < files.size(); ++i) {
                    const auto& file = files[i];
                    const auto& meta = file->getMetadata();
                    bool isPlaying = (file->getPath() == currentPlayingPath);
                    
                    ImGui::PushID(static_cast<int>(i));
                    
                    // Two-line display with full-width selectable
                    float itemHeight = 45.0f;
                    std::string title = file->getDisplayName();
                    std::string subtitle = meta.artist;
                    if (!meta.album.empty()) {
                        if (!subtitle.empty()) subtitle += " - ";
                        subtitle += meta.album;
                    }
                    
                    // Full width selectable with highlight
                    if (isPlaying) {
                        ImGui::PushStyleColor(ImGuiCol_Header, COLOR_ACCENT);
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COLOR_ACCENT);
                    }
                    
                    bool clicked = ImGui::Selectable("##track", isPlaying, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, itemHeight));
                    
                    // Draw text over selectable
                    ImVec2 itemPos = ImGui::GetItemRectMin();
                    ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10, itemPos.y + 5));
                    ImGui::Text("%s", title.c_str());
                    ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10, itemPos.y + 24));
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT_DIM);
                    ImGui::Text("%s", subtitle.c_str());
                    ImGui::PopStyleColor();
                    
                    if (isPlaying) {
                        ImGui::PopStyleColor(2);
                    }
                    
                    // Single click to play
                    if (clicked && playbackController_) {
                        // Use Now Playing playlist for library mode
                        if (playlistView_ && playlistView_->getManager()) {
                            auto nowPlaying = playlistView_->getManager()->getNowPlayingPlaylist();
                            nowPlaying->clear();
                            for (const auto& file : files) {
                                nowPlaying->addTrack(file);
                            }
                            playbackController_->setCurrentPlaylist(nowPlaying.get());
                            playbackController_->play(files[i]);
                        }
                    }
                    
                    // Scroll to current
                    if (isPlaying && ImGui::IsWindowAppearing()) {
                        ImGui::SetScrollHereY(0.5f);
                    }
                    
                    ImGui::PopID();
                }
                
                ImGui::EndChild();
            }
            break;
            
        case Screen::PLAYLIST:
            if (playlistView_ && playlistView_->getManager()) {
                auto playlists = playlistView_->getManager()->getAllPlaylists();
                static int selectedPlaylistIdx = 0;
                
                // Playlist selector
                ImGui::Text("Playlists:");
                ImGui::SameLine();
                static char newPlaylistName[64] = "";
                static bool showNewPlaylistPopup = false;
                if (ImGui::Button("+")) {
                    showNewPlaylistPopup = true;
                    ImGui::OpenPopup("New Playlist");
                }
                
                if (ImGui::BeginPopup("New Playlist")) {
                    ImGui::InputText("Name", newPlaylistName, sizeof(newPlaylistName));
                    if (ImGui::Button("Create") && strlen(newPlaylistName) > 0) {
                        playlistView_->getManager()->createPlaylist(newPlaylistName);
                        newPlaylistName[0] = '\0';
                        ImGui::CloseCurrentPopup();
                    }
                    if (ImGui::Button("Cancel")) {
                        newPlaylistName[0] = '\0';
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                
                ImGui::BeginChild("PlaylistSelector", ImVec2(0, 50), true);
                for (size_t i = 0; i < playlists.size(); ++i) {
                    bool isSelected = (static_cast<int>(i) == selectedPlaylistIdx);
                    if (ImGui::Selectable(playlists[i]->getName().c_str(), isSelected, 0, ImVec2(100, 25))) {
                        selectedPlaylistIdx = static_cast<int>(i);
                    }
                    ImGui::SameLine();
                }
                ImGui::EndChild();
                
                // Tracks in selected playlist
                if (selectedPlaylistIdx < static_cast<int>(playlists.size())) {
                    auto playlist = playlists[selectedPlaylistIdx];
                    auto tracks = playlist->getTracks();
                    
                    // Buttons FIRST
                    if (ImGui::Button("Add Files")) {
                        if (fileBrowserView_) {
                            fileBrowserView_->setMode(FileBrowserView::BrowserMode::PLAYLIST_SELECTION);
                            fileBrowserView_->setTargetPlaylist(playlist->getName());
                            fileBrowserView_->show();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear")) {
                        playlist->clear();
                    }
                    
                    ImGui::Text("%s: %zu tracks", playlist->getName().c_str(), tracks.size());
                    ImGui::Separator();
                    
                    float scrollHeight = ImGui::GetContentRegionAvail().y;
                    ImGui::BeginChild("TrackListScroll", ImVec2(0, scrollHeight), true);
                    
                    for (size_t i = 0; i < tracks.size(); ++i) {
                        const auto& track = tracks[i];
                        const auto& meta = track->getMetadata();
                        bool isPlaying = (track->getPath() == currentPlayingPath);
                        
                        ImGui::PushID(static_cast<int>(i) + 10000);
                        
                        float itemHeight = 45.0f;
                        std::string title = track->getDisplayName();
                        std::string subtitle = meta.artist;
                        if (!meta.album.empty()) {
                            if (!subtitle.empty()) subtitle += " - ";
                            subtitle += meta.album;
                        }
                        
                        if (isPlaying) {
                            ImGui::PushStyleColor(ImGuiCol_Header, COLOR_ACCENT);
                            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COLOR_ACCENT);
                        }
                        
                        bool clicked = ImGui::Selectable("##track", isPlaying, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, itemHeight));
                        
                        ImVec2 itemPos = ImGui::GetItemRectMin();
                        ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10, itemPos.y + 5));
                        ImGui::Text("%s", title.c_str());
                        ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10, itemPos.y + 24));
                        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT_DIM);
                        ImGui::Text("%s", subtitle.c_str());
                        ImGui::PopStyleColor();
                        
                        if (isPlaying) {
                            ImGui::PopStyleColor(2);
                        }
                        
                        if (clicked && playbackController_) {
                            playbackController_->setCurrentPlaylist(playlist.get());
                            playbackController_->play(track);
                        }
                        
                        if (isPlaying && ImGui::IsWindowAppearing()) {
                            ImGui::SetScrollHereY(0.5f);
                        }
                        
                        ImGui::PopID();
                    }
                    
                    ImGui::EndChild();
                }
            }
            break;
            
        case Screen::HISTORY:
            if (historyView_ && historyView_->getHistory()) {
                auto historyTracks = historyView_->getHistory()->getAll();
                
                // Button FIRST
                if (ImGui::Button("Clear History")) {
                    historyView_->getHistory()->clear();
                }
                
                ImGui::Text("History: %zu tracks", historyTracks.size());
                ImGui::Separator();
                
                float scrollHeight = ImGui::GetContentRegionAvail().y;
                ImGui::BeginChild("TrackListScroll", ImVec2(0, scrollHeight), true);
                
                for (size_t i = 0; i < historyTracks.size(); ++i) {
                    const auto& track = historyTracks[i];
                    const auto& meta = track->getMetadata();
                    bool isPlaying = (track->getPath() == currentPlayingPath);
                    
                    ImGui::PushID(static_cast<int>(i) + 20000);
                    
                    float itemHeight = 45.0f;
                    std::string title = track->getDisplayName();
                    std::string subtitle = meta.artist;
                    if (!meta.album.empty()) {
                        if (!subtitle.empty()) subtitle += " - ";
                        subtitle += meta.album;
                    }
                    
                    if (isPlaying) {
                        ImGui::PushStyleColor(ImGuiCol_Header, COLOR_ACCENT);
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, COLOR_ACCENT);
                    }
                    
                    bool clicked = ImGui::Selectable("##track", isPlaying, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, itemHeight));
                    
                    ImVec2 itemPos = ImGui::GetItemRectMin();
                    ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10, itemPos.y + 5));
                    ImGui::Text("%s", title.c_str());
                    ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10, itemPos.y + 24));
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT_DIM);
                    ImGui::Text("%s", subtitle.c_str());
                    ImGui::PopStyleColor();
                    
                    if (isPlaying) {
                        ImGui::PopStyleColor(2);
                    }
                    
                    if (clicked && playbackController_) {
                        // Use Now Playing playlist for history mode
                        if (playlistView_ && playlistView_->getManager()) {
                            auto nowPlaying = playlistView_->getManager()->getNowPlayingPlaylist();
                            nowPlaying->clear();
                            for (const auto& track : historyTracks) {
                                nowPlaying->addTrack(track);
                            }
                            playbackController_->setCurrentPlaylist(nowPlaying.get());
                            playbackController_->play(track);
                        }
                    }
                    
                    if (isPlaying && ImGui::IsWindowAppearing()) {
                        ImGui::SetScrollHereY(0.5f);
                    }
                    
                    ImGui::PopID();
                }
                
                ImGui::EndChild();
            }
            break;
    }
    
    ImGui::PopStyleColor(4);
#endif
}

void MainWindow::renderPlaybackControls() {
#ifdef USE_IMGUI
    ImGui::Separator();
    
    // Style settings
    ImGui::PushStyleColor(ImGuiCol_Button, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COLOR_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); 
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); 
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, COLOR_BG_TEAL);
    
    // --- ROW 1: Progress Bar ---
    if (playbackState_) {
        double position = playbackState_->getPosition();
        double duration = playbackState_->getDuration();
        
        int posMin = static_cast<int>(position) / 60;
        int posSec = static_cast<int>(position) % 60;
        int durMin = static_cast<int>(duration) / 60;
        int durSec = static_cast<int>(duration) % 60;
        
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%02d:%02d", posMin, posSec);
        ImGui::SameLine();
        
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float timeTextWidth = 50.0f; 
        ImGui::PushItemWidth(availableWidth - timeTextWidth * 1.5f); 
        
        float seekPos = static_cast<float>(position);
        if (ImGui::SliderFloat("##seek", &seekPos, 0.0f, static_cast<float>(duration), "")) {
            if (playbackController_) playbackController_->seek(seekPos);
        }
        ImGui::PopItemWidth();
        
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%02d:%02d", durMin, durSec);
    }
    
    ImGui::Spacing();
    
    // --- ROW 2: Info | Controls | Volume ---
    ImGui::Columns(3, "PlaybackColumns", false); 
    
    // Column 1: Now Playing Info (Left) + Advanced Controls
    if (playbackState_ && playbackState_->getCurrentTrack()) {
        auto track = playbackState_->getCurrentTrack();
        const auto& meta = track->getMetadata();
        
        float colWidth = ImGui::GetColumnWidth();
        float buttonsWidth = 60.0f; // Approx for 2 buttons + spacing
        
        // Max widths
        float maxTitleWidth = colWidth - buttonsWidth - 20.0f; 
        float maxArtistWidth = colWidth - 20.0f;
        
        // Helper to truncate text
        auto truncate = [&](const std::string& text, float maxWidth) -> std::string {
            if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) return text;
            std::string s = text;
            while (s.length() > 3 && ImGui::CalcTextSize((s + "...").c_str()).x > maxWidth) {
                s.pop_back();
            }
            return s + "...";
        };

        // Line 1: Title + Buttons
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", truncate(track->getDisplayName(), maxTitleWidth).c_str());
        
        ImGui::SameLine();
        
        // --- Buttons (+) and (:) ---
        float iconBtnSize = 24.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        
        // (+) Add to Playlist
        if (ImGui::Button("+", ImVec2(iconBtnSize, iconBtnSize))) {
            ImGui::OpenPopup("AddToPlaylistPopup");
        }
        
        // Popup logic... (retained, but ensuring it's efficient)
        if (ImGui::BeginPopup("AddToPlaylistPopup")) {
             ImGui::Text("Add to Playlist");
             ImGui::Separator();
             if (playlistView_ && playlistView_->getManager()) {
                 auto playlists = playlistView_->getManager()->getAllPlaylists();
                 bool found = false;
                 for (const auto& playlist : playlists) {
                     if (playlist->getName() == "Now Playing") continue;
                     found = true;
                     if (ImGui::Selectable(playlist->getName().c_str())) {
                         playlist->addTrack(track);
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
        
        ImGui::SameLine();
        
        // (:) Metadata
        if (ImGui::Button(":", ImVec2(iconBtnSize, iconBtnSize))) {
            ImGui::OpenPopup("MetadataPopup");
        }
        
        if (ImGui::BeginPopup("MetadataPopup")) {
            ImGui::Text("Track Details");
            ImGui::Separator();
            ImGui::Text("File: %s", track->getDisplayName().c_str());
            ImGui::Text("Codec: %s", meta.codec.c_str());
            ImGui::Text("Bitrate: %d kbps", meta.bitrate);
            ImGui::Text("Sample Rate: %d Hz", meta.sampleRate);
            ImGui::Text("Channels: %d", meta.channels);
            if (meta.year > 0) ImGui::Text("Year: %d", meta.year);
            if (!meta.genre.empty()) ImGui::Text("Genre: %s", meta.genre.c_str());
            ImGui::EndPopup();
        }
        
        ImGui::PopStyleVar();
        
        // Line 2: Artist
        // Manually move cursor down to ensure it's on a new line distinct from the buttons
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f); 
        
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT_DIM);
        if (!meta.artist.empty()) {
            ImGui::Text("%s", truncate(meta.artist, maxArtistWidth).c_str());
        } else {
            ImGui::Text("Unknown Artist"); // Placeholder to ensure layout stability
        }
        ImGui::PopStyleColor();
    }
    
    ImGui::NextColumn();
    
    // Column 2: Playback Controls (Center)
    float buttonWidth = 35.0f;
    float buttonHeight = 35.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float totalButtonsWidth = (buttonWidth * 4) + (spacing * 3);
    
    float colWidth = ImGui::GetColumnWidth();
    if (colWidth > totalButtonsWidth) {
        float startX = (colWidth - totalButtonsWidth) / 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
    }
    
    if (ImGui::Button("<<", ImVec2(buttonWidth, buttonHeight))) {
        if (playbackController_) playbackController_->previous();
    }
    ImGui::SameLine();
    
    bool isPlaying = false;
    const char* playIcon = ">";
    if (playbackState_) {
         auto status = playbackState_->getStatus();
         if (status == PlaybackStatus::PLAYING) {
             isPlaying = true;
             playIcon = "||";
         }
    }
    
    if (ImGui::Button(playIcon, ImVec2(buttonWidth, buttonHeight))) {
         if (playbackController_) {
             if (isPlaying) playbackController_->pause();
             else if (playbackState_ && playbackState_->getStatus() == PlaybackStatus::PAUSED) playbackController_->resume();
             else if (playbackState_) playbackController_->play(playbackState_->getCurrentTrack());
         }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button(">>", ImVec2(buttonWidth, buttonHeight))) {
        if (playbackController_) playbackController_->next();
    }
    
    ImGui::SameLine();
    
    // Repeat One
    bool isOne = false;
    if (playbackController_ && playbackController_->getCurrentPlaylist()) {
        isOne = (playbackController_->getCurrentPlaylist()->getRepeatMode() == RepeatMode::ONE);
    }
    
    if (isOne) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    if (ImGui::Button("1", ImVec2(buttonWidth, buttonHeight))) {
         if (playbackController_) {
             RepeatMode mode = isOne ? RepeatMode::NONE : RepeatMode::ONE;
             playbackController_->setRepeatMode(mode);
         }
    }
    if (isOne) ImGui::PopStyleColor();
    
    ImGui::NextColumn();
    
    // Column 3: Volume (Right)
    float volumeWidth = 120.0f;
    float volTotalWidth = volumeWidth + 30.0f;
    
    float volColWidth = ImGui::GetColumnWidth();
    // Align right
    if (volColWidth > volTotalWidth) {
         float volStartX = volColWidth - volTotalWidth - 10.0f;
         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + volStartX);
    }
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Vol");
    ImGui::SameLine();
    
    if (playbackState_) {
        float volume = playbackState_->getVolume();
        ImGui::PushItemWidth(volumeWidth);
        if (ImGui::SliderFloat("##volume", &volume, 0.0f, 1.0f, "")) {
            if (playbackController_) playbackController_->setVolume(volume);
        }
        ImGui::PopItemWidth();
    }
    
    ImGui::Columns(1);
    ImGui::PopStyleColor(6);
#endif
}

void MainWindow::switchScreen(Screen screen) {
    currentScreen_ = screen;
    Logger::getInstance().info("Switched to screen: " + std::to_string(static_cast<int>(screen)));
}

void MainWindow::scrollToCurrentTrack() {
    // Handled in renderTrackList via ImGui::SetScrollHereY
}

void MainWindow::handleInput() {
    // Input handled through ImGui
}
