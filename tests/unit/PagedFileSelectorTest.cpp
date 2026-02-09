#include "app/view/components/PagedFileSelector.h"
#include <gtest/gtest.h>
#include <imgui.h>
#include <string>
#include <vector>

class PagedFileSelectorTest : public ::testing::Test
{
  protected:
    PagedFileSelector selector;

    void SetUp() override
    {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1024, 768);

        // Build font atlas for headless testing
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)1);
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    // Helpers
    void startFrame()
    {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(1024, 768));
        ImGui::Begin("Selector Window", nullptr, ImGuiWindowFlags_NoDecoration);
    }

    void endFrame()
    {
        ImGui::End();
        ImGui::Render();
    }

    void simulateClick(ImVec2 pos)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = pos;
        io.MouseDown[0] = true;
        io.MouseClicked[0] = true;
    }

    void simulateDrag(ImVec2 start, ImVec2 end)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = start;
        io.MouseDown[0] = true;
        io.MousePos = end;
    }

    bool isSelected(const std::string& path) const
    {
        return selector.selectedPaths_.count(path) > 0;
    }

    // Helpers to access private members
    int getCurrentPage() const
    {
        return selector.currentPage_;
    }
    int getTotalPages() const
    {
        return selector.totalPages_;
    }
    int getItemsPerPage() const
    {
        return selector.itemsPerPage_;
    }

    void setCurrentPage(int page)
    {
        selector.currentPage_ = page;
    }

    void setPageInputBuffer(const std::string& text)
    {
        snprintf(selector.pageInputBuffer_, sizeof(selector.pageInputBuffer_), "%s", text.c_str());
    }

    void renderPaginationHelper() { selector.renderPagination(); }
    void renderListHelper() { selector.renderList(); }
    void updatePaginationHelper() { selector.updatePagination(); }
    
    void onPrevPageClickedHelper() { selector.onPrevPageClicked(); }
    void onNextPageClickedHelper() { selector.onNextPageClicked(); }
    void onGoToPageClickedHelper() { selector.onGoToPageClicked(); }
};

TEST_F(PagedFileSelectorTest, InitialState)
{
    EXPECT_EQ(getCurrentPage(), 0);
    EXPECT_EQ(getTotalPages(), 1);
    EXPECT_FALSE(selector.hasSelection());
}

TEST_F(PagedFileSelectorTest, PaginationCalculation)
{
    std::vector<FileInfo> items;
    for (int i = 0; i < 40; i++)
    {
        items.push_back({"/path/" + std::to_string(i), "file" + std::to_string(i), "mp3", 0, false});
    }

    selector.setItemsPerPage(10);
    selector.setItems(items);

    EXPECT_EQ(getTotalPages(), 4);
    EXPECT_EQ(getCurrentPage(), 0);
}

TEST_F(PagedFileSelectorTest, PaginationUpdatesOnResize)
{
    std::vector<FileInfo> items;
    for (int i = 0; i < 40; i++)
    {
        items.push_back({"/path/" + std::to_string(i), "file" + std::to_string(i), "mp3", 0, false});
    }
    selector.setItems(items);
    selector.setItemsPerPage(10);

    setCurrentPage(3); // Last page
    EXPECT_EQ(getCurrentPage(), 3);

    // Change items per page to 20 -> should have 2 pages
    selector.setItemsPerPage(20);
    EXPECT_EQ(getTotalPages(), 2);
    // Current page was 3, should be clamped to 1 (0-indexed) => 1
    EXPECT_EQ(getCurrentPage(), 1);
}

TEST_F(PagedFileSelectorTest, SelectAll)
{
    std::vector<FileInfo> items;
    items.push_back({"/a", "a", "mp3", 0, false});
    items.push_back({"/b", "b", "mp3", 0, false});

    selector.setItems(items);
    selector.selectAll();

    auto selected = selector.getSelectedPaths();
    EXPECT_EQ(selected.size(), 2);
    EXPECT_TRUE(selector.hasSelection());
}

TEST_F(PagedFileSelectorTest, SelectRandom)
{
    std::vector<FileInfo> items;
    for (int i = 0; i < 100; i++)
    {
        items.push_back({"/path/" + std::to_string(i), "file", "mp3", 0, false});
    }
    selector.setItems(items);

    selector.selectRandom(10);
    EXPECT_EQ(selector.getSelectedPaths().size(), 10);

    selector.selectRandom(5);
    EXPECT_EQ(selector.getSelectedPaths().size(), 5);
}

TEST_F(PagedFileSelectorTest, ClearSelection)
{
    selector.addSelection("/path/1");
    EXPECT_TRUE(selector.hasSelection());
    selector.clearSelection();
    EXPECT_FALSE(selector.hasSelection());
}

TEST_F(PagedFileSelectorTest, RenderMethods)
{
    std::vector<FileInfo> items;
    for (int i = 0; i < 20; i++)
    {
        items.push_back({"/p/" + std::to_string(i), "f" + std::to_string(i), "mp3", 0, false});
    }
    selector.setItems(items);
    selector.setItemsPerPage(10);
    selector.setCustomLabels("Title", "Artist");
    selector.setListHeight(400);

    startFrame();
    selector.renderActions();
    selector.renderList();
    selector.renderPagination();
    endFrame();
}

TEST_F(PagedFileSelectorTest, RenderPaginationInteractions)
{
    std::vector<FileInfo> items;
    for (int i = 0; i < 30; i++)
        items.push_back({"/p/" + std::to_string(i), "f", "mp3", 0, false});
    
    selector.setItemsPerPage(10);
    selector.setItems(items); // 3 pages
    selector.setListHeight(400); // Shorter list for higher pagination

    // 1. Manually hit branches by calling handlers directly
    onNextPageClickedHelper();
    EXPECT_EQ(getCurrentPage(), 1);
    
    onPrevPageClickedHelper();
    EXPECT_EQ(getCurrentPage(), 0);
    
    setPageInputBuffer("3");
    onGoToPageClickedHelper();
    EXPECT_EQ(getCurrentPage(), 2);
}

TEST_F(PagedFileSelectorTest, RenderPaginationEdgeCasesDirect)
{
    std::vector<FileInfo> items(30, {"/p", "f", "mp3", 0, false});
    selector.setItems(items);
    
    // Hit Go with valid/invalid
    setPageInputBuffer("100"); // Too high
    onGoToPageClickedHelper();
    EXPECT_EQ(getCurrentPage(), 1); // 30 items / 15 per page = 2 pages (0, 1)

    setPageInputBuffer("0"); // Too low
    onGoToPageClickedHelper();
    EXPECT_EQ(getCurrentPage(), 0);
}

TEST_F(PagedFileSelectorTest, RenderWithDisabledItems)
{
    std::vector<FileInfo> items;
    items.push_back({"/p/disabled", "disabled", "mp3", 0, false});
    selector.setItems(items);
    
    std::set<std::string> disabled;
    disabled.insert("/p/disabled");
    selector.setDisabledItems(disabled);

    startFrame();
    selector.renderList(); // Hits isDisabled logic (line 120-128)
    endFrame();
}

TEST_F(PagedFileSelectorTest, SelectRandomEdgeCases)
{
    selector.clearSelection();
    selector.selectRandom(0);
    EXPECT_EQ(selector.getSelectedPaths().size(), 0);

    selector.selectRandom(-1);
    EXPECT_EQ(selector.getSelectedPaths().size(), 0);

    std::vector<FileInfo> items;
    selector.setItems(items);
    selector.selectRandom(10);
    EXPECT_EQ(selector.getSelectedPaths().size(), 0);
}

TEST_F(PagedFileSelectorTest, PaginationEdgeCases)
{
    std::vector<FileInfo> items;
    for (int i = 0; i < 25; i++)
        items.push_back({"/p/" + std::to_string(i), "f", "mp3", 0, false});
    
    selector.setItemsPerPage(10);
    selector.setItems(items); // totalPages = 3

    setCurrentPage(-5);
    selector.setItems(items); // Should clamp to 0
    EXPECT_EQ(getCurrentPage(), 0);

    setCurrentPage(10);
    selector.setItems(items); // Should clamp to 2
    EXPECT_EQ(getCurrentPage(), 2);

    selector.setItems({}); // Empty
    EXPECT_EQ(getTotalPages(), 1);
    EXPECT_EQ(getCurrentPage(), 0);
}

TEST_F(PagedFileSelectorTest, ItemsPerPageInvalid)
{
    selector.setItemsPerPage(0);
    EXPECT_EQ(getItemsPerPage(), 15); // Default
    selector.setItemsPerPage(-10);
    EXPECT_EQ(getItemsPerPage(), 15);
}

TEST_F(PagedFileSelectorTest, ExtensionFormatLogic)
{
    std::vector<FileInfo> items;
    items.push_back({"/p/1.mp3", "1", ".mp3", 0, false});
    items.push_back({"/p/2", "2", "txt", 0, false});
    selector.setItems(items);

    startFrame();
    selector.renderList();
    endFrame();
}

