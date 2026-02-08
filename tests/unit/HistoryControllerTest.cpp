#include "app/controller/HistoryController.h"
#include "app/controller/PlaybackController.h"
#include "app/model/History.h"
#include "app/model/PlaybackState.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include "utils/Config.h" // For Config mock
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class HistoryControllerTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;
    std::shared_ptr<History> history;
    std::shared_ptr<PlaybackState> state;
    std::shared_ptr<PlaybackController> playbackController;
    std::unique_ptr<HistoryController> controller;

    void SetUp() override
    {
        // Mock Config
        AppConfig dummyConfig;
        dummyConfig.configPath = "/tmp/test_config_history.json";
        Config::getInstance().setAppConfig(dummyConfig);

        mockPersist = std::make_shared<MockPersistence>();
        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();

        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
        EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

        history = std::make_shared<History>(10, mockPersist.get());
        state = std::make_shared<PlaybackState>();
        playbackController = std::make_shared<PlaybackController>(mockEngine.get(), state.get(), history.get());

        controller = std::make_unique<HistoryController>(history.get(), playbackController.get());
    }
};

TEST_F(HistoryControllerTest, AddToHistory)
{
    auto track = std::make_shared<MediaFile>("/history.mp3");
    controller->addToHistory(track);

    auto recent = controller->getRecentTracks(1);
    ASSERT_EQ(recent.size(), 1);
    EXPECT_EQ(recent[0]->getPath(), "/history.mp3");
}

TEST_F(HistoryControllerTest, ClearHistory)
{
    auto track = std::make_shared<MediaFile>("/h1.mp3");
    controller->addToHistory(track);
    ASSERT_FALSE(controller->getAllHistory().empty());

    controller->clearHistory();
    EXPECT_TRUE(controller->getAllHistory().empty());
}

TEST_F(HistoryControllerTest, PlayTrack)
{
    // Verify playing from history updates the playback controller
    auto t1 = std::make_shared<MediaFile>("/h1.mp3");
    std::vector<std::shared_ptr<MediaFile>> context = {t1};

    EXPECT_CALL(*mockEngine, play("/h1.mp3")).WillOnce(Return(true));

    controller->playTrack(context, 0);

    EXPECT_EQ(state->getStatus(), PlaybackStatus::PLAYING);
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/h1.mp3");
}
TEST_F(HistoryControllerTest, RemoveFromHistory)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    controller->addToHistory(t1);

    EXPECT_TRUE(controller->removeFromHistoryByPath("/1.mp3"));
    EXPECT_FALSE(controller->removeFromHistory(0));
}

TEST_F(HistoryControllerTest, BulkRemovals)
{
    controller->addToHistory(std::make_shared<MediaFile>("/1.mp3"));
    controller->addToHistory(std::make_shared<MediaFile>("/2.mp3"));

    controller->removeTracks({"/1.mp3"});
    EXPECT_EQ(controller->getAllHistory().size(), 1);

    controller->removeTrackByPath("/2.mp3");
    EXPECT_EQ(controller->getAllHistory().size(), 0);

    controller->addToHistory(std::make_shared<MediaFile>("/1.mp3"));
    controller->clearAll();
    EXPECT_EQ(controller->getAllHistory().size(), 0);
}

TEST_F(HistoryControllerTest, PlayTrackEdgeCases)
{
    auto t1 = std::make_shared<MediaFile>("/h1.mp3");
    std::vector<std::shared_ptr<MediaFile>> context = {t1};

    // Index out of bounds
    EXPECT_CALL(*mockEngine, play(_)).Times(0);
    controller->playTrack(context, 5);
}

TEST_F(HistoryControllerTest, NullHistory)
{
    HistoryController nullCtrl(nullptr, nullptr);
    // Actually controller calls history_->getRecent(count) directly. That's a bug in controller if history_ is null!
    
    // Should not crash (after fix if needed, but let's test current state)
    nullCtrl.addToHistory(nullptr);
    EXPECT_TRUE(nullCtrl.getRecentTracks(5).empty());
    EXPECT_FALSE(nullCtrl.removeFromHistory(0));
    EXPECT_FALSE(nullCtrl.removeFromHistoryByPath(""));
    nullCtrl.clearHistory();
    nullCtrl.playTrack({}, 0);
    nullCtrl.removeTracks({});
    nullCtrl.removeTrackByPath("");
    nullCtrl.clearAll();
    EXPECT_TRUE(nullCtrl.getAllHistory().empty());
}

TEST_F(HistoryControllerTest, PlayTrackNullPlaybackController)
{
    HistoryController hCtrl(history.get(), nullptr);
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    hCtrl.playTrack({t1}, 0); // Should not crash
    SUCCEED();
}
