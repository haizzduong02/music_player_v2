#include "app/view/HistoryView.h"
#include "app/controller/HistoryController.h"
#include "app/controller/PlaybackController.h"
#include "app/model/History.h"
#include "app/model/PlaylistManager.h"
#include "imgui.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class HistoryViewTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;

    std::shared_ptr<History> history;
    std::unique_ptr<HistoryController> historyController;

    std::shared_ptr<PlaybackState> playbackState;
    std::vector<std::shared_ptr<Playlist>> playlists; // Stub
    std::unique_ptr<PlaylistManager> playlistManager;
    std::unique_ptr<PlaybackController> playbackController;

    std::unique_ptr<HistoryView> view;

    void SetUp() override
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1024, 768);
        
        // Build font atlas for headless testing
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)1); // Dummy texture ID

        mockPersist = std::make_shared<MockPersistence>();
        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();

        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
        EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

        history = std::make_shared<History>(10, mockPersist.get());
        playbackState = std::make_shared<PlaybackState>();

        // Circular dependency handling: PlaybackController needs History
        playbackController = std::make_unique<PlaybackController>(mockEngine.get(), playbackState.get(), history.get());
        historyController = std::make_unique<HistoryController>(history.get(), playbackController.get());

        playlistManager = std::make_unique<PlaylistManager>(mockPersist.get());

        view = std::make_unique<HistoryView>(historyController.get(), history.get(), playbackController.get(),
                                             playlistManager.get());
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    // Helpers
    void startFrame()
    {
        ImGui::NewFrame();
        ImGui::Begin("Test Window");
    }

    void endFrame()
    {
        ImGui::End();
        ImGui::Render();
    }
    int getSelectedIndex()
    {
        return view->selectedIndex_;
    }

    void setSelectedIndex(int idx)
    {
        view->selectedIndex_ = idx;
    }

    ITrackListController *getListController()
    {
        return view->listController_;
    }

    // New Helpers for protected access
    void toggleEditMode() { view->toggleEditMode(); }
    bool isEditMode() const { return view->isEditMode_; }
    void selectAll(const std::vector<std::shared_ptr<MediaFile>>& tracks) { view->selectAll(tracks); }
    bool isSelected(const std::string& path) const { return view->isSelected(path); }
    void toggleSelection(const std::string& path) { view->toggleSelection(path); }
    void removeSelectedTracks() { view->removeSelectedTracks(); }
    size_t getSelectedCount() const { return view->selectedPaths_.size(); }
};

TEST_F(HistoryViewTest, Initialization)
{
    EXPECT_EQ(view->getHistory(), history.get());
    // Verify it set listController (base member)
    EXPECT_EQ(getListController(), historyController.get());
}

TEST_F(HistoryViewTest, UpdateResetsSelection)
{
    setSelectedIndex(5);
    EXPECT_EQ(getSelectedIndex(), 5);

    // Notify from history model
    history->addTrack(std::make_shared<MediaFile>("/new.mp3"));

    // View should update via Observer
    // But direct notification might be async or immediate depending on implementation.
    // History calls Subject::notify() synchronously in add().

    EXPECT_EQ(getSelectedIndex(), -1);
}

TEST_F(HistoryViewTest, DestructorDetaches)
{
    // Difficult to test directly without mocking History's detach,
    // but we can ensure no crash when destroying view before model.
    view.reset();

    // Trigger update on history, should not crash (dangling pointer check)
    history->addTrack(std::make_shared<MediaFile>("/another.mp3"));
}

TEST_F(HistoryViewTest, RenderBasic)
{
    // Add some tracks to history
    history->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history->addTrack(std::make_shared<MediaFile>("/song2.mp3"));

    startFrame();
    view->render();
    endFrame();
}

TEST_F(HistoryViewTest, RenderEditModeAndSelection)
{
    auto song1 = std::make_shared<MediaFile>("/song1.mp3");
    history->addTrack(song1);
    
    // 1. Toggle Edit Mode
    toggleEditMode();
    EXPECT_TRUE(isEditMode());

    // 2. Select All
    selectAll(history->getAll());
    EXPECT_TRUE(isSelected("/song1.mp3"));

    // 3. Render in Edit Mode
    startFrame();
    view->render();
    endFrame();

    // 4. Toggle back
    toggleEditMode();
    EXPECT_FALSE(isEditMode());
    EXPECT_EQ(getSelectedCount(), 0);
}

TEST_F(HistoryViewTest, RenderPopups)
{
    history->addTrack(std::make_shared<MediaFile>("/song1.mp3"));

    startFrame();
    // Force popups to open in next render
    ImGui::OpenPopup("MetadataPopup");
    ImGui::OpenPopup("AddToPlaylistPopup##0");

    view->render();
    endFrame();
}

TEST_F(HistoryViewTest, removeSelectedTracksTest)
{
    history->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    toggleEditMode();
    toggleSelection("/song1.mp3");
    
    EXPECT_EQ(history->getAll().size(), 1);
    removeSelectedTracks();
    EXPECT_EQ(history->getAll().size(), 0);
    EXPECT_EQ(getSelectedCount(), 0);
}

TEST_F(HistoryViewTest, RenderWithMarquee)
{
    // Add a song with a very long title to trigger marquee logic if hovered
    auto song = std::make_shared<MediaFile>("/very_long_title_song_that_exceeds_available_width_to_trigger_scrolling_logic.mp3");
    history->addTrack(song);

    startFrame();
    view->render();
    endFrame();
}

TEST_F(HistoryViewTest, HandleInput)
{
    // Hits HistoryView.cpp lines 47-50
    view->handleInput();
}

TEST_F(HistoryViewTest, RenderPlayingTrack)
{
    auto song1 = std::make_shared<MediaFile>("/playing.mp3");
    history->addTrack(song1);
    
    // Set as current track in state
    playbackState->setPlayback(song1, PlaybackStatus::PLAYING);
    
    startFrame();
    view->render();
    endFrame();
}

TEST_F(HistoryViewTest, RenderMetadataPopupDetails)
{
    auto song = std::make_shared<MediaFile>("/song.mp3");
    MediaMetadata meta;
    meta.title = "Title";
    meta.artist = "Artist";
    meta.album = "Album";
    meta.duration = 150;
    meta.bitrate = 320;
    meta.sampleRate = 44100;
    meta.channels = 2;
    meta.codec = "MP3";
    song->setMetadata(meta);
    history->addTrack(song);

    startFrame();
    ImGui::OpenPopup("MetadataPopup");
    view->render();
    endFrame();
}

TEST_F(HistoryViewTest, AddToPlaylistPopupLogic)
{
    history->addTrack(std::make_shared<MediaFile>("/song.mp3"));
    
    // Add some playlists to manager
    playlistManager->createPlaylist("Jazz");
    playlistManager->createPlaylist("Rock");

    startFrame();
    ImGui::OpenPopup("AddToPlaylistPopup##0");
    view->render();
    endFrame();
}

TEST_F(HistoryViewTest, RenderTrackListEmpty)
{
    history->clear();
    startFrame();
    view->render();
    endFrame();
}

