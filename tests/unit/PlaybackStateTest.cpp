#include "app/model/PlaybackState.h"
#include <gtest/gtest.h>

TEST(PlaybackStateTest, GetNextTrackAdvancesIndex)
{
    PlaybackState state;

    std::vector<std::shared_ptr<MediaFile>> queue;
    queue.push_back(std::make_shared<MediaFile>("1.mp3"));
    queue.push_back(std::make_shared<MediaFile>("2.mp3"));

    state.setPlayQueue(queue);

    // 1. Get first track (assuming start at -1 or 0 handling)
    // Generally getNextTrack usually implies "move to next".
    // If we just set queue, index might be 0.

    // Let's check internal logic implied by user.
    // If logic is "get current, then next", or "advance then get".
    // Assuming "advance then get" or typical iterator behavior.

    auto track1 = state.getNextTrack();
    ASSERT_NE(track1, nullptr);
    EXPECT_EQ(track1->getPath(), "1.mp3");

    auto track2 = state.getNextTrack();
    ASSERT_NE(track2, nullptr);
    EXPECT_EQ(track2->getPath(), "2.mp3");

    // 3. End of queue
    EXPECT_FALSE(state.hasNextTrack());
}

TEST(PlaybackStateTest, BackStackLogic)
{
    PlaybackState state;
    auto t1 = std::make_shared<MediaFile>("1.mp3");
    auto t2 = std::make_shared<MediaFile>("2.mp3");

    // Simulate playing T1, then T2
    state.setPlayback(t1, PlaybackStatus::PLAYING);
    state.pushToBackStack(); // Push T1

    state.setPlayback(t2, PlaybackStatus::PLAYING);

    // Pop back -> Should get T1
    auto prev = state.popFromBackStack();
    ASSERT_NE(prev, nullptr);
    EXPECT_EQ(prev->getPath(), "1.mp3");
}

TEST(PlaybackStateTest, SyncQueueIndex)
{
    PlaybackState state;
    auto t1 = std::make_shared<MediaFile>("1.mp3");
    auto t2 = std::make_shared<MediaFile>("2.mp3");
    auto t3 = std::make_shared<MediaFile>("3.mp3");

    std::vector<std::shared_ptr<MediaFile>> queue = {t1, t2, t3};
    state.setPlayQueue(queue);

    // We are playing t2 (pushed from somewhere else maybe)
    // We want to ensure next track is t3
    state.syncQueueIndex(t2);

    auto next = state.getNextTrack();
    ASSERT_NE(next, nullptr);
    EXPECT_EQ(next->getPath(), "3.mp3");
}

TEST(PlaybackStateTest, SetPlaybackNull)
{
    PlaybackState state;
    state.setPlayback(nullptr, PlaybackStatus::STOPPED);
    EXPECT_EQ(state.getCurrentTrack(), nullptr);
    EXPECT_EQ(state.getDuration(), 0.0);
}

TEST(PlaybackStateTest, ClampingLogic)
{
    PlaybackState state;
    state.setDuration(100.0);

    state.setVolume(1.5f);
    EXPECT_EQ(state.getVolume(), 1.0f);
    state.setVolume(-0.5f);
    EXPECT_EQ(state.getVolume(), 0.0f);

    state.setPosition(150.0);
    EXPECT_EQ(state.getPosition(), 100.0);
    state.setPosition(-10.0);
    EXPECT_EQ(state.getPosition(), 0.0);
}

TEST(PlaybackStateTest, BackStackEdgeCases)
{
    PlaybackState state;
    EXPECT_EQ(state.popFromBackStack(), nullptr);

    auto t1 = std::make_shared<MediaFile>("1.mp3");
    auto t2 = std::make_shared<MediaFile>("2.mp3");
    state.setPlayback(t1, PlaybackStatus::PLAYING);
    state.pushToBackStack();
    state.setPlayback(t2, PlaybackStatus::PLAYING);
    state.pushToBackStack();

    state.removeTrackFromBackStack("1.mp3");
    EXPECT_EQ(state.popFromBackStack(), t2);
    EXPECT_EQ(state.popFromBackStack(), nullptr);

    state.pushToBackStack(); // push t2
    state.clearBackStack();
    EXPECT_EQ(state.popFromBackStack(), nullptr);
}

TEST(PlaybackStateTest, QueueEdgeCases)
{
    PlaybackState state;
    EXPECT_EQ(state.getNextTrack(), nullptr);
    EXPECT_FALSE(state.hasNextTrack());

    auto t1 = std::make_shared<MediaFile>("1.mp3");
    state.setPlayQueue({t1});
    state.setQueueIndex(5); // Out of bounds, should be ignored or capped?
    // Logic says: if (index <= playQueue_.size()) queueIndex_ = index;
    // So 5 is > 1, ignored. index remains 0.
    EXPECT_EQ(state.getNextTrack(), t1);
    EXPECT_EQ(state.getNextTrack(), nullptr);

    state.clearPlayQueue();
    EXPECT_EQ(state.getNextTrack(), nullptr);
}

TEST(PlaybackStateTest, SyncQueueMissing)
{
    PlaybackState state;
    auto t1 = std::make_shared<MediaFile>("1.mp3");
    state.setPlayQueue({t1});

    state.syncQueueIndex(nullptr);                                    // No crash
    state.syncQueueIndex(std::make_shared<MediaFile>("missing.mp3")); // Not in queue
    EXPECT_EQ(state.getNextTrack(), t1);
}

TEST(PlaybackStateTest, BackStackNullCurrent)
{
    PlaybackState state;
    // currentTrack is null initially
    state.pushToBackStack();
    EXPECT_EQ(state.popFromBackStack(), nullptr);
}

TEST(PlaybackStateTest, QueueIndexLimit)
{
    PlaybackState state;
    auto t1 = std::make_shared<MediaFile>("1.mp3");
    state.setPlayQueue({t1});
    
    // Set to size (index 1) - this is valid for "next track is invalid"
    state.setQueueIndex(1);
    EXPECT_EQ(state.getNextTrack(), nullptr);
}

TEST(PlaybackStateTest, SetPlaybackEmptyPath)
{
    PlaybackState state;
    auto t = std::make_shared<MediaFile>("");
    state.setPlayback(t, PlaybackStatus::PLAYING);
    EXPECT_EQ(state.getCurrentTrack(), t);
}
