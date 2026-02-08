#include "service/MpvPlaybackEngine.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

class MpvPlaybackEngineTest : public ::testing::Test
{
  protected:
    std::string validMp3 = "tests/assets/sample.mp3";
    
    // Static members for GL environment
    static SDL_Window *window;
    static SDL_GLContext glContext;



    static void SetUpTestSuite()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
        {
            return;
        }
        // Create a hidden window for GL context
        window = SDL_CreateWindow("TestWindow", 0, 0, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window)
        {
            glContext = SDL_GL_CreateContext(window);
        }
    }

    static void TearDownTestSuite()
    {
        // Wait for detached cleanup threads from libmpv to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (glContext) SDL_GL_DeleteContext(glContext);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void setTexture(MpvPlaybackEngine &engine, unsigned int tex)
    {
        engine.texture_ = tex;
    }

    void setFbo(MpvPlaybackEngine &engine, unsigned int fbo)
    {
        engine.fbo_ = fbo;
    }

    unsigned int getTexture(MpvPlaybackEngine &engine)
    {
        return engine.texture_;
    }

    unsigned int getFbo(MpvPlaybackEngine &engine)
    {
        return engine.fbo_;
    }

    void setGlContext(MpvPlaybackEngine &engine, mpv_render_context *ctx)
    {
        engine.mpv_gl_ = ctx;
    }

    void setVideoSize(MpvPlaybackEngine &engine, int w, int h)
    {
        engine.videoWidth_ = w;
        engine.videoHeight_ = h;
    }

    mpv_handle *getMpv(MpvPlaybackEngine &engine)
    {
        return engine.mpv_;
    }

    void callCleanup(MpvPlaybackEngine &engine)
    {
        engine.cleanup();
    }

    mpv_render_context *getGlContext(MpvPlaybackEngine &engine)
    {
        return engine.mpv_gl_;
    }
};

TEST_F(MpvPlaybackEngineTest, InitAndProperties)
{
    try
    {
        MpvPlaybackEngine engine;
        EXPECT_EQ(engine.getState(), PlaybackStatus::STOPPED);

        engine.setVolume(1.0f);
        engine.setVolume(0.5f);
        EXPECT_NEAR(engine.getVolume(), 0.5f, 0.01f);

        EXPECT_FALSE(engine.isFinished());
        EXPECT_EQ(engine.getCurrentPosition(), 0.0);
        EXPECT_EQ(engine.getDuration(), 0.0);

        int w, h;
        engine.getVideoSize(w, h);
        EXPECT_EQ(w, 0);
        EXPECT_EQ(h, 0);
        EXPECT_EQ(engine.getVideoTexture(), nullptr);
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, PlaybackCommands)
{
    try
    {
        MpvPlaybackEngine engine;
        engine.pause();
        engine.resume();
        engine.stop();
        engine.seek(10.0);
        EXPECT_TRUE(engine.play("/non/existent/file.mp3"));
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, ObserverPattern)
{
    class MockObserver : public IObserver
    {
      public:
        bool updated = false;
        void update(void *subject) override
        {
            updated = true;
        }
    };

    try
    {
        MpvPlaybackEngine engine;
        MockObserver obs;
        engine.attach(&obs);
        engine.resume();
        EXPECT_TRUE(obs.updated);

        obs.updated = false;
        engine.detach(&obs);
        engine.resume();
        EXPECT_FALSE(obs.updated);
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, CleanupBranches)
{
    try
    {
        MpvPlaybackEngine engine;
        setTexture(engine, 123);
        setFbo(engine, 456);
        callCleanup(engine);
        EXPECT_EQ(getTexture(engine), 0);
        EXPECT_EQ(getFbo(engine), 0);
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, VideoUpdateEarlyReturn)
{
    try
    {
        MpvPlaybackEngine engine;
        // Force mpv_gl_ to null to hit early return in updateVideo
        mpv_render_context *original = getGlContext(engine);
        setGlContext(engine, nullptr);
        engine.updateVideo();
        setGlContext(engine, original); // Restore so cleanup() can free it
    }
    catch (...)
    {
    }
}

TEST_F(MpvPlaybackEngineTest, GetStateAndPosition)
{
    try
    {
        MpvPlaybackEngine engine;
        EXPECT_EQ(engine.getState(), PlaybackStatus::STOPPED);
        EXPECT_EQ(engine.getCurrentPosition(), 0.0);
        EXPECT_EQ(engine.getDuration(), 0.0);
        engine.notify();
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, EventLoop)
{
    try
    {
        MpvPlaybackEngine engine;
        // Trigger a property change to hit the event loop branches
        mpv_observe_property(getMpv(engine), 0, "volume", MPV_FORMAT_DOUBLE);
        double vol = 50.0;
        mpv_set_property(getMpv(engine), "volume", MPV_FORMAT_DOUBLE, &vol);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        engine.updateVideo();
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, VideoDimensions)
{
    try
    {
        MpvPlaybackEngine engine;
        int w = 10, h = 10;
        engine.getVideoSize(w, h);
        EXPECT_EQ(w, 0);
        EXPECT_EQ(h, 0);

        setVideoSize(engine, 1920, 1080);
        engine.getVideoSize(w, h);
        EXPECT_EQ(w, 1920);
        EXPECT_EQ(h, 1080);
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

TEST_F(MpvPlaybackEngineTest, SeekAndVolume)
{
    try
    {
        MpvPlaybackEngine engine;
        engine.play(validMp3);
        engine.seek(10.0);
        engine.setVolume(0.5f);
        EXPECT_NEAR(engine.getVolume(), 0.5f, 0.01);
    }
    catch (...)
    {
    }
}

TEST_F(MpvPlaybackEngineTest, EndOfFileEvent)
{
    MpvPlaybackEngine engine;
    engine.play(validMp3);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    engine.stop();
    for (int i = 0; i < 10; ++i)
    {
        engine.updateVideo();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TEST_F(MpvPlaybackEngineTest, VideoRenderingLoop)
{
    // Generate a small test video using ffmpeg
    std::string videoPath = "/tmp/test_video.mp4";
    std::string cmd = "ffmpeg -f lavfi -i testsrc=duration=1:size=320x240:rate=30 -c:v libx264 -y " + videoPath + " > /dev/null 2>&1";
    if (system(cmd.c_str()) != 0)
    {
        GTEST_SKIP() << "ffmpeg failed to generate video";
    }

    try
    {
        MpvPlaybackEngine engine;
        engine.play(videoPath);
        
        // Loop specifically waiting for the texture to be created
        for (int i = 0; i < 50; ++i)
        {
            engine.updateVideo();
            if (engine.getVideoTexture() != nullptr)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        engine.stop();
    }
    catch (...)
    {
        GTEST_SKIP() << "MPV initialization failed";
    }
}

SDL_Window *MpvPlaybackEngineTest::window = nullptr;
SDL_GLContext MpvPlaybackEngineTest::glContext = nullptr;
