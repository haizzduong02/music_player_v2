#ifndef SDL2_PLAYBACK_ENGINE_H
#define SDL2_PLAYBACK_ENGINE_H

#include "interfaces/IPlaybackEngine.h"
#include <string>
#include <vector>
#include <mutex>

/**
 * @file SDL2PlaybackEngine.h
 * @brief Production playback engine using SDL2  
 */

#include <memory> 

class SDL2PlaybackEngine : public IPlaybackEngine {
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Observer pattern
    std::vector<IObserver*> observers_;
    std::mutex observerMutex_;
    
public:
    SDL2PlaybackEngine();
    ~SDL2PlaybackEngine() override;
    
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
};

#endif // SDL2_PLAYBACK_ENGINE_H
