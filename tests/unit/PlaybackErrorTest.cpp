#include "app/controller/PlaybackController.h"
#include "app/model/MediaFile.h"
#include "app/model/PlaybackState.h"
#include "app/model/History.h"
#include "app/model/Playlist.h"      // Added based on compilation error feedback or expected dependencies
#include "tests/mocks/MockPlaybackEngine.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::Return;
using ::testing::_;

class PlaybackErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockEngine = new MockPlaybackEngine();
        playbackState = new PlaybackState();
        history = new History(10);
        // Ownership transferred to controller
        controller = std::make_unique<PlaybackController>(mockEngine, playbackState, history);
    }

    void TearDown() override {
        // mockEngine is NOT owned by controller in my implementation of PlaybackController constructor?
        // Let's check PlaybackController constructor signature:
        // PlaybackController(IPlaybackEngine *engine, PlaybackState *state, History *history, ...)
        // It takes pointers. It does NOT take ownership (usually).
        
        controller.reset();
        delete mockEngine;
        delete playbackState;
        delete history;
    }

    MockPlaybackEngine* mockEngine;
    PlaybackState* playbackState;
    History* history;
    std::unique_ptr<PlaybackController> controller;
};

TEST_F(PlaybackErrorTest, HandlesPlaybackError) {
    // Arrange
    bool callbackInvoked = false;
    std::string callbackPath;

    controller->setOnTrackLoadFailedCallback([&](const std::string& path) {
        callbackInvoked = true;
        callbackPath = path;
    });

    auto track = std::make_shared<MediaFile>("test.mp3");

    // Expect play call
    EXPECT_CALL(*mockEngine, play("test.mp3")).WillOnce(Return(true));
    
    // Act 1: Play
    controller->play(track);

    // Simulate Error State
    // The controller calls engine->getState() in update()
    EXPECT_CALL(*mockEngine, getState()).WillRepeatedly(Return(PlaybackStatus::ERROR));
    
    // Expect stop call
    EXPECT_CALL(*mockEngine, stop()).Times(1);

    // Act 2: Update (simulate loop)
    controller->update(nullptr);

    // Assert
    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(callbackPath, "test.mp3");
}
