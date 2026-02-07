#include "../../../inc/app/view/NowPlayingView.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef USE_SDL2
#include <SDL2/SDL_opengl.h>
#endif

NowPlayingView::NowPlayingView(PlaybackController* controller, PlaybackState* state)
    : controller_(controller), state_(state) {
    
    // Attach as observer to playback state
    if (state_) {
        state_->attach(this);
    }
}

NowPlayingView::~NowPlayingView() {
    // Detach from state
    if (state_) {
        state_->detach(this);
    }
    
    // Cleanup texture
    if (albumArtTexture_) {
        glDeleteTextures(1, &albumArtTexture_);
    }
}

// Color scheme (Redefined for NowPlayingView)
static const ImVec4 COLOR_BG_TEAL = ImVec4(0.11f, 0.40f, 0.35f, 1.0f);       // #1C6758
static const ImVec4 COLOR_ACCENT = ImVec4(0.94f, 0.35f, 0.49f, 1.0f);        // #F05A7E
static const ImVec4 COLOR_TEXT_DIM = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);      

void NowPlayingView::render() {
    // This is now an embedded view, so no Begin/End window unless we are ensuring context
    // But since it's a child of MainWindow rendering, we just render widgets.
    // However, to ensure it looks like a distinct bar, we might use a Child window or just Separator + widgets.
    
    // Based on MainWindow logic, it starts with a Separator.
    
    ImGui::Separator();
    
    // Style settings
    ImGui::PushStyleColor(ImGuiCol_Button, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COLOR_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); 
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); 
    ImGui::PushStyleColor(ImGuiCol_FrameBg, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, COLOR_BG_TEAL);
    
    // --- ROW 1: Progress Bar ---
    if (state_) {
        double position = state_->getPosition();
        double duration = state_->getDuration();
        
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
        // Detect if user is interacting to avoid fighting with update
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 2.0f));
        if (ImGui::SliderFloat("##seek", &seekPos, 0.0f, static_cast<float>(duration), "")) {
            if (controller_) controller_->seek(seekPos);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%02d:%02d", durMin, durSec);
    }
    
    ImGui::Spacing();
    
    // --- ROW 2: Info | Controls | Volume ---
    ImGui::Columns(3, "PlaybackColumns", false); 
    
    // Column 1: Now Playing Info (Left) + Advanced Controls
    if (state_ && state_->getCurrentTrack()) {
        auto track = state_->getCurrentTrack();
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

        // Line 1: Title + Heart Button
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", truncate(track->getDisplayName(), maxTitleWidth).c_str());
        
        ImGui::SameLine();
        if (playlistManager_) {
            auto favPlaylist = playlistManager_->getPlaylist(PlaylistManager::FAVORITES_PLAYLIST_NAME);
            if (favPlaylist) {
                bool isFavorite = favPlaylist->contains(track->getPath());
                
                if (isFavorite) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); // Light Red/Pink
                }
                
                if (ImGui::Button(isFavorite ? "♥" : "♡")) {
                    if (isFavorite) {
                        favPlaylist->removeTrackByPath(track->getPath());
                        Logger::getInstance().info("Removed from Favorites: " + track->getDisplayName());
                    } else {
                        favPlaylist->addTrack(track);
                        Logger::getInstance().info("Added to Favorites: " + track->getDisplayName());
                    }
                    favPlaylist->save();
                }
                
                if (isFavorite) {
                    ImGui::PopStyleColor();
                }
            }
        }
        
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
        if (controller_) controller_->previous();
    }
    ImGui::SameLine();
    
    bool isPlaying = false;
    const char* playIcon = ">";
    if (state_) {
         auto status = state_->getStatus();
         if (status == PlaybackStatus::PLAYING) {
             isPlaying = true;
             playIcon = "||";
         }
    }
    
    if (ImGui::Button(playIcon, ImVec2(buttonWidth, buttonHeight))) {
         if (controller_) {
             if (isPlaying) controller_->pause();
             else if (state_ && state_->getStatus() == PlaybackStatus::PAUSED) controller_->resume();
             else if (state_) controller_->play(state_->getCurrentTrack());
         }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button(">>", ImVec2(buttonWidth, buttonHeight))) {
        if (controller_) controller_->next();
    }
    
    ImGui::SameLine();
    
    // Repeat One
    if (controller_) {
        // Use unified getter for both Playlist and Global/Library modes
        RepeatMode mode = controller_->getRepeatMode();
        
        // Visual feedback based on mode
        if (mode == RepeatMode::ALL) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));  // Green when ALL
        } else if (mode == RepeatMode::ONE) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.1f, 1.0f));  // Orange when ONE
        }
        
        const char* label = "Loop: OFF";
        if (mode == RepeatMode::ALL) label = "Loop: ALL";
        else if (mode == RepeatMode::ONE) label = "Loop: ONE";
        
        // Using wider button for text label
        float loopBtnWidth = 80.0f;
        
        if (ImGui::Button(label, ImVec2(loopBtnWidth, buttonHeight))) { 
             controller_->toggleRepeatMode();
        }
        
        if (mode == RepeatMode::ALL || mode == RepeatMode::ONE) {
            ImGui::PopStyleColor();
        }
    }
    
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
    
    if (state_) {
        float volume = state_->getVolume();
        ImGui::PushItemWidth(volumeWidth);
        if (ImGui::SliderFloat("##volume", &volume, 0.0f, 1.0f, "")) {
            if (controller_) controller_->setVolume(volume);
        }
        ImGui::PopItemWidth();
    }
    
    ImGui::Columns(1);
    ImGui::PopStyleColor(6);
}

void NowPlayingView::handleInput() {
    // Input handled through ImGui in render()
}

void NowPlayingView::update(void* subject) {
    // Playback state has changed
    if (state_ && state_->getStatus() == PlaybackStatus::PLAYING) {
        if (!visible_) {
            visible_ = true;
            ImGui::SetWindowFocus("Now Playing");
        }
    }
}
