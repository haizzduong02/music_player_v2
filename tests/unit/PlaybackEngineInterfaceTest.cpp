#include <gtest/gtest.h>
#include "interfaces/IPlaybackEngine.h"

class DummyPlaybackEngine : public IPlaybackEngine {
public:
    bool play(const std::string& f) override { return true; }
    void pause() override {}
    void resume() override {}
    void stop() override {}
    void seek(double p) override {}
    void setVolume(float v) override {}
    PlaybackStatus getState() const override { return PlaybackStatus::STOPPED; }
    double getCurrentPosition() const override { return 0.0; }
    double getDuration() const override { return 0.0; }
    float getVolume() const override { return 1.0f; }
    bool isFinished() const override { return false; }
    
    // ISubject methods
    void attach(IObserver* observer) override {}
    void detach(IObserver* observer) override {}
    void notify() override {}
    
    // Do NOT override video methods to hit the base implementation
};

TEST(PlaybackEngineInterfaceTest, DefaultVideoImplementations) {
    DummyPlaybackEngine engine;
    EXPECT_EQ(engine.getVideoTexture(), nullptr);
    
    int w = -1, h = -1;
    engine.getVideoSize(w, h);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);
    
    engine.updateVideo(); // Should do nothing and not crash
}
