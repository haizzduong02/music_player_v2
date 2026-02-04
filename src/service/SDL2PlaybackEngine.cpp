#include "../../inc/service/SDL2PlaybackEngine.h"
#include "../../inc/utils/Logger.h"
#include <algorithm>

SDL2PlaybackEngine::SDL2PlaybackEngine() {
    Logger::getInstance().info("SDL2PlaybackEngine initialized");
}

SDL2PlaybackEngine::~SDL2PlaybackEngine() {
    Logger::getInstance().info("SDL2PlaybackEngine destroyed");
}

// ISubject implementation
void SDL2PlaybackEngine::attach(IObserver* observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void SDL2PlaybackEngine::detach(IObserver* observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end()
    );
}

void SDL2PlaybackEngine::notify() {
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto observer : observers_) {
        if (observer) {
            observer->update(this);
        }
    }
}

// IPlaybackEngine implementation
bool SDL2PlaybackEngine::play(const std::string& filepath) {
    Logger::getInstance().info("Playing: " + filepath);
    notify();
    return true;
}

void SDL2PlaybackEngine::pause() {
    Logger::getInstance().info("Paused");
    notify();
}

void SDL2PlaybackEngine::resume() {
    Logger::getInstance().info("Resumed");
    notify();
}

void SDL2PlaybackEngine::stop() {
    Logger::getInstance().info("Stopped");
    notify();
}

void SDL2PlaybackEngine::seek(double positionSeconds) {
    Logger::getInstance().info("Seeking to: " + std::to_string(positionSeconds));
}

void SDL2PlaybackEngine::setVolume(float volume) {
    Logger::getInstance().info("Volume: " + std::to_string(volume));
}

PlaybackStatus SDL2PlaybackEngine::getState() const {
    return PlaybackStatus::STOPPED;
}

double SDL2PlaybackEngine::getCurrentPosition() const {
    return 0.0;
}

double SDL2PlaybackEngine::getDuration() const {
    return 0.0;
}

float SDL2PlaybackEngine::getVolume() const {
    return 1.0f;
}

bool SDL2PlaybackEngine::isFinished() const {
    return false;
}
