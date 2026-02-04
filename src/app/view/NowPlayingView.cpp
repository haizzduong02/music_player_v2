#include "../../../inc/app/view/NowPlayingView.h"
#include "../../../inc/utils/Logger.h"
#include <imgui.h>

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
}

void NowPlayingView::render() {
    if (!isVisible()) {
        return;
    }
    
    ImGui::Begin("Now Playing", &visible_);
    
    auto currentTrack = state_->getCurrentTrack();
    
    if (currentTrack) {
        const auto& meta = currentTrack->getMetadata();
        
        // Album art placeholder (150x150)
        if (meta.hasAlbumArt) {
            ImGui::BeginChild("AlbumArtFrame", ImVec2(154, 154), true);
            ImGui::Text("[Album Art]");
            ImGui::Text("Embedded");
            ImGui::Text("Image");
            ImGui::EndChild();
            ImGui::SameLine();
        } else {
            ImGui::BeginChild("AlbumArtFrame", ImVec2(154, 154), true);
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
        float seekPos = static_cast<float>(position);
        if (ImGui::SliderFloat("##seek", &seekPos, 0.0f, static_cast<float>(duration), "")) {
            controller_->seek(seekPos);
        }
        
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
        
        ImGui::Separator();
        
        // Volume control
        float volume = state_->getVolume();
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
            controller_->setVolume(volume);
        }
        
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
