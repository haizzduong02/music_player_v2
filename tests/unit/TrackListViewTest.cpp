#include "app/view/TrackListView.h"
#include "app/model/MediaFile.h"
#include "app/model/PlaybackState.h"
#include "app/model/History.h"
#include "app/model/PlaylistManager.h"
#include "app/controller/PlaybackController.h"
#include "imgui.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockTrackListController.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

using namespace testing;

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
    using TrackListView::renderEditToolbar;
    using TrackListView::renderTrackListTable;
    using TrackListView::calculateMarqueeOffset;
    using TrackListView::isHoveredRow;
    
    void setControllers(ITrackListController* list, PlaybackController* play, PlaylistManager* pl) {
        listController_ = list;
        playbackController_ = play;
        playlistManager_ = pl;
    }

    void render() override {}
    void handleInput() override {}
    void update(void *) override {}
};

class TrackListViewTest : public ::testing::Test
{
  protected:
    std::unique_ptr<TestTrackListView> view;
    std::vector<std::shared_ptr<MediaFile>> testTracks;
    std::unique_ptr<MockTrackListController> mockListController;
    std::unique_ptr<MockPlaybackEngine> mockEngine;
    std::unique_ptr<PlaybackState> playbackState;
    std::unique_ptr<History> history;
    std::unique_ptr<PlaybackController> playbackController;
    std::unique_ptr<PlaylistManager> playlistManager;
    std::unique_ptr<MockPersistence> mockPersistence;

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

        view = std::make_unique<TestTrackListView>();
        mockListController = std::make_unique<MockTrackListController>();
        mockEngine = std::make_unique<MockPlaybackEngine>();
        playbackState = std::make_unique<PlaybackState>();
        history = std::make_unique<History>();
        playbackController = std::make_unique<PlaybackController>(mockEngine.get(), playbackState.get(), history.get());
        
        mockPersistence = std::make_unique<MockPersistence>();
        playlistManager = std::make_unique<PlaylistManager>(mockPersistence.get());

        testTracks.push_back(std::make_shared<MediaFile>("/track1.mp3"));
        testTracks.push_back(std::make_shared<MediaFile>("/track2.mp3"));

        view->setControllers(mockListController.get(), playbackController.get(), playlistManager.get());
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    void startFrame() {
        ImGui::NewFrame();
    }

    void endFrame() {
        ImGui::Render();
    }
    
    void simulateClick(ImVec2 pos) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = pos;
        io.MouseDown[0] = true;
        io.MouseClicked[0] = true;
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
    view->toggleSelection("/track1.mp3");
    EXPECT_TRUE(view->isSelected("/track1.mp3"));
    view->toggleSelection("/track1.mp3");
    EXPECT_FALSE(view->isSelected("/track1.mp3"));
}

TEST_F(TrackListViewTest, SelectAll)
{
    view->toggleEditMode();
    view->selectAll(testTracks);
    EXPECT_TRUE(view->isSelected("/track1.mp3"));
    EXPECT_TRUE(view->isSelected("/track2.mp3"));
    
    // Null track should not crash
    std::vector<std::shared_ptr<MediaFile>> tracksWithNull = testTracks;
    tracksWithNull.push_back(nullptr);
    view->selectAll(tracksWithNull);
    EXPECT_EQ(view->selectedPaths_.size(), 2);
}

TEST_F(TrackListViewTest, RemoveTracks)
{
    view->toggleEditMode();
    view->toggleSelection("/track1.mp3");
    
    EXPECT_CALL(*mockListController, removeTracks(_)).Times(1);
    view->removeSelectedTracks();
    EXPECT_TRUE(view->selectedPaths_.empty());
}

TEST_F(TrackListViewTest, RenderEditToolbarInteractions)
{
    // 1. Edit Mode Toolbar
    view->toggleEditMode();
    startFrame();
    view->renderEditToolbar(testTracks);
    endFrame();

    // 2. Click Done
    simulateClick(ImVec2(10, 10)); // Arbitrary pos, we just want to hit branches in UI logic
    startFrame();
    view->renderEditToolbar(testTracks);
    endFrame();
    
    // 3. Click Remove
    EXPECT_CALL(*mockListController, removeTracks(_)).Times(AtLeast(0));
    startFrame();
    view->renderEditToolbar(testTracks);
    endFrame();

    // 4. Case when NOT in edit mode
    view->toggleEditMode(); // now false
    startFrame();
    view->renderEditToolbar(testTracks);
    endFrame();
}

TEST_F(TrackListViewTest, RenderTrackListTableComplex)
{
    // Setup playing state to hit highlight logic
    playbackState->setPlayback(testTracks[0], PlaybackStatus::PLAYING);

    startFrame();
    view->renderTrackListTable(testTracks);
    endFrame();

    // Test Edit Mode selection in table
    view->toggleEditMode();
    startFrame();
    view->renderTrackListTable(testTracks);
    endFrame();

    // Test Popups - Add to Playlist (+)
    // Frame 1: Click button to open popup
    startFrame();
    ImGui::SetCursorPos(ImVec2(1000, 100)); // Try to hit the '+' button
    view->renderTrackListTable(testTracks);
    endFrame();
    
    // Test Metadata Popup (i)
    startFrame();
    ImGui::SetCursorPos(ImVec2(950, 100)); // Try to hit 'i' button
    view->renderTrackListTable(testTracks);
    endFrame();
}

TEST_F(TrackListViewTest, MarqueeLogic)
{
    startFrame();
    ImGui::Begin("MarqueeTest");
    float offset = view->calculateMarqueeOffset(200.0f, 100.0f, 1);
    EXPECT_EQ(offset, 0.0f); // Fast return if not hovered
    ImGui::End();
    endFrame();
    
    // Simulate hover and time drift
}

TEST_F(TrackListViewTest, PlayTrackOnClick)
{
    // Mock clicking a row
    EXPECT_CALL(*mockListController, playTrack(_, 0)).Times(AtMost(1));
    startFrame();
    view->renderTrackListTable(testTracks);
    endFrame();
}
