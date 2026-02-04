#include "../../../inc/app/view/MainWindow.h"
#include "../../../inc/utils/Logger.h"
#include <sstream>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

MainWindow::MainWindow() {
    Logger::getInstance().info("MainWindow created");
}

void MainWindow::render() {
    if (!isVisible()) {
        return;
    }
    
#ifdef USE_IMGUI
    // Prevent segfault if ImGui context is not initialized (e.g. headless)
    if (ImGui::GetCurrentContext() == nullptr) {
        static bool logged = false;
        if (!logged) {
            Logger::getInstance().warn("ImGui logic skipped: Context not initialized");
            logged = true;
        }
        return;
    }

    renderMenuBar();
    
    // Render all child views
    if (libraryView_) libraryView_->render();
    if (playlistView_) playlistView_->render();
    if (nowPlayingView_) nowPlayingView_->render();
    if (historyView_) historyView_->render();
    if (fileBrowserView_) fileBrowserView_->render();
#endif
}

void MainWindow::handleInput() {
    // Input handled through ImGui
}

void MainWindow::addView(IView* view) {
    // Views are managed through setters
    Logger::getInstance().info("View added to MainWindow");
}

void MainWindow::removeView(IView* view) {
    // Views are managed through setters
    Logger::getInstance().info("View removed from MainWindow");
}

void MainWindow::renderMenuBar() {
#ifdef USE_IMGUI
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                // Signal to exit application
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (libraryView_ && ImGui::MenuItem("Library")) {
                libraryView_->show();
            }
            if (playlistView_ && ImGui::MenuItem("Playlists")) {
                playlistView_->show();
            }
            if (nowPlayingView_ && ImGui::MenuItem("Now Playing")) {
                nowPlayingView_->show();
            }
            if (historyView_ && ImGui::MenuItem("History")) {
                historyView_->show();
            }
            if (fileBrowserView_ && ImGui::MenuItem("File Browser")) {
                fileBrowserView_->show();
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
#endif
}
