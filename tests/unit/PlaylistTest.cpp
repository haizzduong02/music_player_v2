#include "app/model/Playlist.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;

class MockObserver : public IObserver
{
  public:
    MOCK_METHOD(void, update, (void *subject), (override));
};

class PlaylistTest : public ::testing::Test
{
  protected:
    std::shared_ptr<Playlist> playlist;
    std::shared_ptr<MockObserver> observer;

    void SetUp() override
    {
        playlist = std::make_shared<Playlist>("TestPlaylist");
        observer = std::make_shared<MockObserver>();
        playlist->attach(observer.get());
    }
};

TEST_F(PlaylistTest, AddTrack)
{
    EXPECT_CALL(*observer, update(playlist.get())).Times(1);
    
    auto track = std::make_shared<MediaFile>("/song1.mp3");
    EXPECT_TRUE(playlist->addTrack(track));
    
    EXPECT_EQ(playlist->size(), 1);
    EXPECT_TRUE(playlist->contains("/song1.mp3"));
}

TEST_F(PlaylistTest, AddDuplicateTrack)
{
    auto track = std::make_shared<MediaFile>("/song1.mp3");
    playlist->addTrack(track); // First add
    
    EXPECT_CALL(*observer, update(_)).Times(0);
    EXPECT_FALSE(playlist->addTrack(track)); // Duplicate add
    
    EXPECT_EQ(playlist->size(), 1);
}



TEST_F(PlaylistTest, InsertTrack)
{
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    playlist->addTrack(std::make_shared<MediaFile>("/song3.mp3"));

    EXPECT_CALL(*observer, update(playlist.get())).Times(1);
    
    auto track = std::make_shared<MediaFile>("/song2.mp3");
    EXPECT_TRUE(playlist->insertTrack(track, 1));
    
    EXPECT_EQ(playlist->size(), 3);
    EXPECT_EQ(playlist->getTrack(1)->getPath(), "/song2.mp3");
}

TEST_F(PlaylistTest, InsertTrackInvalidPosition)
{
    EXPECT_CALL(*observer, update(_)).Times(0);
    
    auto track = std::make_shared<MediaFile>("/song1.mp3");
    EXPECT_FALSE(playlist->insertTrack(track, 5)); // Empty playlist, pos 5 invalid
}

TEST_F(PlaylistTest, RemoveTrackByIndex)
{
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    playlist->addTrack(std::make_shared<MediaFile>("/song2.mp3"));
    
    EXPECT_CALL(*observer, update(playlist.get())).Times(1);
    EXPECT_TRUE(playlist->removeTrack(0));
    
    EXPECT_EQ(playlist->size(), 1);
    EXPECT_EQ(playlist->getTrack(0)->getPath(), "/song2.mp3");
}

TEST_F(PlaylistTest, RemoveTrackInvalidIndex)
{
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    
    EXPECT_CALL(*observer, update(_)).Times(0);
    EXPECT_FALSE(playlist->removeTrack(1));
}

TEST_F(PlaylistTest, RemoveTrackByPath)
{
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    
    EXPECT_CALL(*observer, update(playlist.get())).Times(1);
    EXPECT_TRUE(playlist->removeTrackByPath("/song1.mp3"));
    
    EXPECT_EQ(playlist->size(), 0);
}

TEST_F(PlaylistTest, RemoveTrackByInvalidPath)
{
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    
    EXPECT_CALL(*observer, update(_)).Times(0);
    EXPECT_FALSE(playlist->removeTrackByPath("/song2.mp3"));
}

TEST_F(PlaylistTest, Shuffle)
{
    for (int i = 0; i < 10; ++i) {
        playlist->addTrack(std::make_shared<MediaFile>("/song" + std::to_string(i) + ".mp3"));
    }
    
    std::vector<std::string> orderBefore;
    for (const auto& t : playlist->getTracks()) orderBefore.push_back(t->getPath());
    
    EXPECT_CALL(*observer, update(playlist.get())).Times(1);
    playlist->shuffle();
    
    std::vector<std::string> orderAfter;
    for (const auto& t : playlist->getTracks()) orderAfter.push_back(t->getPath());
    
    // There is a tiny chance shuffle results in same order, but unlikely for 10 items
    EXPECT_NE(orderBefore, orderAfter);
    EXPECT_EQ(orderBefore.size(), orderAfter.size());
}

TEST_F(PlaylistTest, ShuffleEmptyOrSingle)
{
    // Empty
    EXPECT_CALL(*observer, update(_)).Times(0);
    playlist->shuffle();
    ::testing::Mock::VerifyAndClearExpectations(observer.get());
    
    // Single
    EXPECT_CALL(*observer, update(_)).Times(1); // addTrack triggers update
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    ::testing::Mock::VerifyAndClearExpectations(observer.get());


    // Shuffle should NOT trigger update for single item
    EXPECT_CALL(*observer, update(_)).Times(0);
    playlist->shuffle();
}

TEST_F(PlaylistTest, AddNullTrack)
{
    EXPECT_FALSE(playlist->addTrack(nullptr));
    EXPECT_EQ(playlist->size(), 0);
}

TEST_F(PlaylistTest, InsertNullTrack)
{
    EXPECT_FALSE(playlist->insertTrack(nullptr, 0));
    EXPECT_EQ(playlist->size(), 0);
}

TEST_F(PlaylistTest, InsertInvalidPosition)
{
    auto track = std::make_shared<MediaFile>("/song1.mp3");
    EXPECT_TRUE(playlist->addTrack(track));
    
    // Position 2 is invalid (size is 1, max index 1 for insert? No, insert at end is size. Invalid is > size)
    EXPECT_FALSE(playlist->insertTrack(track, 2));
    EXPECT_EQ(playlist->size(), 1);
}

TEST_F(PlaylistTest, GetTrackInvalidIndex)
{
    EXPECT_EQ(playlist->getTrack(0), nullptr);
    
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    EXPECT_EQ(playlist->getTrack(1), nullptr);
}

TEST_F(PlaylistTest, FromJsonMissingKeys)
{
    nlohmann::json j_no_tracks = {{"name", "ZeroTracks"}};
    Playlist p("Temp");
    from_json(j_no_tracks, p);
    EXPECT_EQ(p.getName(), "ZeroTracks");
    EXPECT_EQ(p.size(), 0); // Should not crash
    
    nlohmann::json j_missing_name = {{"tracks", nlohmann::json::array()}};
    from_json(j_missing_name, p);
    EXPECT_EQ(p.getName(), "ZeroTracks"); // Name unchanged
}

TEST_F(PlaylistTest, Rename)
{
    EXPECT_CALL(*observer, update(playlist.get())).Times(1);
    playlist->rename("NewName");
    EXPECT_EQ(playlist->getName(), "NewName");
}

TEST_F(PlaylistTest, RepeatMode)
{
    EXPECT_EQ(playlist->getRepeatMode(), RepeatMode::NONE);
    EXPECT_FALSE(playlist->isLoopEnabled());
    
    playlist->setRepeatMode(RepeatMode::ALL);
    EXPECT_EQ(playlist->getRepeatMode(), RepeatMode::ALL);
    EXPECT_TRUE(playlist->isLoopEnabled());
    
    playlist->setRepeatMode(RepeatMode::ONE);
    EXPECT_EQ(playlist->getRepeatMode(), RepeatMode::ONE);
    EXPECT_TRUE(playlist->isLoopEnabled());
}

TEST_F(PlaylistTest, JsonSerialization)
{
    playlist->addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    playlist->setRepeatMode(RepeatMode::ALL);
    
    nlohmann::json j;
    to_json(j, *playlist);
    
    EXPECT_EQ(j["name"], "TestPlaylist");
    EXPECT_EQ(j["tracks"].size(), 1);
    EXPECT_EQ(j["tracks"][0]["path"], "/song1.mp3");
    
    // Deserialize
    Playlist p2("OldName");
    from_json(j, p2);
    
    EXPECT_EQ(p2.getName(), "TestPlaylist");
    EXPECT_EQ(p2.size(), 1);
    EXPECT_EQ(p2.getTrack(0)->getPath(), "/song1.mp3");
}
