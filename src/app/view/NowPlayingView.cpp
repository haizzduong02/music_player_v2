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

void NowPlayingView::render() {
    if (!isVisible()) {
        return;
    }
    
    ImGui::Begin("Now Playing", &visible_);
    
    auto currentTrack = state_->getCurrentTrack();
    
    if (currentTrack) {
        const auto& meta = currentTrack->getMetadata();
        
        // Check if track changed, reload album art
        if (currentTrackPath_ != currentTrack->getPath()) {
            currentTrackPath_ = currentTrack->getPath();
            
            // Cleanup old texture
            if (albumArtTexture_) {
                glDeleteTextures(1, &albumArtTexture_);
                albumArtTexture_ = 0;
            }
            
            // Load new album art if available
            if (meta.hasAlbumArt && !meta.albumArtData.empty()) {
                int width, height, channels;
                unsigned char* imageData = stbi_load_from_memory(
                    meta.albumArtData.data(),
                    static_cast<int>(meta.albumArtData.size()),
                    &width, &height, &channels, 4);  // Force RGBA
                
                if (imageData) {
                    glGenTextures(1, &albumArtTexture_);
                    glBindTexture(GL_TEXTURE_2D, albumArtTexture_);
                    
                    // Setup filtering parameters for display
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    
                    // Upload pixels into texture
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
                    
                    stbi_image_free(imageData);
                    Logger::getInstance().info("Album art loaded: " + std::to_string(width) + "x" + std::to_string(height));
                }
            }
        }
        
        // Display album art if available, otherwise placeholder
        if (albumArtTexture_) {
            ImGui::Image((ImTextureID)(intptr_t)albumArtTexture_, ImVec2(150, 150));
            ImGui::SameLine();
        } else {
            ImGui::BeginChild("AlbumArtPlaceholder", ImVec2(154, 154), true);
            ImGui::Text("[No Art]");
            ImGui::EndChild();
            ImGui::SameLine();
        }
        
        // Begin info column
        ImGui::BeginGroup();
        
        // Display track info - consolidated header
        std::string title = currentTrack->getDisplayName();
        ImGui::TextWrapped("Title: %s", title.c_str());
        if (!meta.artist.empty()) {
            ImGui::Text("Artist: %s", meta.artist.c_str());
        }
        if (!meta.album.empty()) {
            ImGui::Text("Album: %s", meta.album.c_str());
        }
        
        // Progress bar
        double position = state_->getPosition();
        double duration = state_->getDuration();
        float progress = duration > 0 ? static_cast<float>(position / duration) : 0.0f;
        
        ImGui::ProgressBar(progress, ImVec2(200, 0));
        ImGui::Text("%.0f:%.02d / %.0f:%.02d", 
            position / 60, static_cast<int>(position) % 60,
            duration / 60, static_cast<int>(duration) % 60);
        
        ImGui::EndGroup();
        
        // Seek slider (full width)
        ImGui::Separator();
        
        // Style for better slider visibility - disable hover on frame, white thumb
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));  // No hover effect
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White thumb
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));  // Light gray when dragging
        
        float seekPos = static_cast<float>(position);
        if (ImGui::SliderFloat("##seek", &seekPos, 0.0f, static_cast<float>(duration), "")) {
            controller_->seek(seekPos);
        }
        
        ImGui::PopStyleColor(3);
        
        ImGui::Separator();
        
        // Playback controls
        auto status = state_->getStatus();
        
        // Previous button
        if (ImGui::Button("<<")) {
            controller_->previous();
        }
        
        ImGui::SameLine();
        
        // Play/Pause button
        if (status == PlaybackStatus::PLAYING) {
            if (ImGui::Button("||")) {  // Pause
                controller_->pause();
            }
        } else {
            if (ImGui::Button(">")) {  // Play
                if (status == PlaybackStatus::PAUSED) {
                    controller_->resume();
                } else {
                    controller_->play(currentTrack);
                }
            }
        }
        
        ImGui::SameLine();
        
        // Stop button
        if (ImGui::Button("[]")) {
            controller_->stop();
        }
        
        ImGui::SameLine();
        
        // Next button
        if (ImGui::Button(">>")) {
            controller_->next();
        }
        
        ImGui::SameLine();
        
        // Loop button (Now Playing Loop: ONE vs OFF)
        auto currentPlaylist = controller_->getCurrentPlaylist();
        if (currentPlaylist) {
            RepeatMode mode = currentPlaylist->getRepeatMode();
            bool isOne = (mode == RepeatMode::ONE);
            
            // Visual feedback
            if (isOne) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));  // Green when ONE
            } else if (mode == RepeatMode::ALL) {
                 ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.7f, 1.0f)); // Blue when ALL (indication)
            }
            
            const char* label = isOne ? "Repeat Track: ON" : "Repeat Track";
            
            if (ImGui::Button(label)) {
                // Toggle ONE <-> NONE
                if (isOne)
                    controller_->setRepeatMode(RepeatMode::NONE);
                else
                    controller_->setRepeatMode(RepeatMode::ONE);
            }
            
            if (isOne || mode == RepeatMode::ALL) {
                ImGui::PopStyleColor(1); // Wait, only PushButton pushed? 1 color. Previous code pushed 3 colors.
                // Let's check previous code.
            }
        }
        
        ImGui::Separator();
        
        // Volume control
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));  // No hover effect
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));  // White thumb
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));  // Light gray when dragging
        
        float volume = state_->getVolume();
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
            controller_->setVolume(volume);
        }
        
        ImGui::PopStyleColor(3);
        
    } else {
        ImGui::Text("No track loaded");
    }
    
    ImGui::End();
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
