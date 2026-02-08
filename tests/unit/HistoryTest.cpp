#include "app/model/History.h"
#include "tests/mocks/MockPersistence.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class HistoryTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;

    void SetUp() override
    {
        mockPersist = std::make_shared<MockPersistence>();
    }
};

TEST_F(HistoryTest, MovesExistingTrackToTop)
{
    // Expect load to be called on initialization
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    auto song1 = std::make_shared<MediaFile>("/song1.mp3");
    auto song2 = std::make_shared<MediaFile>("/song2.mp3");

    history.addTrack(song1);
    history.addTrack(song2);

    // Add song1 again -> Should move to index 0
    history.addTrack(song1);

    auto recent = history.getRecent(10);
    ASSERT_EQ(recent.size(), 2);                   // Size shouldn't grow to 3
    ASSERT_EQ(recent[0]->getPath(), "/song1.mp3"); // Most recent
    ASSERT_EQ(recent[1]->getPath(), "/song2.mp3");
}

TEST_F(HistoryTest, TrimsToMaxSize)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    size_t max = 3;
    History history(max, mockPersist.get());

    history.addTrack(std::make_shared<MediaFile>("/1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/2.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/3.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/4.mp3")); // Should push out /1.mp3

    auto recent = history.getRecent(10);
    ASSERT_EQ(recent.size(), 3);
    EXPECT_EQ(recent[0]->getPath(), "/4.mp3"); // Newest
    EXPECT_EQ(recent[2]->getPath(), "/2.mp3"); // Oldest (/1 was deleted)
}

TEST_F(HistoryTest, SavesOnAdd)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));

    // Expect saveToFile to be called when adding a track
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).Times(1).WillOnce(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/test.mp3"));
}

// Additional tests for better coverage

TEST_F(HistoryTest, AddNullTrack)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));

    History history(10, mockPersist.get());
    EXPECT_FALSE(history.addTrack(nullptr));
}

TEST_F(HistoryTest, AddTrackAlreadyAtTop)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    auto song1 = std::make_shared<MediaFile>("/song1.mp3");

    history.addTrack(song1);
    // Add same track again - already at top, should return true without modifying
    EXPECT_TRUE(history.addTrack(song1));

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 1);
}

TEST_F(HistoryTest, RemoveTrackByIndex)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song2.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song3.mp3"));

    EXPECT_TRUE(history.removeTrack(1)); // Remove middle track

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0]->getPath(), "/song3.mp3");
    EXPECT_EQ(recent[1]->getPath(), "/song1.mp3");
}

TEST_F(HistoryTest, RemoveTrackByInvalidIndex)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));

    EXPECT_FALSE(history.removeTrack(5));  // Out of bounds
    EXPECT_FALSE(history.removeTrack(10)); // Way out of bounds
}

TEST_F(HistoryTest, RemoveTrackByPath)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song2.mp3"));

    EXPECT_TRUE(history.removeTrackByPath("/song1.mp3"));

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0]->getPath(), "/song2.mp3");
}

TEST_F(HistoryTest, RemoveTrackByNonExistentPath)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));

    EXPECT_FALSE(history.removeTrackByPath("/nonexistent.mp3"));
}

TEST_F(HistoryTest, Clear)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song2.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song3.mp3"));

    history.clear();

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 0);
}

TEST_F(HistoryTest, GetTrackValidIndex)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song2.mp3"));

    auto track = history.getTrack(0);
    ASSERT_NE(track, nullptr);
    EXPECT_EQ(track->getPath(), "/song2.mp3"); // Most recent
}

TEST_F(HistoryTest, GetTrackInvalidIndex)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));

    History history(10, mockPersist.get());
    EXPECT_EQ(history.getTrack(0), nullptr);
    EXPECT_EQ(history.getTrack(100), nullptr);
}

TEST_F(HistoryTest, SetMaxSize)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song2.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song3.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song4.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song5.mp3"));

    // Reduce max size - should trim
    history.setMaxSize(2);

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 2);
}

TEST_F(HistoryTest, Contains)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));
    history.addTrack(std::make_shared<MediaFile>("/song2.mp3"));

    EXPECT_TRUE(history.contains("/song1.mp3"));
    EXPECT_TRUE(history.contains("/song2.mp3"));
    EXPECT_FALSE(history.contains("/song3.mp3"));
}

TEST_F(HistoryTest, Save)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillOnce(Return(true)).WillOnce(Return(true));

    History history(10, mockPersist.get());
    history.addTrack(std::make_shared<MediaFile>("/song1.mp3"));

    EXPECT_TRUE(history.save());
}

TEST_F(HistoryTest, SaveWithoutPersistence)
{
    History history(10, nullptr);
    EXPECT_FALSE(history.save());
}

TEST_F(HistoryTest, LoadSuccess)
{
    std::string historyJson = R"({
        "history": [
            {"filepath": "/loaded1.mp3", "metadata": {"title": "Loaded 1"}},
            {"filepath": "/loaded2.mp3", "metadata": {"title": "Loaded 2"}}
        ]
    })";

    EXPECT_CALL(*mockPersist, loadFromFile(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(historyJson), Return(true)));

    History history(10, mockPersist.get());
    EXPECT_TRUE(history.load());

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 2);
}

TEST_F(HistoryTest, LoadFailure)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillOnce(Return(false));

    History history(10, mockPersist.get());
    EXPECT_FALSE(history.load());
}

TEST_F(HistoryTest, LoadWithoutPersistence)
{
    History history(10, nullptr);
    EXPECT_FALSE(history.load());
}

TEST_F(HistoryTest, LoadMalformedJson)
{
    std::string malformedJson = "{ not valid json";

    EXPECT_CALL(*mockPersist, loadFromFile(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(malformedJson), Return(true)));

    History history(10, mockPersist.get());
    EXPECT_FALSE(history.load()); // Should handle exception gracefully
}

TEST_F(HistoryTest, LoadEmptyHistory)
{
    std::string emptyJson = R"({"history": []})";

    EXPECT_CALL(*mockPersist, loadFromFile(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(emptyJson), Return(true)));

    History history(10, mockPersist.get());
    EXPECT_TRUE(history.load());

    auto recent = history.getRecent(10);
    EXPECT_EQ(recent.size(), 0);
}

TEST_F(HistoryTest, GetRecentLimitedCount)
{
    EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

    History history(100, mockPersist.get());
    for (int i = 0; i < 10; ++i)
    {
        history.addTrack(std::make_shared<MediaFile>("/song" + std::to_string(i) + ".mp3"));
    }

    auto recent = history.getRecent(3);
    EXPECT_EQ(recent.size(), 3);
}

