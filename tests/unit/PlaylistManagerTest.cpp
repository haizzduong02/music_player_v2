#include "app/model/PlaylistManager.h"
#include "tests/mocks/MockPersistence.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

class PlaylistManagerTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;

    void SetUp() override
    {
        mockPersist = std::make_shared<MockPersistence>();
    }
};

TEST_F(PlaylistManagerTest, CannotDeleteSystemPlaylists)
{
    // Expect loading of existing playlists (none/empty)
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    PlaylistManager pm(mockPersist.get());

    // "Now Playing" and "Favorites" are created by default
    EXPECT_TRUE(pm.exists("Now Playing"));
    EXPECT_TRUE(pm.exists("Favorites"));

    EXPECT_FALSE(pm.deletePlaylist("Now Playing"));
    EXPECT_FALSE(pm.deletePlaylist("Favorites"));

    // Can create and delete normal one
    pm.createPlaylist("Gym");
    EXPECT_TRUE(pm.deletePlaylist("Gym"));
}

TEST_F(PlaylistManagerTest, RenameFailsIfNameExists)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    PlaylistManager pm(mockPersist.get());
    pm.createPlaylist("Rock");
    pm.createPlaylist("Jazz");

    // Try to rename Rock -> Jazz
    EXPECT_FALSE(pm.renamePlaylist("Rock", "Jazz"));

    // Helper to check if rename succeeded (it shouldn't)
    EXPECT_TRUE(pm.exists("Rock"));
}

TEST_F(PlaylistManagerTest, RenameSuccess)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    PlaylistManager pm(mockPersist.get());
    pm.createPlaylist("OldName");

    EXPECT_TRUE(pm.renamePlaylist("OldName", "NewName"));
    EXPECT_FALSE(pm.exists("OldName"));
    EXPECT_TRUE(pm.exists("NewName"));
}
TEST_F(PlaylistManagerTest, MigrationFromLegacyIndex)
{
    // 1. playlists.json exists but is an array of strings (legacy index)
    EXPECT_CALL(*mockPersist, fileExists("data/playlists.json")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockPersist, loadFromFile("data/playlists.json", _))
        .WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<1>("[\"Rock\", \"Jazz\"]"), Return(true)));

    // 2. Legacy per-playlist files exist
    EXPECT_CALL(*mockPersist, fileExists("data/playlist_Rock.json")).WillOnce(Return(true));
    EXPECT_CALL(*mockPersist, loadFromFile("data/playlist_Rock.json", _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>("{\"name\":\"Rock\",\"tracks\": [{\"path\": \"/rock1.mp3\"}]}"), Return(true)));

    EXPECT_CALL(*mockPersist, fileExists("data/playlist_Jazz.json")).WillOnce(Return(true));
    EXPECT_CALL(*mockPersist, loadFromFile("data/playlist_Jazz.json", _))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>("{\"name\":\"Jazz\",\"tracks\": []}"), Return(true)));

    // 3. Mock other system playlists files not existing
    EXPECT_CALL(*mockPersist, fileExists("data/playlist_Now Playing.json")).WillOnce(Return(false));
    EXPECT_CALL(*mockPersist, fileExists("data/playlist_Favorites.json")).WillOnce(Return(false));

    // 4. Persistence delete calls
    EXPECT_CALL(*mockPersist, deleteFile("data/playlist_Rock.json")).WillOnce(Return(true));
    EXPECT_CALL(*mockPersist, deleteFile("data/playlist_Jazz.json")).WillOnce(Return(true));

    // 5. Consolidated save call
    EXPECT_CALL(*mockPersist, saveToFile("data/playlists.json", _)).WillOnce(Return(true));

    PlaylistManager pm(mockPersist.get());
    EXPECT_TRUE(pm.loadAll());
    EXPECT_TRUE(pm.exists("Rock"));
    EXPECT_TRUE(pm.exists("Jazz"));
    EXPECT_EQ(pm.getPlaylist("Rock")->size(), 1);
}

TEST_F(PlaylistManagerTest, LoadNewFormatSuccess)
{
    EXPECT_CALL(*mockPersist, fileExists("data/playlists.json")).WillOnce(Return(true));
    EXPECT_CALL(*mockPersist, loadFromFile("data/playlists.json", _))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<1>("[{\"name\": \"Consolidated\", \"tracks\": []}]"),
                                   Return(true)));

    PlaylistManager pm(mockPersist.get());
    EXPECT_TRUE(pm.loadAll());
    EXPECT_TRUE(pm.exists("Consolidated"));
}

TEST_F(PlaylistManagerTest, LoadEmptyNewFormat)
{
    EXPECT_CALL(*mockPersist, fileExists("data/playlists.json")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockPersist, loadFromFile("data/playlists.json", _))
        .WillRepeatedly(::testing::DoAll(::testing::SetArgReferee<1>("[]"), Return(true)));

    // It should trigger migration (and likely fail if no legacy files)
    EXPECT_CALL(*mockPersist, fileExists("data/playlist_Now Playing.json")).WillOnce(Return(false));
    EXPECT_CALL(*mockPersist, fileExists("data/playlist_Favorites.json")).WillOnce(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillOnce(Return(true));

    PlaylistManager pm(mockPersist.get());
    EXPECT_TRUE(pm.loadAll());
}
