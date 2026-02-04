#include "../../inc/service/SDL2PlaybackEngine.h"
#include "../../inc/utils/Logger.h"
#include <algorithm>

#ifdef USE_SDL_MIXER
#include <SDL_mixer.h>
#endif

// Forward declaration of internal struct
struct SDL2PlaybackEngine::Impl {
#ifdef USE_SDL_MIXER
    Mix_Music* music = nullptr;
#endif
};

SDL2PlaybackEngine::SDL2PlaybackEngine() : impl_(std::make_unique<Impl>()) {
#ifdef USE_SDL_MIXER
    // Initialize SDL_mixer formats
    int flags = MIX_INIT_MP3 | MIX_INIT_FLAC | MIX_INIT_OGG;
    int initialized = Mix_Init(flags);
    if ((initialized & flags) != flags) {
        Logger::getInstance().warn("SDL_mixer failed to initialize some formats: " + std::string(Mix_GetError()));
    }

    // Initialize SDL_mixer audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        Logger::getInstance().error("SDL_mixer could not initialize! SDL_mixer Error: " + std::string(Mix_GetError()));
    } else {
        Logger::getInstance().info("SDL2PlaybackEngine initialized with SDL_mixer");
    }
#else
    Logger::getInstance().warn("SDL2PlaybackEngine initialized WITHOUT SDL_mixer (Dummy Mode)");
#endif
}

SDL2PlaybackEngine::~SDL2PlaybackEngine() {
#ifdef USE_SDL_MIXER
    if (impl_->music) {
        Mix_FreeMusic(impl_->music);
    }
    Mix_Quit();
#endif
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
#ifdef USE_SDL_MIXER
    if (impl_->music) {
        Mix_FreeMusic(impl_->music);
        impl_->music = nullptr;
    }

    impl_->music = Mix_LoadMUS(filepath.c_str());
    if (impl_->music == nullptr) {
        Logger::getInstance().error("Failed to load music! SDL_mixer Error: " + std::string(Mix_GetError()));
        return false;
    }

    if (Mix_PlayMusic(impl_->music, 1) == -1) {
        Logger::getInstance().error("SDL_mixer could not play music! SDL_mixer Error: " + std::string(Mix_GetError()));
        return false;
    }

    Logger::getInstance().info("Playing: " + filepath);
    notify();
    return true;
#else
    Logger::getInstance().info("Playing (Dummy): " + filepath);
    notify();
    return true;
#endif
}

void SDL2PlaybackEngine::pause() {
#ifdef USE_SDL_MIXER
    if (Mix_PlayingMusic() != 0) {
        if (Mix_PausedMusic() == 0) {
            Mix_PauseMusic();
            Logger::getInstance().info("Paused");
        }
    }
#else
    Logger::getInstance().info("Paused (Dummy)");
#endif
    notify();
}

void SDL2PlaybackEngine::resume() {
#ifdef USE_SDL_MIXER
    if (Mix_PlayingMusic() != 0) {
        if (Mix_PausedMusic() == 1) {
            Mix_ResumeMusic();
            Logger::getInstance().info("Resumed");
        }
    }
#else
    Logger::getInstance().info("Resumed (Dummy)");
#endif
    notify();
}

void SDL2PlaybackEngine::stop() {
#ifdef USE_SDL_MIXER
    Mix_HaltMusic();
#endif
    Logger::getInstance().info("Stopped");
    notify();
}

void SDL2PlaybackEngine::seek(double positionSeconds) {
#ifdef USE_SDL_MIXER
    if (impl_->music) {
        // Mix_SetMusicPosition expects seconds
        if (Mix_SetMusicPosition(positionSeconds) == -1) {
             Logger::getInstance().warn("Seek failed: " + std::string(Mix_GetError()));
        }
    }
#endif
    Logger::getInstance().info("Seeking to: " + std::to_string(positionSeconds));
}

void SDL2PlaybackEngine::setVolume(float volume) {
#ifdef USE_SDL_MIXER
    // Volume: 0 to 128
    int mixVolume = static_cast<int>(volume * 128); // Assuming volume is 0.0 - 1.0
    if (mixVolume < 0) mixVolume = 0;
    if (mixVolume > 128) mixVolume = 128;
    Mix_VolumeMusic(mixVolume);
#endif
    Logger::getInstance().info("Volume: " + std::to_string(volume));
}

PlaybackStatus SDL2PlaybackEngine::getState() const {
#ifdef USE_SDL_MIXER
    if (Mix_PlayingMusic() == 0) {
        return PlaybackStatus::STOPPED;
    } else {
        if (Mix_PausedMusic() == 1) {
            return PlaybackStatus::PAUSED;
        } else {
            return PlaybackStatus::PLAYING;
        }
    }
#else
    return PlaybackStatus::STOPPED;
#endif
}

double SDL2PlaybackEngine::getCurrentPosition() const {
    // SDL_mixer doesn't support getting exact position easily for all formats
    // This often requires custom music player logic or specific format handling
    return 0.0; 
}

double SDL2PlaybackEngine::getDuration() const {
    #ifdef USE_SDL_MIXER
     if (impl_->music) {
        // Mix_MusicDuration is not available in all SDL2_mixer versions
        // Return 0 for now or implement duration parsing via TagLib metadata interaction elsewhere
        return 0.0;
     }
    #endif
    return 0.0;
}

float SDL2PlaybackEngine::getVolume() const {
#ifdef USE_SDL_MIXER
    return (float)Mix_VolumeMusic(-1) / 128.0f;
#else
    return 1.0f;
#endif
}

bool SDL2PlaybackEngine::isFinished() const {
#ifdef USE_SDL_MIXER
    return (Mix_PlayingMusic() == 0);
#else
    return false;
#endif
}
