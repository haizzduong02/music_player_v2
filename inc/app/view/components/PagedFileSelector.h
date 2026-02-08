#ifndef PAGED_FILE_SELECTOR_H
#define PAGED_FILE_SELECTOR_H

#include "interfaces/IFileSystem.h" // For FileInfo
#include <functional>
#include <set>
#include <string>
#include <vector>

/**
 * @file PagedFileSelector.h
 * @brief Reusable component for selecting files with pagination
 */

class PagedFileSelector
{
    friend class PagedFileSelectorTest;

  public:
    PagedFileSelector();
    ~PagedFileSelector() = default;

    /**
     * @brief Set the list of items to display
     * @param items List of FileInfo objects
     */
    void setItems(const std::vector<FileInfo> &items);

    /**
     * @brief Render the file list table
     */
    void renderList();

    /**
     * @brief Render the action buttons (Select All, Clear, etc)
     */
    void renderActions();

    /**
     * @brief Render pagination controls
     */
    void renderPagination();

    /**
     * @brief Get selected file paths
     * @return Vector of selected paths
     */
    std::vector<std::string> getSelectedPaths() const;

    /**
     * @brief Select all currently displayed items (or all items?)
     * Usually "Select All" implies all valid items in the current view/folder
     */
    void selectAll();

    /**
     * @brief Clear selection
     */
    void clearSelection();

    /**
     * @brief Add a path to selection
     */
    void addSelection(const std::string &path)
    {
        selectedPaths_.insert(path);
    }

    /**
     * @brief Select random items (clearing existing selection)
     * @param count Number of items to select
     */
    void selectRandom(int count);

    /**
     * @brief Check if there is any selection
     */
    bool hasSelection() const;

    /**
     * @brief Set items per page
     */
    void setItemsPerPage(int count);

    /**
     * @brief Set custom column labels
     * @param nameLabel Label for the name column (default "Name")
     * @param typeLabel Label for the type/extension column (default "Type")
     */
    void setCustomLabels(const std::string &nameLabel, const std::string &typeLabel);

    /**
     * @brief Set the fixed height of the list/table
     * @param height Height in pixels. 0.0f means auto/fill.
     */
    void setListHeight(float height)
    {
        height_ = height;
    }

  private:
    std::vector<FileInfo> items_;
    std::set<std::string> selectedPaths_;

    // UI Config
    std::string labelName_ = "Name";
    std::string labelType_ = "Type";

    int currentPage_ = 0;
    int itemsPerPage_ = 15;
    int totalPages_ = 1;
    char pageInputBuffer_[16] = "1";
    float height_ = 0.0f; // 0.0f = auto-fill available space

    void updatePagination();
    void renderPaginationControls();
    void renderTable();
};

#endif // PAGED_FILE_SELECTOR_H
