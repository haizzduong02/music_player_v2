#include "app/view/LibraryView.h"
#include "app/controller/PlaybackController.h"
#include "app/model/PlaylistManager.h"
#include "app/view/FileBrowserView.h"
#include "utils/Logger.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif
#include <algorithm>

LibraryView::LibraryView(LibraryController *controller, Library *library, PlaybackController *playbackController,
                         class PlaylistManager *playlistManager)
    : libraryController_(controller), library_(library), selectedIndex_(-1)
{

    // Initialize TrackListView base members
    listController_ = controller;
    playbackController_ = playbackController;
    playlistManager_ = playlistManager;

    // Attach as observer to library
    if (library_)
    {
        library_->attach(this);
        refreshDisplay(); // Initial population
    }
}

LibraryView::~LibraryView()
{
    // Detach from library
    if (library_)
    {
        library_->detach(this);
    }
}

void LibraryView::render()
{
    if (needsRefresh_.exchange(false))
    {
        refreshDisplay();
    }

    renderSearchBar();

    ImGui::SameLine();

    // Filter Dropdown
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##LibExtensionFilter", selectedExtension_.c_str()))
    {
        std::string newSelection = selectedExtension_;
        bool selectionChanged = false;

        for (const auto &ext : availableExtensions_)
        {
            bool isSelected = (selectedExtension_ == ext);
            if (ImGui::Selectable(ext.c_str(), isSelected))
            {
                newSelection = ext;
                selectionChanged = true;
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();

        if (selectionChanged)
        {
            selectedExtension_ = newSelection;
            refreshDisplay();
        }
    }

    // Top Controls
    if (ImGui::Button("Add Files"))
    {
        if (fileBrowserView_)
        {
            fileBrowserView_->setMode(FileBrowserView::BrowserMode::LIBRARY);
            fileBrowserView_->show();
        }
    }

    ImGui::SameLine();

    // Use cached files list
    const auto &files = displayedFiles_;

    ImGui::Text("Library: %zu tracks", files.size());
    ImGui::Separator();

    renderEditToolbar(files);

    renderTrackListTable(files);
}

void LibraryView::renderSearchBar()
{
    static char searchBuffer[256] = "";
    strncpy(searchBuffer, searchQuery_.c_str(), sizeof(searchBuffer));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.45f, 0.45f, 1.0f));
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer)))
    {
        searchQuery_ = searchBuffer;
        refreshDisplay();
    }
    ImGui::PopStyleColor();
}

void LibraryView::handleInput()
{
    // Handled through ImGui
}

void LibraryView::update(void *subject)
{
    (void)subject;
    needsRefresh_ = true;
}

void LibraryView::refreshDisplay()
{
    // This view currently calculates display tracks on the fly in render()
    // but we can use this to cache or force a refresh if needed.
    if (library_)
    {
        auto allFiles = searchQuery_.empty() ? library_->getAll() : library_->search(searchQuery_);

        // 1. Populate extensions
        availableExtensions_.clear();
        availableExtensions_.insert("All");
        for (const auto &file : allFiles)
        {
            std::string ext = file->getExtension();
            if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
            if (!ext.empty())
                availableExtensions_.insert(ext);
        }

        // 2. Filter
        if (selectedExtension_ == "All")
        {
            displayedFiles_ = allFiles;
        }
        else
        {
            displayedFiles_.clear();
            for (const auto &file : allFiles)
            {
                std::string ext = file->getExtension();
                if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
                
                if (ext == selectedExtension_)
                {
                    displayedFiles_.push_back(file);
                }
            }
        }
    }
}
