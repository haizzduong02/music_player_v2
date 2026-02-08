#include "app/controller/PlaybackController.h"
#include "app/model/History.h"
#include "app/model/PlaybackState.h"
#include "app/model/Playlist.h" // Needed for Playlist logic
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include "utils/Config.h" // Added for Config mock
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class PlaybackControllerTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<History> history;
    std::shared_ptr<PlaybackState> state;
    std::unique_ptr<PlaybackController> controller;

    void SetUp() override
    {
        // Mock Config to prevent overwriting real config
        AppConfig dummyConfig;
        dummyConfig.configPath = "/tmp/test_config.json";
        Config::getInstance().setAppConfig(dummyConfig);

        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();
        mockPersist = std::make_shared<MockPersistence>();

        // Mock persistence for History to avoid warnings
        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
        EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

        history = std::make_shared<History>(10, mockPersist.get());
        state = std::make_shared<PlaybackState>();
        controller = std::make_unique<PlaybackController>(mockEngine.get(), state.get(), history.get());
    }

    // Helper to access private method (Fixture is a friend)
    void triggerHandlePlaybackFinished()
    {
        controller->handlePlaybackFinished();
    }

    // Helper to reset throttle (Fixture is a friend)
    void resetThrottle()
    {
        controller->lastPlayTime_ = 0.0;
    }
};

TEST_F(PlaybackControllerTest, PlayStartsEngineAndUpdatesState)
{
    auto track = std::make_shared<MediaFile>("/song.mp3");

    EXPECT_CALL(*mockEngine, play("/song.mp3")).WillOnce(Return(true));

    bool result = controller->play(track);

    EXPECT_TRUE(result);
    EXPECT_EQ(state->getStatus(), PlaybackStatus::PLAYING);
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/song.mp3");
}

TEST_F(PlaybackControllerTest, PauseStopsEngineAndUpdatesState)
{
    EXPECT_CALL(*mockEngine, pause());

    controller->pause();
    EXPECT_EQ(state->getStatus(), PlaybackStatus::PAUSED);
}

TEST_F(PlaybackControllerTest, NextAdvancesQueue)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    auto t2 = std::make_shared<MediaFile>("/2.mp3");
    std::vector<std::shared_ptr<MediaFile>> queue = {t1, t2};
    state->setPlayQueue(queue);

    // Play t1
    EXPECT_CALL(*mockEngine, play("/1.mp3")).WillOnce(Return(true));
    controller->play(t1);

    // Next -> Should play t2
    EXPECT_CALL(*mockEngine, play("/2.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->next());
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/2.mp3");
}

TEST_F(PlaybackControllerTest, NextStopsAtEndOfQueue)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    state->setPlayQueue({t1});

    controller->play(t1);

    // Next -> Should fail/not play
    EXPECT_CALL(*mockEngine, play(_)).Times(0);
    EXPECT_FALSE(controller->next());
}

TEST_F(PlaybackControllerTest, HandlePlaybackFinishedAutoAdvances)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    auto t2 = std::make_shared<MediaFile>("/2.mp3");
    state->setPlayQueue({t1, t2});

    controller->play(t1);

    // Simulate finish -> Should trigger play(t2)
    EXPECT_CALL(*mockEngine, play("/2.mp3")).WillOnce(Return(true));

    triggerHandlePlaybackFinished();
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/2.mp3");
}

TEST_F(PlaybackControllerTest, PlaylistNavigationLoops)
{
    auto t1 = std::make_shared<MediaFile>("/p1.mp3");
    auto t2 = std::make_shared<MediaFile>("/p2.mp3");
    Playlist playlist("Test Playlist");
    playlist.addTrack(t1);
    playlist.addTrack(t2);
    playlist.setRepeatMode(RepeatMode::ALL);

    controller->setCurrentPlaylist(&playlist);

    // Play t2 (last track)
    EXPECT_CALL(*mockEngine, play("/p2.mp3")).WillOnce(Return(true));
    controller->play(t2);

    // Next -> Should loop to t1
    EXPECT_CALL(*mockEngine, play("/p1.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->next());
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/p1.mp3");
}

TEST_F(PlaybackControllerTest, ToggleRepeatMode)
{
    // Default NONE
    EXPECT_EQ(controller->getRepeatMode(), RepeatMode::NONE);

    // Toggle -> ONE
    controller->toggleRepeatMode();
    EXPECT_EQ(controller->getRepeatMode(), RepeatMode::ONE);

    // Toggle -> ALL
    controller->toggleRepeatMode();
    EXPECT_EQ(controller->getRepeatMode(), RepeatMode::ALL);

    // Toggle -> NONE
    controller->toggleRepeatMode();
    EXPECT_EQ(controller->getRepeatMode(), RepeatMode::NONE);
}

TEST_F(PlaybackControllerTest, HandlePlaybackFinishedRepeatOne)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    state->setPlayQueue({t1});
    controller->play(t1);

    controller->setRepeatMode(RepeatMode::ONE);

    // Reset throttle to ensure play is called
    resetThrottle();

    // Finish -> Should play t1 again
    EXPECT_CALL(*mockEngine, play("/1.mp3")).WillOnce(Return(true));

    triggerHandlePlaybackFinished();
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/1.mp3");
}

TEST_F(PlaybackControllerTest, PlayThrottling)
{
    auto track = std::make_shared<MediaFile>("/throttle.mp3");
    EXPECT_CALL(*mockEngine, play(_)).WillOnce(Return(true));

    EXPECT_TRUE(controller->play(track));
    // Second call within 500ms should be throttled
    EXPECT_TRUE(controller->play(track));
}

TEST_F(PlaybackControllerTest, PlayEngineFailure)
{
    auto track = std::make_shared<MediaFile>("/fail.mp3");
    EXPECT_CALL(*mockEngine, play("/fail.mp3")).WillOnce(Return(false));
    EXPECT_FALSE(controller->play(track));
}

TEST_F(PlaybackControllerTest, StopResetsPosition)
{
    state->setPosition(10.0);
    EXPECT_CALL(*mockEngine, stop());
    controller->stop();
    EXPECT_EQ(state->getPosition(), 0.0);
    EXPECT_EQ(state->getStatus(), PlaybackStatus::STOPPED);
}

TEST_F(PlaybackControllerTest, PreviousSeekOrBack)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    auto t2 = std::make_shared<MediaFile>("/2.mp3");

    // Test seek behavior (> 3s)
    MediaMetadata meta{};
    meta.duration = 10;
    t1->setMetadata(meta);

    Playlist p("dummy");
    p.addTrack(t1);
    controller->setCurrentPlaylist(&p);

    controller->play(t1);
    state->setPosition(5.0);
    EXPECT_CALL(*mockEngine, seek(0.0));
    EXPECT_TRUE(controller->previous());
    EXPECT_EQ(state->getPosition(), 0.0);

    // Test backstack behavior (< 3s)
    controller->setCurrentPlaylist(nullptr); // back to Library mode
    state->setPosition(1.0);
    state->pushToBackStack(); // push t1
    controller->play(t2);
    resetThrottle();

    EXPECT_CALL(*mockEngine, play("/1.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->previous());
    EXPECT_EQ(state->getCurrentTrack(), t1);
}

TEST_F(PlaybackControllerTest, PlayContextEdgeCases)
{
    controller->playContext({}, 0); // Empty context, no crash

    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    controller->playContext({t1}, 5); // Index out of bounds, no crash
}

TEST_F(PlaybackControllerTest, UpdateTimeAdvances)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    state->setPlayback(t1, PlaybackStatus::PLAYING);
    state->setDuration(100.0);
    state->setPosition(10.0);

    // Regular update
    EXPECT_CALL(*mockEngine, isFinished()).WillOnce(Return(false));
    controller->updateTime(0.5); // deltaTime=0.5
    EXPECT_EQ(state->getPosition(), 10.5);

    // Finish via engine
    EXPECT_CALL(*mockEngine, isFinished()).WillOnce(Return(true));
    // Since we don't have a next track, it will stop or stay.
    controller->updateTime(0.1);

    // Finish via time threshold
    state->setPosition(100.5);
    EXPECT_CALL(*mockEngine, isFinished()).WillOnce(Return(false));
    // Should trigger handlePlaybackFinished because 100.5 + 1.0 > 100.0 + 1.0
    controller->updateTime(1.0);
}

TEST_F(PlaybackControllerTest, GlobalRepeatAllQueue)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    state->setPlayQueue({t1});
    controller->setRepeatMode(RepeatMode::ALL); // Global mode

    controller->play(t1);
    state->setQueueIndex(1); // End of queue

    resetThrottle();
    EXPECT_CALL(*mockEngine, play("/1.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->next()); // Should loop back to start
}

TEST_F(PlaybackControllerTest, HandleFinishedRepeatOne)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    controller->setRepeatMode(RepeatMode::ONE);
    controller->play(t1);

    resetThrottle();
    EXPECT_CALL(*mockEngine, play("/1.mp3")).WillOnce(Return(true));

    // Trigger handlePlaybackFinished directly via friend access if possible,
    // or via updateTime with engine->isFinished()
    EXPECT_CALL(*mockEngine, isFinished()).WillOnce(Return(true));
    controller->updateTime(0.1);

    EXPECT_EQ(state->getCurrentTrack(), t1);
    EXPECT_EQ(state->getStatus(), PlaybackStatus::PLAYING);
}


TEST_F(PlaybackControllerTest, PlayWithoutStackPush)
{
    auto track = std::make_shared<MediaFile>("/song.mp3");
    EXPECT_CALL(*mockEngine, play("/song.mp3")).WillOnce(Return(true));

    // Ensure pushToStack logic is exercised (flag = false)
    controller->play(track, false); 
    // Verify stack size didn't increase if we could check it (state doesn't expose stack size easily, but we trust the coverage check)
}

TEST_F(PlaybackControllerTest, PlayWithNullHistory)
{
    // Re-init with null history
    controller = std::make_unique<PlaybackController>(mockEngine.get(), state.get(), nullptr);
    auto track = std::make_shared<MediaFile>("/song.mp3");
    EXPECT_CALL(*mockEngine, play("/song.mp3")).WillOnce(Return(true));
    
    EXPECT_TRUE(controller->play(track));
}

TEST_F(PlaybackControllerTest, NextTrackNotInPlaylist)
{
    // Setup: Playlist active, but current track is NOT in it
    Playlist playlist("Test");
    playlist.addTrack(std::make_shared<MediaFile>("/p1.mp3"));
    controller->setCurrentPlaylist(&playlist);
    
    auto tOutside = std::make_shared<MediaFile>("/outside.mp3");
    state->setPlayback(tOutside, PlaybackStatus::PLAYING);
    
    // Should restart playlist from index 0
    EXPECT_CALL(*mockEngine, play("/p1.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->next());
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/p1.mp3");
}

TEST_F(PlaybackControllerTest, PreviousAtStartNoLoop)
{
    // Playlist active, at index 0, Loop NONE
    auto t1 = std::make_shared<MediaFile>("/p1.mp3");
    Playlist playlist("Test");
    playlist.addTrack(t1);
    playlist.setRepeatMode(RepeatMode::NONE);
    controller->setCurrentPlaylist(&playlist);
    
    controller->play(t1);
    state->setPosition(1.0); // < 3s
    
    // reset throttle
    resetThrottle();
    
    // Should NOT play anything new (or replay same?)
    // Logic: prevIndex = -1. If no loop, prevIndex = 0.
    // So it plays index 0 (itself) again?
    // Code: if (prevIndex < 0) { ... else prevIndex=0 } -> play(tracks[0])
    EXPECT_CALL(*mockEngine, play("/p1.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->previous());
}

TEST_F(PlaybackControllerTest, PreviousEmptyBackStack)
{
    controller->setCurrentPlaylist(nullptr);
    // Backstack empty
    EXPECT_FALSE(controller->previous());
}

TEST_F(PlaybackControllerTest, UpdateTimeZeroDuration)
{
    auto t1 = std::make_shared<MediaFile>("/1.mp3");
    state->setPlayback(t1, PlaybackStatus::PLAYING);
    state->setDuration(0.0); // Unknown duration
    state->setPosition(0.0);
    
    EXPECT_CALL(*mockEngine, isFinished()).WillOnce(Return(false));
    
    // Should NOT trigger finish even if pos > 1.0 (condition is duration > 0)
    controller->updateTime(2.0); 
    // Position is clamped to duration (0.0) by PlaybackState
    EXPECT_EQ(state->getPosition(), 0.0);
    EXPECT_EQ(state->getStatus(), PlaybackStatus::PLAYING);
}

TEST_F(PlaybackControllerTest, NextEndPlaylistNoRepeat)
{
    auto t1 = std::make_shared<MediaFile>("/p1.mp3");
    Playlist playlist("Test");
    playlist.addTrack(t1);
    playlist.setRepeatMode(RepeatMode::NONE);
    controller->setCurrentPlaylist(&playlist);
    
    controller->play(t1);
    state->setQueueIndex(0); // Should be synced
    
    // play t1, next -> end of playlist -> return false
    EXPECT_FALSE(controller->next());
}

TEST_F(PlaybackControllerTest, PreviousStartPlaylistLoop)
{
    auto t1 = std::make_shared<MediaFile>("/p1.mp3");
    auto t2 = std::make_shared<MediaFile>("/p2.mp3");
    Playlist playlist("Test");
    playlist.addTrack(t1);
    playlist.addTrack(t2);
    playlist.setRepeatMode(RepeatMode::ALL); // Loop enabled
    controller->setCurrentPlaylist(&playlist);
    
    controller->play(t1);
    state->setPosition(1.0); // < 3s, so go to prev track
    
    // reset throttle
    resetThrottle();
    
    // prev of t1 is t2 (loop)
    EXPECT_CALL(*mockEngine, play("/p2.mp3")).WillOnce(Return(true));
    EXPECT_TRUE(controller->previous());
    EXPECT_EQ(state->getCurrentTrack()->getPath(), "/p2.mp3");
}

TEST_F(PlaybackControllerTest, SetRepeatModeWithPlaylist)
{
    Playlist playlist("Test");
    controller->setCurrentPlaylist(&playlist);
    
    controller->setRepeatMode(RepeatMode::ONE);
    EXPECT_EQ(playlist.getRepeatMode(), RepeatMode::ONE);
    
    // Global mode should be unaffected/default? Or we don't care.
    // Verify controller returns correct mode
    EXPECT_EQ(controller->getRepeatMode(), RepeatMode::ONE);
}

TEST_F(PlaybackControllerTest, HandleFinishedRepeatOneNoTrack)
{
    controller->setRepeatMode(RepeatMode::ONE);
    
    // State has no current track (e.g. stopped and cleared)
    state->setPlayback(nullptr, PlaybackStatus::STOPPED);
    
    // Should fall through to next() which returns false
    triggerHandlePlaybackFinished();
    
    EXPECT_EQ(state->getStatus(), PlaybackStatus::STOPPED);
}

TEST_F(PlaybackControllerTest, NullStateOrEngineNullChecks)
{
    // Test methods with null dependencies to hit safety branches
    auto nullCtrl = std::make_unique<PlaybackController>(nullptr, nullptr, nullptr);
    
    EXPECT_FALSE(nullCtrl->play(nullptr));
    nullCtrl->pause();
    nullCtrl->resume();
    nullCtrl->stop();
    nullCtrl->seek(1.0);
    nullCtrl->setVolume(0.5f);
    nullCtrl->updateTime(0.1);
    
    // Should not crash and should return false/early return
    SUCCEED();
}

TEST_F(PlaybackControllerTest, UpdateObserverDoesNothing)
{
    // Hit line 273 (empty update method)
    controller->update(nullptr);
    SUCCEED();
}

TEST_F(PlaybackControllerTest, SetRepeatModeNoPlaylist)
{
    controller->setCurrentPlaylist(nullptr);
    controller->setRepeatMode(RepeatMode::ALL);
    EXPECT_EQ(controller->getRepeatMode(), RepeatMode::ALL);
}

TEST_F(PlaybackControllerTest, SeekNullChecks)
{
    // Partial nulls
    auto partialCtrl = std::make_unique<PlaybackController>(mockEngine.get(), nullptr, nullptr);
    EXPECT_CALL(*mockEngine, seek(10.0));
    partialCtrl->seek(10.0);
    
    auto partialCtrl2 = std::make_unique<PlaybackController>(nullptr, state.get(), nullptr);
    state->setDuration(100.0);
    partialCtrl2->seek(20.0);
    EXPECT_EQ(state->getPosition(), 20.0);
}

TEST_F(PlaybackControllerTest, SetVolumeNullChecks)
{
    auto partialCtrl = std::make_unique<PlaybackController>(mockEngine.get(), nullptr, nullptr);
    EXPECT_CALL(*mockEngine, setVolume(0.5f));
    partialCtrl->setVolume(0.5f);
    
    auto partialCtrl2 = std::make_unique<PlaybackController>(nullptr, state.get(), nullptr);
    partialCtrl2->setVolume(0.8f);
    EXPECT_EQ(state->getVolume(), 0.8f);
}

TEST_F(PlaybackControllerTest, ResumeFunctional)
{
    EXPECT_CALL(*mockEngine, resume());
    controller->resume();
    EXPECT_EQ(state->getStatus(), PlaybackStatus::PLAYING);
}

TEST_F(PlaybackControllerTest, FindTrackIndexViaNextNullCheck)
{
    Playlist playlist("Test");
    controller->setCurrentPlaylist(&playlist);
    state->setPlayback(nullptr, PlaybackStatus::STOPPED);
    
    // next() calls findTrackIndexInPlaylist(nullptr) -> returns -1 -> goes to "!empty" check
    EXPECT_FALSE(controller->next());
}

TEST_F(PlaybackControllerTest, UpdateTimeFallbackFinish)
{
    auto track = std::make_shared<MediaFile>("/short.mp3");
    state->setPlayback(track, PlaybackStatus::PLAYING);
    state->setDuration(10.0);
    state->setPosition(11.1); // significantly past duration
    
    // reset throttle
    resetThrottle();
    
    // Next should be called
    EXPECT_CALL(*mockEngine, isFinished()).WillOnce(Return(false));
    // expect next() call - hardest to verify directly without more mocks, 
    // but we can check if it attempts to play something else or logs "Playback finished"
    
    controller->updateTime(0.5); // 11.1 + 0.5 = 11.6 > 11.0
}

TEST_F(PlaybackControllerTest, ToggleRepeatModeWithPlaylist)
{
    Playlist playlist("List");
    playlist.setRepeatMode(RepeatMode::NONE);
    controller->setCurrentPlaylist(&playlist);
    
    controller->toggleRepeatMode(); // NONE -> ONE
    EXPECT_EQ(playlist.getRepeatMode(), RepeatMode::ONE);
}
