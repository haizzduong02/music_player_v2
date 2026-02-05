#ifndef MPV_PLAYBACK_ENGINE_H
#define MPV_PLAYBACK_ENGINE_H

#include "../interfaces/IPlaybackEngine.h"
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <mpv/client.h>
#include <mpv/render_gl.h>

/**
 * @file MpvPlaybackEngine.h
 * @brief Playback engine using libmpv
 */

class MpvPlaybackEngine : public IPlaybackEngine {
public:
    MpvPlaybackEngine();
    ~MpvPlaybackEngine() override;

    // ISubject interface
    void attach(IObserver* observer) override;
    void detach(IObserver* observer) override;
    void notify() override;

    // IPlaybackEngine interface
    bool play(const std::string& filepath) override;
    void pause() override;
    void resume() override;
    void stop() override;
    void seek(double positionSeconds) override;
    void setVolume(float volume) override;

    PlaybackStatus getState() const override;
    double getCurrentPosition() const override;
    double getDuration() const override;
    float getVolume() const override;
    bool isFinished() const override;
    
    // Video support
    void* getVideoTexture() override;
    void getVideoSize(int& width, int& height) override;
    void updateVideo() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    mpv_handle* mpv_ = nullptr;
    mpv_render_context* mpv_gl_ = nullptr;
    
    // OpenGL state
    unsigned int fbo_ = 0;
    unsigned int texture_ = 0;
    int videoWidth_ = 0;
    int videoHeight_ = 0;
    bool eofReached_ = false;  // Track EOF for auto-advance
    
    std::vector<IObserver*> observers_;
    std::mutex observerMutex_;

    void initMpv();
    void initGL();
    void cleanup();
    void handleEvents();
    
    static void wakeupCallback(void* ctx);
};

#endif // MPV_PLAYBACK_ENGINE_H
