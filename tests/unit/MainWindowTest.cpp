#include "app/view/MainWindow.h"
#include "interfaces/IMetadataReader.h"
#include "app/controller/LibraryController.h"
#include "app/controller/PlaybackController.h"
#include "app/controller/PlaylistController.h"
#include "app/model/History.h"
#include "app/model/Library.h"
#include "app/model/PlaybackState.h"
#include "app/model/PlaylistManager.h"
#include "app/view/FileBrowserView.h"
#include "app/view/HistoryView.h"
#include "app/view/LibraryView.h"
#include "app/view/NowPlayingView.h"
#include "app/view/PlaylistView.h"
#include "imgui.h"
#include "tests/mocks/MockFileSystem.h"
#include "tests/mocks/MockMetadataReader.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class MainWindowTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<NiceMock<MockFileSystem>> mockFs;
    std::shared_ptr<NiceMock<MockMetadataReader>> mockMeta;
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;

    std::shared_ptr<Library> library;
    std::shared_ptr<PlaylistManager> playlistManager;
    std::shared_ptr<History> history;
    std::shared_ptr<PlaybackState> playbackState;

    std::unique_ptr<LibraryController> libraryController;
    std::unique_ptr<PlaylistController> playlistController;
    std::unique_ptr<PlaybackController> playbackController;

    std::unique_ptr<LibraryView> libraryView;
    std::unique_ptr<PlaylistView> playlistView;
    std::unique_ptr<HistoryView> historyView;
    std::unique_ptr<NowPlayingView> nowPlayingView;
    std::unique_ptr<FileBrowserView> fileBrowserView;

    std::unique_ptr<MainWindow> window;

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

        mockPersist = std::make_shared<MockPersistence>();
        mockFs = std::make_shared<NiceMock<MockFileSystem>>();
        mockMeta = std::make_shared<NiceMock<MockMetadataReader>>();
        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();

        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));

        library = std::make_shared<Library>(mockPersist.get());
        playlistManager = std::make_shared<PlaylistManager>(mockPersist.get());
        history = std::make_shared<History>(10, mockPersist.get());
        playbackState = std::make_shared<PlaybackState>();

        playbackController = std::make_unique<PlaybackController>(mockEngine.get(), playbackState.get(), history.get());
        libraryController =
            std::make_unique<LibraryController>(library.get(), mockFs.get(), mockMeta.get(), playbackController.get());
        playlistController =
            std::make_unique<PlaylistController>(playlistManager.get(), library.get(), mockMeta.get());

        libraryView = std::make_unique<LibraryView>(libraryController.get(), library.get(), playbackController.get(),
                                                   playlistManager.get());
        playlistView =
            std::make_unique<PlaylistView>(playlistController.get(), playlistManager.get(), playbackController.get());
        historyView = std::make_unique<HistoryView>(nullptr, history.get(), playbackController.get(),
                                                   playlistManager.get());
        nowPlayingView = std::make_unique<NowPlayingView>(playbackController.get(), playbackState.get());
        fileBrowserView = std::make_unique<FileBrowserView>(mockFs.get(), libraryController.get());

        window = std::make_unique<MainWindow>();
        window->setLibraryView(libraryView.get());
        window->setPlaylistView(playlistView.get());
        window->setHistoryView(historyView.get());
        window->setNowPlayingView(nowPlayingView.get());
        window->setFileBrowserView(fileBrowserView.get());
        window->setPlaybackController(playbackController.get());
        window->setPlaybackState(playbackState.get());
        window->show();
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    void startFrame()
    {
        ImGui::NewFrame();
    }

    void endFrame()
    {
        ImGui::Render();
    }

    void simulateClick(ImVec2 pos)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = pos;
        io.MouseDown[0] = true;
        io.MouseClicked[0] = true;
        io.MouseReleased[0] = false;
    }

    void releaseClick()
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDown[0] = false;
        io.MouseClicked[0] = false;
        io.MouseReleased[0] = true;
    }
};

TEST_F(MainWindowTest, TabBarInteractions)
{
    // Try to click all three tabs via sweep
    for (float x = 20; x < 350; x += 20) {
        simulateClick(ImVec2(x, 15));
        startFrame();
        window->render();
        endFrame();
        releaseClick();
    }
}

TEST_F(MainWindowTest, RenderingStates)
{
    // 1. Video Rendering branch
    EXPECT_CALL(*mockEngine, getVideoTexture()).WillRepeatedly(Return((void*)0x1234));
    EXPECT_CALL(*mockEngine, getVideoSize(_, _)).WillRepeatedly(::testing::SetArgReferee<0>(1920));
    // Second arg is 1, let's use a nice number
    EXPECT_CALL(*mockEngine, getVideoSize(_, _)).WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<0>(1920), ::testing::SetArgReferee<1>(1080)));

    startFrame();
    window->render();
    endFrame();

    // 2. Album Art with data branch
    EXPECT_CALL(*mockEngine, getVideoTexture()).WillRepeatedly(Return(nullptr));
    auto track = std::make_shared<MediaFile>("/art.mp3");
    MediaMetadata meta;
    meta.hasAlbumArt = true;
    meta.albumArtData = {0x89, 0x50, 0x4E, 0x47}; // Fake PNG header
    track->setMetadata(meta);
    playbackState->setPlayback(track, PlaybackStatus::PLAYING);

    startFrame();
    window->render();
    endFrame();
    
    // 3. No Art Placeholder
    meta.hasAlbumArt = false;
    track->setMetadata(meta);
    startFrame();
    window->render();
    endFrame();
}

TEST_F(MainWindowTest, PopupRendering)
{
    // Test that popups are called for all screens
    window->switchScreen(Screen::LIBRARY);
    startFrame();
    window->render();
    endFrame();

    window->switchScreen(Screen::PLAYLIST);
    startFrame();
    window->render();
    endFrame();

    window->switchScreen(Screen::HISTORY);
    startFrame();
    window->render();
    endFrame();
}
