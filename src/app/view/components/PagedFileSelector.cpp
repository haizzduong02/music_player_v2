#include "app/view/components/PagedFileSelector.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <numeric>
#include <random>

PagedFileSelector::PagedFileSelector()
{
    snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "1");
}

void PagedFileSelector::setItems(const std::vector<FileInfo> &items)
{
    items_ = items;
    updatePagination();
}

void PagedFileSelector::setCustomLabels(const std::string &nameLabel, const std::string &typeLabel)
{
    labelName_ = nameLabel;
    labelType_ = typeLabel;
}

void PagedFileSelector::setItemsPerPage(int count)
{
    if (count > 0)
    {
        itemsPerPage_ = count;
        updatePagination();
    }
}

void PagedFileSelector::updatePagination()
{
    if (items_.empty())
    {
        totalPages_ = 1;
        currentPage_ = 0;
    }
    else
    {
        totalPages_ = (int)((items_.size() + itemsPerPage_ - 1) / itemsPerPage_);
    }

    if (currentPage_ >= totalPages_)
        currentPage_ = totalPages_ - 1;
    if (currentPage_ < 0)
        currentPage_ = 0;

    snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
}

void PagedFileSelector::renderActions()
{
    // Actions bar
    if (ImGui::Button("Select All"))
    {
        selectAll();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection"))
    {
        clearSelection();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%zu selected", selectedPaths_.size());
}

void PagedFileSelector::renderList()
{
    // Table
    ImVec2 outerSize = ImVec2(0.0f, height_); // Use configured height (0=auto)
    if (ImGui::BeginTable("FilesSelectorTable", 4,
                          ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_ScrollY,
                          outerSize))
    {
        ImGui::TableSetupColumn("##Select", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn(labelName_.c_str(), ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn(labelType_.c_str(), ImGuiTableColumnFlags_WidthFixed, 100.0f); // Wider for Artist/Type
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_NoHide);
        ImGui::TableHeadersRow();

        size_t startIndex = static_cast<size_t>(currentPage_) * itemsPerPage_;
        size_t endIndex = std::min(startIndex + static_cast<size_t>(itemsPerPage_), items_.size());

        for (size_t i = startIndex; i < endIndex; ++i)
        {
            const auto &fileInfo = items_[i];

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            bool isSelected = (selectedPaths_.find(fileInfo.path) != selectedPaths_.end());
            ImGui::PushID((int)i);
            if (ImGui::Checkbox("##check", &isSelected))
            {
                if (isSelected)
                    selectedPaths_.insert(fileInfo.path);
                else
                    selectedPaths_.erase(fileInfo.path);
            }
            ImGui::PopID();

            ImGui::TableNextColumn();
            ImGui::Text("%s", fileInfo.name.c_str());

            ImGui::TableNextColumn();
            std::string ext = (fileInfo.extension.length() > 0 && fileInfo.extension[0] == '.')
                                  ? fileInfo.extension.substr(1)
                                  : fileInfo.extension;
            ImGui::Text("%s", ext.c_str());

            ImGui::TableNextColumn();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", fileInfo.path.c_str());
        }
        ImGui::EndTable();
    }

    if (items_.empty())
    {
        ImGui::TextDisabled("No files found");
    }
}

void PagedFileSelector::renderPagination()
{
    renderPaginationControls();
}

void PagedFileSelector::renderPaginationControls()
{
    if (totalPages_ <= 1)
        return;

    float buttonWidth = 60.0f;
    float textWidth = 100.0f;
    float totalWidth = buttonWidth * 3 + textWidth + 80.0f;

    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + ImGui::GetCursorPosX());

    if (ImGui::Button("Prev", ImVec2(buttonWidth, 0)))
    {
        if (currentPage_ > 0)
        {
            currentPage_--;
            snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
        }
    }

    ImGui::SameLine();

    // Page Input
    ImGui::PushItemWidth(50.0f);
    if (ImGui::InputText("##PageInputSelector", pageInputBuffer_, sizeof(pageInputBuffer_),
                         ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        int page = std::atoi(pageInputBuffer_);
        if (page < 1)
            page = 1;
        if (page > totalPages_)
            page = totalPages_;
        currentPage_ = page - 1;
        snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::Text("of %d", totalPages_);

    ImGui::SameLine();
    if (ImGui::Button("Go", ImVec2(buttonWidth, 0)))
    {
        int page = std::atoi(pageInputBuffer_);
        if (page < 1)
            page = 1;
        if (page > totalPages_)
            page = totalPages_;
        currentPage_ = page - 1;
        snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
    }

    ImGui::SameLine();

    if (ImGui::Button("Next", ImVec2(buttonWidth, 0)))
    {
        if (currentPage_ < totalPages_ - 1)
        {
            currentPage_++;
            snprintf(pageInputBuffer_, sizeof(pageInputBuffer_), "%d", currentPage_ + 1);
        }
    }
}

std::vector<std::string> PagedFileSelector::getSelectedPaths() const
{
    return std::vector<std::string>(selectedPaths_.begin(), selectedPaths_.end());
}

void PagedFileSelector::selectAll()
{
    for (const auto &item : items_)
    {
        selectedPaths_.insert(item.path);
    }
}

void PagedFileSelector::clearSelection()
{
    selectedPaths_.clear();
}

bool PagedFileSelector::hasSelection() const
{
    return !selectedPaths_.empty();
}

void PagedFileSelector::selectRandom(int count)
{
    clearSelection();
    if (items_.empty() || count <= 0)
        return;

    std::vector<int> indices(items_.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    int toSelect = std::min((int)items_.size(), count);
    for (int i = 0; i < toSelect; ++i)
    {
        selectedPaths_.insert(items_[indices[i]].path);
    }
}
