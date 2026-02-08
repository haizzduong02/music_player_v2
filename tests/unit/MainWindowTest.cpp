#include "app/view/MainWindow.h"
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
};

TEST_F(MainWindowTest, SwitchScreen)
{
    EXPECT_EQ(window->getCurrentScreen(), Screen::LIBRARY);

    window->switchScreen(Screen::HISTORY);
    EXPECT_EQ(window->getCurrentScreen(), Screen::HISTORY);

    window->switchScreen(Screen::PLAYLIST);
    EXPECT_EQ(window->getCurrentScreen(), Screen::PLAYLIST);
}

TEST_F(MainWindowTest, RenderAllScreens)
{
    // Render Library
    startFrame();
    window->render();
    endFrame();

    // Render History
    window->switchScreen(Screen::HISTORY);
    startFrame();
    window->render();
    endFrame();

    // Render Playlist
    window->switchScreen(Screen::PLAYLIST);
    startFrame();
    window->render();
    endFrame();
}

TEST_F(MainWindowTest, RenderWithFileBrowser)
{
    fileBrowserView->show();
    startFrame();
    window->render();
    endFrame();
}

TEST_F(MainWindowTest, RenderWithPlayingTrack)
{
    auto track = std::make_shared<MediaFile>("/test.mp3");
    playbackState->setPlayback(track, PlaybackStatus::PLAYING);
    
    startFrame();
    window->render();
    endFrame();
}

TEST_F(MainWindowTest, HandleInput)
{
    window->handleInput();
}
