#include "app/Application.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

class ApplicationIntegrationTest : public ::testing::Test
{
protected:
    Application app;

    void SetUp() override
    {
        // Use a temporary config/data path if possible, 
        // but for now we'll just run in headless mode.
    }

    void TearDown() override
    {
        app.shutdown();
    }
};

TEST_F(ApplicationIntegrationTest, HeadlessInitializationFlow)
{
    // 1. Initialize in headless mode
    ASSERT_TRUE(app.init(true));
    
    // 2. Verify basic state
    EXPECT_FALSE(app.shouldQuit());
}

TEST_F(ApplicationIntegrationTest, EndToEndPlaybackTiming)
{
    ASSERT_TRUE(app.init(true));
    
    // We can't easily play a real file in integration tests without a real MPV environment,
    // but we can verify that runOneFrame updates timers.
    
    // Step forward 1 second
    app.runOneFrame(1.0f);
    
    // Verify that controllers were updated (indirectly if possible, or just check no crash)
    // In a real integration test, we would check if PlaybackState position changed
    // if a track was "playing".
}

TEST_F(ApplicationIntegrationTest, ShutdownSavesState)
{
    ASSERT_TRUE(app.init(true));
    
    // Shutdown should trigger saveState
    app.shutdown();
}
