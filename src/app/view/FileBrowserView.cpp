#include "app/view/FileBrowserView.h"
#include "app/controller/PlaylistController.h"
#include "app/model/MediaFileFactory.h"
#include "utils/Logger.h"
#include <algorithm>
#include <set>
#include <unordered_set>
#include <imgui.h>

FileBrowserView::FileBrowserView(IFileSystem *fileSystem, LibraryController *libController)
    : fileSystem_(fileSystem), libController_(libController)
{

    // Start at home directory if available, otherwise root
    const char *homeEnv = std::getenv("HOME");
    std::string startPath = (homeEnv && fileSystem_->exists(homeEnv)) ? homeEnv : "/";

    // Check if we can start at "music_media" subfolder? No, user said they moved everything to home directly or wants
    // home as default. Let's just use home.
    currentPath_ = startPath;

    // Increase density for File Browser as requested ("increase selector height")
    fileSelector_.setItemsPerPage(25);

    refreshCurrentDirectory();
}

void FileBrowserView::show()
{
    // Refresh to ensure we have latest library state (important if library loaded after view init)
    refreshCurrentDirectory();
    fileSelector_.clearSelection();
    BaseView::show();
}

void FileBrowserView::setPlaylistController(PlaylistController *controller)
{
    playlistController_ = controller;
}

void FileBrowserView::setMode(BrowserMode mode)
{
    mode_ = mode;
}

void FileBrowserView::setTargetPlaylist(const std::string &playlistName)
{
    targetPlaylistName_ = playlistName;
}

void FileBrowserView::render()
{
    if (!isVisible())
        return;

    // If in popup mode, we don't render as a window (handled by parent)
    if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN)
        return;

    // Set larger window size
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(800, 600), ImVec2(1600, 1000));

    if (ImGui::Begin("File Browser", &visible_))
    {
        renderContent();
    }
    ImGui::End();
}

void FileBrowserView::renderPopup()
{
    // Popup logic
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal("File Browser", &open))
    {
        renderContent();

        ImGui::EndPopup();
    }

    if (!open)
    {
        visible_ = false;
    }
}

void FileBrowserView::renderContent()
{
    // Current path info at the very top (optional but good for context)
    ImGui::TextDisabled("Location: %s", currentPath_.c_str());
    ImGui::Separator();

    // Get available content size
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float footerHeight = 10.0f; // Minimal footer now
    float contentHeight = availableSize.y - footerHeight;

    // Left panel width
    float leftPanelWidth = availableSize.x * 0.30f;

    // LEFT PANEL: Folders only + Navigation
    if (ImGui::BeginChild("FolderPanel", ImVec2(leftPanelWidth, contentHeight), true))
    {
        // Navigation buttons at the top of the tree
        float navBtnWidth = (ImGui::GetContentRegionAvail().x - 10.0f) / 3.0f;

        if (ImGui::Button("Up", ImVec2(navBtnWidth, 0)))
            onNavigateUpClicked();
        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(navBtnWidth, 0)))
            onRefreshClicked();
        ImGui::SameLine();

        const char *homeEnv = std::getenv("HOME");
        std::string homePath = homeEnv ? std::string(homeEnv) : "/home";
        if (ImGui::Button("Home", ImVec2(navBtnWidth, 0)))
            onHomeClicked();

        ImGui::Separator();
        ImGui::Text("Folders (%d tracks)", currentTrackCount_);
        ImGui::Separator();

        for (size_t i = 0; i < currentFiles_.size(); ++i)
        {
            const auto &fileInfo = currentFiles_[i];
            if (!fileInfo.isDirectory)
                continue;

            if (ImGui::Selectable(fileInfo.name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    onFolderDoubleClicked(fileInfo.path);
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginGroup();

    // RIGHT PANEL: Media files + Actions
    // RIGHT PANEL: Media files + Actions
    // Right Panel container
    if (ImGui::BeginChild("FilePanel", ImVec2(0, contentHeight), true))
    {
        // Calculate heights
        // Header: Add Button + Selector Actions (~30-40px)
        // Footer: Pagination (~30px)
        // List: Remaining

        // Fixed Header Group
        ImGui::BeginGroup();
        {
            // Actions at the top of the file list
            std::string addBtnText = "Add Selected";
            if (mode_ == BrowserMode::PLAYLIST_SELECTION)
                addBtnText = "Add to Playlist";
            else if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN)
                addBtnText = "Add & Return";
            else
                addBtnText = "Add to Library";

            if (ImGui::Button(addBtnText.c_str()))
            {
                onAddSelectedClicked();
            }

            ImGui::SameLine();
            // Filter Dropdown
            ImGui::SetNextItemWidth(100);
            if (ImGui::BeginCombo("##ExtensionFilter", selectedExtension_.c_str()))
            {
                for (const auto &ext : availableExtensions_)
                {
                    bool isSelected = (selectedExtension_ == ext);
                    std::string label = ext;
                    if (label.empty()) label = "Unknown";
                    else if (label[0] == '.') label = label.substr(1); // Remove dot for display

                    if (ImGui::Selectable(label.c_str(), isSelected))
                    {
                        selectedExtension_ = ext;
                        applyFilter();
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Button("Add Random 20"))
            {
                onAddRandomClicked();
            }

            ImGui::SameLine();
            ImGui::Text("|"); // Simple separator substitute
            ImGui::SameLine();

            // Selector Actions (Select All, Clear)
            fileSelector_.renderActions();
        }
        ImGui::EndGroup();

        ImGui::Separator();

        // Scrollable File List
        // Reserve space for pagination footer (approx 40px)
        footerHeight = 40.0f; // Update existing variable instead of redeclaring
        float listHeight = ImGui::GetContentRegionAvail().y - footerHeight;
        fileSelector_.setListHeight(listHeight);
        fileSelector_.renderList();

        // Fixed Footer (Pagination)
        fileSelector_.renderPagination();
    } // Close BeginChild("FilePanel")
    ImGui::EndChild();
    ImGui::EndGroup();
}

void FileBrowserView::handleInput()
{
    // Handled through ImGui
}

void FileBrowserView::setCurrentDirectory(const std::string &path)
{
    navigateTo(path);
}

void FileBrowserView::navigateUp()
{
    // Get parent directory
    size_t lastSlash = currentPath_.find_last_of("/\\");
    if (lastSlash != std::string::npos && lastSlash > 0)
    {
        currentPath_ = currentPath_.substr(0, lastSlash);
        refreshCurrentDirectory();
    }
}

void FileBrowserView::navigateTo(const std::string &path)
{
    if (fileSystem_->exists(path) && fileSystem_->isDirectory(path))
    {
        currentPath_ = path;
        refreshCurrentDirectory();
    }
    else
    {
        Logger::warn("Invalid directory: " + path);
    }
}

void FileBrowserView::refreshCurrentDirectory()
{
    // 1. Get directories for navigation (Left Panel)
    auto allFiles = fileSystem_->browse(currentPath_);
    currentFiles_.clear();
    for (const auto &file : allFiles)
    {
        if (file.isDirectory)
        {
            currentFiles_.push_back(file);
        }
    }

    // 2. Get recursive media files with max depth (Right Panel)
    allMediaFiles_.clear();
    availableExtensions_.clear();
    availableExtensions_.insert("All");

    std::vector<FileInfo> mediaFiles; // Temporary scan result container

    // Get all supported extensions
    std::vector<std::string> extensions = MediaFileFactory::getAllSupportedFormats();

    // Scan recursively with max depth of 3
    std::vector<std::string> mediaPaths = fileSystem_->getMediaFiles(currentPath_, extensions, 3);

    // Convert to FileInfo
    for (const auto &path : mediaPaths)
    {
        FileInfo info;
        info.path = path;

        // Extract filename and extension manually
        size_t lastSlash = path.find_last_of("/\\");
        info.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

        size_t lastDot = info.name.find_last_of('.');
        info.extension = (lastDot != std::string::npos) ? info.name.substr(lastDot) : "";
        info.isDirectory = false;
        info.size = 0;

        if (!info.extension.empty())
        {
            availableExtensions_.insert(info.extension);
        }

        allMediaFiles_.push_back(info);
    }

    applyFilter();
}

void FileBrowserView::applyFilter()
{
    // 3. Filter and Sort logic
    std::vector<FileInfo> filteredFiles;
    if (selectedExtension_ == "All")
    {
        filteredFiles = allMediaFiles_;
    }
    else
    {
        for (const auto &file : allMediaFiles_)
        {
            if (file.extension == selectedExtension_)
            {
                filteredFiles.push_back(file);
            }
        }
    }
    
    std::vector<FileInfo> &mediaFiles = filteredFiles; // Ref for sorting code block compatibility
    std::unordered_set<std::string> libraryPaths;
    if (libController_)
    {
        libraryPaths = libController_->getAllTrackPaths();
    }
    
    std::set<std::string> disabledPaths;

    // Identify disabled files
    for (const auto &file : mediaFiles)
    {
        bool found = false;
        if (libraryPaths.count(file.path))
        {
            found = true;
        }
        else
        {
            // Try canonical path match in case of symlinks/normalization differences
            try
            {
                std::error_code ec;
                std::string avgCanonical = std::filesystem::canonical(file.path, ec).string();
                if (!ec && libraryPaths.count(avgCanonical))
                {
                    found = true;
                }
            }
            catch (...) {}
        }

        if (found)
        {
            disabledPaths.insert(file.path);
        }
    }

    // Sort: Unadded first, then Added. Tie-break with name.
    std::sort(mediaFiles.begin(), mediaFiles.end(), [&](const FileInfo &a, const FileInfo &b) {
        bool aInLib = disabledPaths.count(a.path); // Use disabledPaths set which is already resolved
        bool bInLib = disabledPaths.count(b.path);
        
        if (aInLib != bInLib)
        {
            return aInLib < bInLib; 
        }
        return a.name < b.name;
    });

    currentTrackCount_ = (int)mediaFiles.size();

    fileSelector_.setItems(mediaFiles);
    fileSelector_.setDisabledItems(disabledPaths); // Pass disabled items
    
    // fileSelector_.clearSelection(); // Optional: clear when valid path changes?
    // Usually navigating to a new folder implies new context, so clearing is safer to avoid confusion.
    fileSelector_.clearSelection();
}

// Pagination methods removed - delegated to PagedFileSelector

void FileBrowserView::onNavigateUpClicked()
{
    navigateUp();
}

void FileBrowserView::onRefreshClicked()
{
    refreshCurrentDirectory();
}

void FileBrowserView::onHomeClicked()
{
    const char *homeEnv = std::getenv("HOME");
    std::string homePath = homeEnv ? std::string(homeEnv) : "/home";
    navigateTo(homePath);
}

void FileBrowserView::onFolderDoubleClicked(const std::string &path)
{
    navigateTo(path);
}

void FileBrowserView::onAddSelectedClicked()
{
    processFiles(fileSelector_.getSelectedPaths());
}

void FileBrowserView::onAddRandomClicked()
{
    fileSelector_.selectRandom(20);
    processFiles(fileSelector_.getSelectedPaths());
}

void FileBrowserView::processFiles(const std::vector<std::string> &paths)
{
    if (paths.empty())
        return;

    if (mode_ == BrowserMode::PLAYLIST_SELECTION && playlistController_ && !targetPlaylistName_.empty())
    {
        for (const auto &path : paths)
        {
            playlistController_->addToPlaylistAndLibrary(targetPlaylistName_, path);
        }
        Logger::info("Added " + std::to_string(paths.size()) + " files to playlist.");
    }
    else
    {
        // Async add to library
        // We pass all paths. The async method handles metadata reading.
        libController_->addMediaFilesAsync(paths);
        
        // Return paths immediately for callback (e.g. if adding to current playlist view)
        // Note: The files might not be in library yet, but usually callbacks handle string paths.
        if (mode_ == BrowserMode::LIBRARY_ADD_AND_RETURN && onFilesAddedCallback_)
        {
            onFilesAddedCallback_(paths);
            visible_ = false;
        }
    }
}
