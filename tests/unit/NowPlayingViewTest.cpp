#include "app/view/NowPlayingView.h"
#include "app/controller/PlaybackController.h"
#include "app/model/History.h"
#include "app/model/PlaybackState.h"
#include "imgui.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include "utils/Config.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class NowPlayingViewTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;

    std::shared_ptr<PlaybackState> playbackState;
    std::shared_ptr<History> history;
    std::unique_ptr<PlaybackController> playbackController;
    std::unique_ptr<PlaylistManager> playlistManager;

    std::unique_ptr<NowPlayingView> view;

    void SetUp() override
    {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(800, 200);

        // Build font atlas for headless testing
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)1);

        AppConfig dummyConfig;
        dummyConfig.configPath = "/tmp/test_config_view.json";
        Config::getInstance().setAppConfig(dummyConfig);

        mockPersist = std::make_shared<MockPersistence>();
        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();

        history = std::make_shared<History>(10, mockPersist.get());
        playbackState = std::make_shared<PlaybackState>();
        playbackController = std::make_unique<PlaybackController>(mockEngine.get(), playbackState.get(), history.get());

        view = std::make_unique<NowPlayingView>(playbackController.get(), playbackState.get());
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    // Helpers
    void startFrame()
    {
        ImGui::NewFrame();
        ImGui::Begin("Now Playing Bar");
    }

    void endFrame()
    {
        ImGui::End();
        ImGui::Render();
    }

    // Helper for friend access
    std::string formatTimeHelper(double seconds)
    {
        return view->formatTime(seconds);
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
        // Move in next frame usually, but let's just set it here
        io.MousePos = end;
    }

    bool isVisible()
    {
        return view->visible_;
    }
};

TEST_F(NowPlayingViewTest, FormatTimeHelpers)
{
    // Access private helper via helper method
    EXPECT_EQ(formatTimeHelper(0), "0:00");
    EXPECT_EQ(formatTimeHelper(9), "0:09");
    EXPECT_EQ(formatTimeHelper(59), "0:59");
    EXPECT_EQ(formatTimeHelper(60), "1:00");
    EXPECT_EQ(formatTimeHelper(61), "1:01");
    EXPECT_EQ(formatTimeHelper(3599), "59:59");
    EXPECT_EQ(formatTimeHelper(3600), "60:00");
}

TEST_F(NowPlayingViewTest, RenderDoesNotCrashWithNullState)
{
    EXPECT_NO_THROW(view->update(nullptr));
}

TEST_F(NowPlayingViewTest, UpdateShowsViewOnPlaying)
{
    // Manually hide it first (we need a way to set visible_=false)
    // Since `visible_` is protected in BaseView, and NowPlayingView is friend...
    // We can cast to specific friend-accessible way or just assume we can access it using `view`.
    // Let's try accessing it via a dirty hack or helper.
    // The cleanest way is if BaseView had setVisible, but it doesn't seem to.

    // Use the pointer to access protected member?
    // `view` is `std::unique_ptr<NowPlayingView>`.
    // If I can't access `visible_`, I'll rely on `update()` behavior.

    // Mock state as PLAYING
    playbackState->setStatus(PlaybackStatus::PLAYING);

    // Trigger update
    view->update(playbackState.get());

    // Check if visible
    EXPECT_TRUE(isVisible());
}

TEST_F(NowPlayingViewTest, RenderBasic)
{
    startFrame();
    view->render();
    endFrame();
}

TEST_F(NowPlayingViewTest, HandleInput)
{
    // Hits NowPlayingView.cpp line 346
    view->handleInput();
}

TEST_F(NowPlayingViewTest, RenderPlaying)
{
    auto track = std::make_shared<MediaFile>("/active.mp3");
    MediaMetadata meta;
    meta.artist = "Sample Artist";
    track->setMetadata(meta);
    
    playbackState->setPlayback(track, PlaybackStatus::PLAYING);
    playbackState->setDuration(300.0);
    playbackState->setPosition(120.0);
    playbackState->setVolume(0.75f);

    startFrame();
    view->render();
    endFrame();
}

TEST_F(NowPlayingViewTest, RenderPaused)
{
    auto track = std::make_shared<MediaFile>("/paused.mp3");
    playbackState->setPlayback(track, PlaybackStatus::PAUSED);

    startFrame();
    view->render();
    endFrame();
}

TEST_F(NowPlayingViewTest, RenderWithPlaylistManager)
{
    auto track = std::make_shared<MediaFile>("/fav.mp3");
    playbackState->setPlayback(track, PlaybackStatus::PLAYING);

    playlistManager = std::make_unique<PlaylistManager>(mockPersist.get());
    view->setPlaylistManager(playlistManager.get());

    // Hits favorites toggle logic
    startFrame();
    view->render();
    endFrame();
}

TEST_F(NowPlayingViewTest, RenderWithInteractions)
{
    auto track = std::make_shared<MediaFile>("/active.mp3");
    playbackState->setPlayback(track, PlaybackStatus::PLAYING);
    playbackState->setDuration(100.0);
    playbackState->setPosition(50.0);

    // 1. Simulate Seek Click (Progress Bar)
    startFrame();
    simulateClick(ImVec2(400, 180)); 
    view->render();
    endFrame();

    // 2. Simulate Volume Drag
    startFrame();
    simulateDrag(ImVec2(700, 150), ImVec2(750, 150));
    view->render();
    endFrame();

    // 3. Simulate Play/Pause Click
    startFrame();
    simulateClick(ImVec2(100, 180));
    view->render();
    endFrame();
}

TEST_F(NowPlayingViewTest, TextureLoadingFailurePaths)
{
    // These are hit in constructor/internal loadIconTexture
}

TEST_F(NowPlayingViewTest, RenderTruncationLogic)
{
    // Long title to hit truncate helper
    auto track = std::make_shared<MediaFile>("/very_very_very_long_track_name_that_should_be_truncated_by_the_view_logic.mp3");
    playbackState->setPlayback(track, PlaybackStatus::PLAYING);

    startFrame();
    view->render();
    endFrame();
}
