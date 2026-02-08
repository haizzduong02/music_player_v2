#include "app/controller/PlaylistTrackListController.h"
#include "app/controller/PlaybackController.h"
#include "app/model/Library.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include "tests/mocks/MockMetadataReader.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;

class PlaylistTrackListControllerTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPlaybackEngine> mockEngine;
    MockMetadataReader mockMetadataReader;
    std::unique_ptr<PlaybackController> playbackController;
    std::unique_ptr<PlaylistController> playlistController;
    std::shared_ptr<Playlist> playlist;
    std::unique_ptr<PlaylistManager> playlistManager;
    std::unique_ptr<History> history;
    std::unique_ptr<Library> library;

    void SetUp() override
    {
        mockEngine = std::make_shared<MockPlaybackEngine>();

        // Create PlaybackState
        auto playbackState = std::make_unique<PlaybackState>();

        // Create History
        history = std::make_unique<History>(50, nullptr);

        // Create PlaybackController
        playbackController =
            std::make_unique<PlaybackController>(mockEngine.get(), playbackState.release(), history.get());

        // Create Library
        library = std::make_unique<Library>(nullptr);

        // Create PlaylistManager
        playlistManager = std::make_unique<PlaylistManager>(nullptr);

        // Create PlaylistController
        playlistController = std::make_unique<PlaylistController>(playlistManager.get(), library.get(), &mockMetadataReader);

        // Create test playlist
        playlist = std::make_shared<Playlist>("TestPlaylist");
        playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
        playlist->addTrack(std::make_shared<MediaFile>("/song2.mp3"));
        playlist->addTrack(std::make_shared<MediaFile>("/song3.mp3"));
    }
};

// Constructor test
TEST_F(PlaylistTrackListControllerTest, Construction)
{
    PlaylistTrackListController controller(playlistController.get(), playlist, playbackController.get());
    // Should not crash
    SUCCEED();
}

// playTrack tests
TEST_F(PlaylistTrackListControllerTest, PlayTrackValid)
{
    PlaylistTrackListController controller(playlistController.get(), playlist, playbackController.get());

    auto tracks = playlist->getTracks();

    EXPECT_CALL(*mockEngine, play(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockEngine, getState()).WillRepeatedly(Return(PlaybackStatus::PLAYING));
    EXPECT_CALL(*mockEngine, getDuration()).WillRepeatedly(Return(180.0));

    controller.playTrack(tracks, 0);
    // Should have played the track
}

TEST_F(PlaylistTrackListControllerTest, PlayTrackNullPlaybackController)
{
    PlaylistTrackListController controller(playlistController.get(), playlist, nullptr);

    auto tracks = playlist->getTracks();

    // Should not crash with null playbackController
    EXPECT_NO_THROW(controller.playTrack(tracks, 0));
}

TEST_F(PlaylistTrackListControllerTest, PlayTrackNullPlaylist)
{
    PlaylistTrackListController controller(playlistController.get(), nullptr, playbackController.get());

    std::vector<std::shared_ptr<MediaFile>> tracks;
    tracks.push_back(std::make_shared<MediaFile>("/test.mp3"));

    // Should not crash with null playlist
    EXPECT_NO_THROW(controller.playTrack(tracks, 0));
}

// removeTracks tests
TEST_F(PlaylistTrackListControllerTest, RemoveTracksValid)
{
    // Add playlist to manager first
    playlistManager->createPlaylist("TestPlaylist");
    auto managedPlaylist = playlistManager->getPlaylist("TestPlaylist");
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song2.mp3"));

    PlaylistTrackListController controller(playlistController.get(), managedPlaylist, playbackController.get());

    std::set<std::string> paths = {"/song1.mp3"};
    controller.removeTracks(paths);

    // Verify track was removed
    EXPECT_EQ(managedPlaylist->size(), 1);
}

TEST_F(PlaylistTrackListControllerTest, RemoveTracksMultiple)
{
    playlistManager->createPlaylist("TestPlaylist");
    auto managedPlaylist = playlistManager->getPlaylist("TestPlaylist");
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song2.mp3"));
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song3.mp3"));

    PlaylistTrackListController controller(playlistController.get(), managedPlaylist, playbackController.get());

    std::set<std::string> paths = {"/song1.mp3", "/song3.mp3"};
    controller.removeTracks(paths);

    EXPECT_EQ(managedPlaylist->size(), 1);
}

TEST_F(PlaylistTrackListControllerTest, RemoveTracksNullController)
{
    PlaylistTrackListController controller(nullptr, playlist, playbackController.get());

    std::set<std::string> paths = {"/song1.mp3"};
    EXPECT_NO_THROW(controller.removeTracks(paths));
}

TEST_F(PlaylistTrackListControllerTest, RemoveTracksNullPlaylist)
{
    PlaylistTrackListController controller(playlistController.get(), nullptr, playbackController.get());

    std::set<std::string> paths = {"/song1.mp3"};
    EXPECT_NO_THROW(controller.removeTracks(paths));
}

// removeTrackByPath tests
TEST_F(PlaylistTrackListControllerTest, RemoveTrackByPathValid)
{
    playlistManager->createPlaylist("TestPlaylist");
    auto managedPlaylist = playlistManager->getPlaylist("TestPlaylist");
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song2.mp3"));

    PlaylistTrackListController controller(playlistController.get(), managedPlaylist, playbackController.get());

    controller.removeTrackByPath("/song1.mp3");

    EXPECT_EQ(managedPlaylist->size(), 1);
}

TEST_F(PlaylistTrackListControllerTest, RemoveTrackByPathNullController)
{
    PlaylistTrackListController controller(nullptr, playlist, playbackController.get());
    EXPECT_NO_THROW(controller.removeTrackByPath("/song1.mp3"));
}

TEST_F(PlaylistTrackListControllerTest, RemoveTrackByPathNullPlaylist)
{
    PlaylistTrackListController controller(playlistController.get(), nullptr, playbackController.get());
    EXPECT_NO_THROW(controller.removeTrackByPath("/song1.mp3"));
}

// clearAll tests
TEST_F(PlaylistTrackListControllerTest, ClearAllValid)
{
    playlistManager->createPlaylist("TestPlaylist");
    auto managedPlaylist = playlistManager->getPlaylist("TestPlaylist");
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song2.mp3"));
    managedPlaylist->addTrack(std::make_shared<MediaFile>("/song3.mp3"));

    PlaylistTrackListController controller(playlistController.get(), managedPlaylist, playbackController.get());

    controller.clearAll();

    EXPECT_EQ(managedPlaylist->size(), 0);
}

TEST_F(PlaylistTrackListControllerTest, ClearAllNullController)
{
    PlaylistTrackListController controller(nullptr, playlist, playbackController.get());
    EXPECT_NO_THROW(controller.clearAll());
}

TEST_F(PlaylistTrackListControllerTest, ClearAllNullPlaylist)
{
    PlaylistTrackListController controller(playlistController.get(), nullptr, playbackController.get());
    EXPECT_NO_THROW(controller.clearAll());
}

TEST_F(PlaylistTrackListControllerTest, ClearAllEmptyPlaylist)
{
    auto emptyPlaylist = std::make_shared<Playlist>("EmptyPlaylist");
    PlaylistTrackListController controller(playlistController.get(), emptyPlaylist, playbackController.get());

    EXPECT_NO_THROW(controller.clearAll());
}

