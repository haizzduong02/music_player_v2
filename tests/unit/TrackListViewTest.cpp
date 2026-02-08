#include "app/view/TrackListView.h"
#include "app/model/MediaFile.h"
#include "imgui.h"
#include "tests/mocks/MockPersistence.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

// Concrete implementation of TrackListView for testing
class TestTrackListView : public TrackListView
{
  public:
    // Expose protected methods for testing
    using TrackListView::isEditMode_;
    using TrackListView::isSelected;
    using TrackListView::removeSelectedTracks;
    using TrackListView::selectAll;
    using TrackListView::selectedPaths_;
    using TrackListView::toggleEditMode;
    using TrackListView::toggleSelection;

    void render() override
    {
    }
    void handleInput() override
    {
    }
    void update(void *) override
    {
    }
};

class TrackListViewTest : public ::testing::Test
{
  protected:
    std::unique_ptr<TestTrackListView> view;
    std::vector<std::shared_ptr<MediaFile>> testTracks;

    void SetUp() override
    {
        // Headless ImGui not strictly needed for logic tests, but good practice if called
        ImGui::CreateContext();

        view = std::make_unique<TestTrackListView>();

        testTracks.push_back(std::make_shared<MediaFile>("/track1.mp3"));
        testTracks.push_back(std::make_shared<MediaFile>("/track2.mp3"));
        testTracks.push_back(std::make_shared<MediaFile>("/track3.mp3"));
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }
};

TEST_F(TrackListViewTest, EditModeToggles)
{
    EXPECT_FALSE(view->isEditMode_);
    view->toggleEditMode();
    EXPECT_TRUE(view->isEditMode_);
    view->toggleEditMode();
    EXPECT_FALSE(view->isEditMode_);
}

TEST_F(TrackListViewTest, EditModeClearSelectionOnExit)
{
    view->toggleEditMode();
    view->toggleSelection("/track1.mp3");
    EXPECT_TRUE(view->isSelected("/track1.mp3"));

    // Exiting edit mode should clear selection
    view->toggleEditMode();
    EXPECT_FALSE(view->isEditMode_);
    EXPECT_FALSE(view->isSelected("/track1.mp3"));
}

TEST_F(TrackListViewTest, SelectionLogic)
{
    view->toggleEditMode();

    // Toggle On
    view->toggleSelection("/track1.mp3");
    EXPECT_TRUE(view->isSelected("/track1.mp3"));
    EXPECT_FALSE(view->isSelected("/track2.mp3"));

    // Toggle Off
    view->toggleSelection("/track1.mp3");
    EXPECT_FALSE(view->isSelected("/track1.mp3"));
}

TEST_F(TrackListViewTest, SelectAll)
{
    view->toggleEditMode();
    view->selectAll(testTracks);

    EXPECT_TRUE(view->isSelected("/track1.mp3"));
    EXPECT_TRUE(view->isSelected("/track2.mp3"));
    EXPECT_TRUE(view->isSelected("/track3.mp3"));
}
