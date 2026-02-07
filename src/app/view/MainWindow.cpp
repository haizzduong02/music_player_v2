#include "app/view/MainWindow.h"
#include "utils/Logger.h"
#include "app/controller/PlaybackController.h"
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
static const ImVec4 COLOR_BG_BLACK = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);      // Dark Grey instead of Pitch Black
static const ImVec4 COLOR_BG_TEAL = ImVec4(0.08f, 0.25f, 0.25f, 1.0f);       // Darker Teal for better text contrast
static const ImVec4 COLOR_ACCENT = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);        // Brighter Teal/Cyan for Accent
static const ImVec4 COLOR_ACCENT_HOVER = ImVec4(0.00f, 0.70f, 0.60f, 0.6f);
static const ImVec4 COLOR_TEXT = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);          // Pure White
static const ImVec4 COLOR_TEXT_DIM = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);      // Brighter Dim Text      

MainWindow::MainWindow() {
    Logger::info("MainWindow created");
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
        
        // Calculate layout sizes
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float availableHeight = ImGui::GetContentRegionAvail().y; // Use full remaining height
        
        // Left Panel: Track List (~25%)
        float leftPanelWidth = availableWidth * 0.25f;
        
        // Right Panel: Art + Controls (~65%)
        float rightPanelWidth = availableWidth - leftPanelWidth - 5.0f; // Subtract spacing
        
        // --- Left Panel ---
        ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
        ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, availableHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        // Delegate rendering to sub-views
        switch (currentScreen_) {
            case Screen::LIBRARY:
                if (libraryView_) libraryView_->render();
                break;
            case Screen::HISTORY:
                if (historyView_) historyView_->render();
                break;
            case Screen::PLAYLIST:
                if (playlistView_) playlistView_->render();
                break;
            default:
                break;
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        
        // --- Right Panel ---
        ImGui::BeginGroup();
        {
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, COLOR_TEXT_DIM);
            // Bottom Controls Height
            // NowPlayingView contains progress bar, slider, buttons, volume, metadata... 
            // It needs significant vertical space. Let's reserve ~200px for it, or use remaining height for Art.
            float controlsHeight = 100.0f;
            float artHeight = availableHeight - controlsHeight;
            float artWidth = rightPanelWidth;
            
            // 1. Album Art / Video (Top Right) - Keep Black Logic inside renderAlbumArt or here?
            // renderAlbumArt uses transparent or inferred BG. 
            // MainWindow originally used a black child for Right Panel.
            // Let's create a child for it.
            ImGui::BeginChild("ArtPanel", ImVec2(artWidth, artHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            renderAlbumArt();
            ImGui::EndChild();
            
            // 2. Playback Controls (Bottom Right)
            ImGui::BeginChild("ControlsPanel", ImVec2(rightPanelWidth, controlsHeight), true); // Using frame
            renderPlaybackControls();
            ImGui::EndChild();
            ImGui::PopStyleColor(); // Pop TextDisabled
        }
        ImGui::EndGroup(); // End Right Panel Group
    }
    ImGui::End();
    
    ImGui::PopStyleColor(2);
    
    // Render file browser if open
    if (fileBrowserView_) fileBrowserView_->render();
    
    // Render View Popups (Root Level)
    if (currentScreen_ == Screen::LIBRARY && libraryView_) libraryView_->renderPopups();
    if (currentScreen_ == Screen::PLAYLIST && playlistView_) playlistView_->renderPopups();
    if (currentScreen_ == Screen::HISTORY && historyView_) historyView_->renderPopups();
#endif
}

void MainWindow::renderTabBar() {
#ifdef USE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_Button, COLOR_BG_TEAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.5f, 1.0f)); // Lighter teal
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, COLOR_ACCENT);
    
    float buttonWidth = 100.0f;
    float buttonHeight = 30.0f;

    // Library button
    bool isLibraryActive = (currentScreen_ == Screen::LIBRARY);
    if (isLibraryActive) ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ACCENT);
    if (ImGui::Button("Library", ImVec2(buttonWidth, buttonHeight))) {
        switchScreen(Screen::LIBRARY);
    }
    if (isLibraryActive) ImGui::PopStyleColor();

    ImGui::SameLine();
    
    // Playlist button
    bool isPlaylistActive = (currentScreen_ == Screen::PLAYLIST);
    if (isPlaylistActive) ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ACCENT);
    if (ImGui::Button("Playlist", ImVec2(buttonWidth, buttonHeight))) {
        switchScreen(Screen::PLAYLIST);
    }
    if (isPlaylistActive) ImGui::PopStyleColor();

    ImGui::SameLine();

    // History button
    bool isHistoryActive = (currentScreen_ == Screen::HISTORY);
    if (isHistoryActive) ImGui::PushStyleColor(ImGuiCol_Button, COLOR_ACCENT);
    if (ImGui::Button("History", ImVec2(buttonWidth, buttonHeight))) {
        switchScreen(Screen::HISTORY);
    }
    if (isHistoryActive) ImGui::PopStyleColor();
    
    //ImGui::SameLine();
    
    // Now Playing info on the right
    // if (playbackState_ && playbackState_->getCurrentTrack()) {
    //     auto track = playbackState_->getCurrentTrack();
    //     std::string nowPlayingText = "Now Playing: " + track->getDisplayName();
    //     float textWidth = ImGui::CalcTextSize(nowPlayingText.c_str()).x;
    //     ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - 20);
    //     ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ACCENT);
    //     ImGui::Text("%s", nowPlayingText.c_str());
    //     ImGui::PopStyleColor();
    // }
    
    ImGui::PopStyleColor(3);
#endif
}

// void MainWindow::renderNowPlayingInfo() {
// #ifdef USE_IMGUI
//     if (playbackState_ && playbackState_->getCurrentTrack()) {
//         auto track = playbackState_->getCurrentTrack();
//         const auto& meta = track->getMetadata();
        
//         // Title (large)
//         ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 0 ? ImGui::GetIO().Fonts->Fonts[0] : nullptr);
//         ImGui::TextWrapped("%s", track->getDisplayName().c_str());
//         ImGui::PopFont();
        
//         // Artist - Album (smaller, dimmed)
//         ImGui::PushStyleColor(ImGuiCol_Text, COLOR_TEXT_DIM);
//         std::string subtitle;
//         if (!meta.artist.empty()) subtitle = meta.artist;
//         if (!meta.album.empty()) {
//             if (!subtitle.empty()) subtitle += " - ";
//             subtitle += meta.album;
//         }
//         if (!subtitle.empty()) {
//             ImGui::TextWrapped("%s", subtitle.c_str());
//         }
//         ImGui::PopStyleColor();
        
//         ImGui::Spacing();
//     } else {
//         ImGui::Text("No track playing");
//         ImGui::Spacing();
//     }
// #endif
// }

void MainWindow::renderAlbumArt() {
#ifdef USE_IMGUI
    // 1. Lấy kích thước cố định của cửa sổ con (ArtPanel)
    // GetWindowSize() trả về kích thước ổn định, không bị ảnh hưởng bởi nội dung bên trong
    ImVec2 winSize = ImGui::GetWindowSize();
    
    // 2. Trừ đi phần lề (Padding) mặc định của ImGui để tránh tràn
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    float usableWidth = winSize.x - (padding.x * 2);
    float usableHeight = winSize.y - (padding.y * 2);

    // 3. Tính toán kích thước hình vuông dựa trên vùng khả dụng
    float minDimension = (usableWidth < usableHeight) ? usableWidth : usableHeight;

    // Check for video texture first
    void* videoTexture = nullptr;
    if (playbackController_ && playbackController_->getEngine()) {
        videoTexture = playbackController_->getEngine()->getVideoTexture();
    }

    if (videoTexture) {
        // 1. Lấy kích thước gốc của Video
        int vW = 0, vH = 0;
        playbackController_->getEngine()->getVideoSize(vW, vH);
        
        float finalW, finalH;

        if (vW > 0 && vH > 0) {
            // --- THUẬT TOÁN SCALE TO FIT (Vừa khít khung) ---
            // Tính tỷ lệ giữa khung chứa và video gốc cho cả 2 chiều
            float scaleX = usableWidth / (float)vW;
            float scaleY = usableHeight / (float)vH;
            
            // Lấy tỷ lệ nhỏ hơn (min) để đảm bảo video nằm trọn trong khung
            // (Ví dụ: Video vuông nhưng màn hình chữ nhật -> bị giới hạn bởi chiều cao -> dùng scaleY)
            float scale = (scaleX < scaleY) ? scaleX : scaleY;

            // Nhân thêm 0.95 nếu muốn hở viền một chút (giống album art)
            // Hoặc để 1.0f nếu muốn video to tối đa
            scale *= 1.0f; 

            finalW = (float)vW * scale;
            finalH = (float)vH * scale;
        } else {
            // Fallback khi chưa load được size video (giả sử 16:9)
            // Để tránh giật hình, ta tính theo usableWidth
            float defaultAspect = 16.0f / 9.0f;
            finalW = usableWidth * 0.95f;
            finalH = finalW / defaultAspect;
            
            // Nếu fallback bị cao quá thì kẹp lại
            if (finalH > usableHeight * 0.95f) {
                finalH = usableHeight * 0.95f;
                finalW = finalH * defaultAspect;
            }
        }
        
        // 2. Căn giữa (Center)
        float vStartX = (usableWidth - finalW) / 2.0f;
        float vStartY = (usableHeight - finalH) / 2.0f;
        
        ImGui::SetCursorPos(ImVec2(padding.x + vStartX, padding.y + vStartY));
        ImGui::Image((ImTextureID)videoTexture, ImVec2(finalW, finalH));

    } else if (playbackState_ && playbackState_->getCurrentTrack()) {
        auto track = playbackState_->getCurrentTrack();
        const auto& meta = track->getMetadata();
        
        
        // --- LOGIC LOAD ẢNH (Đã tối ưu để giảm lag) ---
        if (currentTrackPath_ != track->getPath()) {
            currentTrackPath_ = track->getPath();
            
            // Xóa texture cũ
            if (albumArtTexture_) {
                glDeleteTextures(1, &albumArtTexture_);
                albumArtTexture_ = 0;
            }
            
            // Load mới
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
        
        // --- RENDER ALBUM ART ---
        // Tính toán vị trí căn giữa
        float albumSize = minDimension;
        float startX = (usableWidth - albumSize) / 2.0f;
        float startY = (usableHeight - albumSize) / 2.0f;
        
        // Đặt con trỏ vẽ (Cộng thêm padding để tính từ mép cửa sổ vào)
        ImGui::SetCursorPos(ImVec2(padding.x + startX, padding.y + startY));

        if (albumArtTexture_) {
            ImGui::Image((ImTextureID)(intptr_t)albumArtTexture_, ImVec2(albumSize, albumSize));
        } else {
            // Placeholder
            float startX = 5.0f;
            float startY = 5.0f;

            ImGui::SetCursorPos(ImVec2(padding.x + startX, padding.y + startY));

            ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
            ImGui::BeginChild("AlbumArtPlaceholder", ImVec2(usableWidth, usableHeight), true);
        
            const char* text = "No Art";
            ImVec2 textSize = ImGui::CalcTextSize(text);
            ImGui::SetCursorPos(ImVec2((usableWidth - textSize.x) / 2, (usableHeight - textSize.y) / 2));
            ImGui::Text("%s", text);
            
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    } else {
        // --- NO PLAYING ---

        ImGui::SetCursorPos(ImVec2(padding.x, padding.y));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, COLOR_BG_TEAL);
        ImGui::BeginChild("AlbumArtPlaceholder", ImVec2(usableWidth, usableHeight), true);
        
        const char* text = "No Playing";
        ImVec2 textSize = ImGui::CalcTextSize(text);
        ImGui::SetCursorPos(ImVec2((usableWidth - textSize.x) / 2, (usableHeight - textSize.y) / 2));
        ImGui::Text("%s", text);
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
#endif
}


void MainWindow::renderPlaybackControls() {
#ifdef USE_IMGUI
    if (nowPlayingView_) {
        nowPlayingView_->render();
    } else {
        ImGui::Text("Now Playing View not initialized");
    }
#endif
}

void MainWindow::switchScreen(Screen screen) {
    currentScreen_ = screen;
    Logger::info("Switched to screen: " + std::to_string(static_cast<int>(screen)));
}

void MainWindow::scrollToCurrentTrack() {
    // Handled in renderTrackList via ImGui::SetScrollHereY
}

void MainWindow::handleInput() {
    // Input handled through ImGui
}
