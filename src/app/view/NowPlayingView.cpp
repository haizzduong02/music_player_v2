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

    // Load icons
    playTexture_ = loadIconTexture("assets/icons/play.tga");
    pauseTexture_ = loadIconTexture("assets/icons/pause.tga");
    nextTexture_ = loadIconTexture("assets/icons/next.tga");
    prevTexture_ = loadIconTexture("assets/icons/prev.tga");
    heartFilledTexture_ = loadIconTexture("assets/icons/heart_filled.tga");
    heartOutlineTexture_ = loadIconTexture("assets/icons/heart_outline.tga");
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
    
    // Cleanup icons
    if (playTexture_) glDeleteTextures(1, &playTexture_);
    if (pauseTexture_) glDeleteTextures(1, &pauseTexture_);
    if (nextTexture_) glDeleteTextures(1, &nextTexture_);
    if (prevTexture_) glDeleteTextures(1, &prevTexture_);
    if (heartFilledTexture_) glDeleteTextures(1, &heartFilledTexture_);
    if (heartOutlineTexture_) glDeleteTextures(1, &heartOutlineTexture_);
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
        
        // Heart button removed from here, moved to controls section
        
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
    float buttonWidth = 32.0f;
    float buttonHeight = 32.0f;
    float loopBtnWidth = 60.0f; // Narrower since it's 2 lines
    float loopBtnHeight = 32.0f; // Taller for 2 lines
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float totalButtonsWidth = (buttonWidth * 4) + loopBtnWidth + (spacing * 4);
    
    float colWidth = ImGui::GetColumnWidth();
    if (colWidth > totalButtonsWidth) {
        float startX = (colWidth - totalButtonsWidth) / 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
    }
    
    float baseLineY = ImGui::GetCursorPosY();
    float iconOffset = (loopBtnHeight - buttonHeight) / 2.0f;
    
    // Standardize height for all icons in this row
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
    
    auto set_icon_pos = [&]() {
        ImGui::SetCursorPosY(baseLineY + iconOffset);
    };

    // Favorites toggle (move to front)
    if (state_ && playlistManager_) {
        set_icon_pos();
        auto track = state_->getCurrentTrack();
        if (track) {
            auto favPlaylist = playlistManager_->getPlaylist(PlaylistManager::FAVORITES_PLAYLIST_NAME);
            if (favPlaylist) {
                bool isFavorite = favPlaylist->contains(track->getPath());
                unsigned int iconTex = isFavorite ? heartFilledTexture_ : heartOutlineTexture_;
                
                if (ImGui::ImageButton("##fav_controls", (ImTextureID)(intptr_t)iconTex, ImVec2(buttonWidth, buttonHeight))) {
                    if (isFavorite) {
                        favPlaylist->removeTrackByPath(track->getPath());
                    } else {
                        favPlaylist->addTrack(track);
                    }
                    favPlaylist->save();
                }
                ImGui::SameLine();
            }
        }
    }

    set_icon_pos();
    if (ImGui::ImageButton("##prev", (ImTextureID)(intptr_t)prevTexture_, ImVec2(buttonWidth, buttonHeight))) {
        if (controller_) controller_->previous();
    }
    ImGui::SameLine();
    
    bool isPlaying = false;
    unsigned int playIconTex = playTexture_;
    if (state_) {
         auto status = state_->getStatus();
         if (status == PlaybackStatus::PLAYING) {
             isPlaying = true;
             playIconTex = pauseTexture_;
         }
    }
    
    set_icon_pos();
    if (ImGui::ImageButton("##play", (ImTextureID)(intptr_t)playIconTex, ImVec2(buttonWidth, buttonHeight))) {
         if (controller_) {
             if (isPlaying) controller_->pause();
             else if (state_ && state_->getStatus() == PlaybackStatus::PAUSED) controller_->resume();
             else if (state_) controller_->play(state_->getCurrentTrack());
         }
    }
    
    ImGui::SameLine();
    
    set_icon_pos();
    if (ImGui::ImageButton("##next", (ImTextureID)(intptr_t)nextTexture_, ImVec2(buttonWidth, buttonHeight))) {
        if (controller_) controller_->next();
    }
    
    ImGui::SameLine();
    
    // Repeat Toggle (2-line text)
    if (controller_) {
        RepeatMode mode = controller_->getRepeatMode();
        
        char label[32];
        ImVec4 color = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
        if (mode == RepeatMode::ALL) {
            snprintf(label, sizeof(label), "LOOP\nALL");
            color = ImVec4(0.0f, 0.7f, 0.6f, 1.0f); // Teal
        } else if (mode == RepeatMode::ONE) {
            snprintf(label, sizeof(label), "LOOP\nONE");
            color = ImVec4(0.8f, 0.5f, 0.0f, 1.0f); // Orange
        } else {
            snprintf(label, sizeof(label), "LOOP\nOFF");
        }

        ImGui::SetCursorPosY(baseLineY); // Align Loop to the top (or center, but 2 lines fill loopBtnHeight)
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        if (ImGui::Button(label, ImVec2(loopBtnWidth, loopBtnHeight))) { 
             controller_->toggleRepeatMode();
        }
        ImGui::PopStyleColor();
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Current Mode: %s", (mode == RepeatMode::ALL ? "ALL" : (mode == RepeatMode::ONE ? "ONE" : "OFF")));
        }
    }
    
    ImGui::PopStyleVar(); // Pop standardized FramePadding
    
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

unsigned int NowPlayingView::loadIconTexture(const std::string& path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        Logger::getInstance().error("Failed to load icon: " + path);
        return 0;
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Use linear filtering for pixel art to keep it clean or point filtering for crispness
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    stbi_image_free(data);
    return texture;
}
