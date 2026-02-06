#include "../../inc/service/MpvPlaybackEngine.h"
#include "../../inc/utils/Logger.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <vector>
#include <thread>

struct MpvPlaybackEngine::Impl {
    // Hidden implementation if needed
};

// OpenGL extensions handling manually if needed, or rely on SDL's GL proc address
// For simplicity in this environment, using standard names assuming they are linked or pointers

static void* get_proc_address(void* ctx, const char* name) {
    return (void*)SDL_GL_GetProcAddress(name);
}

static void on_mpv_wakeup(void* ctx) {
    // Thread-safe wakeup - could push an event to SDL event loop,
    // but here we poll in updateVideo() so this might just signal a flag
}

MpvPlaybackEngine::MpvPlaybackEngine() : impl_(std::make_unique<Impl>()) {
    initMpv();
    initGL();
}

MpvPlaybackEngine::~MpvPlaybackEngine() {
    cleanup();
}

void MpvPlaybackEngine::initMpv() {
    mpv_ = mpv_create();
    if (!mpv_) {
        throw std::runtime_error("Failed to create mpv context");
    }

    mpv_set_option_string(mpv_, "terminal", "yes");
    mpv_set_option_string(mpv_, "msg-level", "all=v");
    mpv_set_option_string(mpv_, "vd-lavc-threads", "4");
    
    // Explicitly prioritize PulseAudio, then ALSA, then SDL
    mpv_set_option_string(mpv_, "ao", "pulse,alsa,sdl");
    mpv_set_option_string(mpv_, "audio-client-name", "MusicPlayer");
    
    // Enable video output but don't spawn a window (we render to texture)
    mpv_set_option_string(mpv_, "vo", "libmpv");

    Logger::getInstance().info("Calling mpv_initialize...");
    if (mpv_initialize(mpv_) < 0) {
        throw std::runtime_error("Failed to initialize mpv");
    }
    Logger::getInstance().info("mpv_initialize success");
    
    mpv_set_wakeup_callback(mpv_, on_mpv_wakeup, this);
    Logger::getInstance().info("MpvPlaybackEngine initialized");
}

void MpvPlaybackEngine::initGL() {
    // For software rendering, we don't need GL init params
    // We just initialize the SW render context
    
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_SW},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl_, mpv_, params) < 0) {
        Logger::getInstance().error("Failed to create mpv SW render context");
        return;
    }
    
    Logger::getInstance().info("Mpv SW render context initialized");
}

void MpvPlaybackEngine::cleanup() {
    // Release GL resources on main thread (must be done here)
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    
    if (fbo_ != 0) {
        typedef void (APIENTRY * PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint* framebuffers);
        PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
        if (glDeleteFramebuffers) glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }

    // Offload MPV destruction to detached thread to avoid blocking UI on audio timeouts
    if (mpv_) {
        // Capture pointers by value
        mpv_handle* mpv = mpv_;
        mpv_render_context* mpv_gl = mpv_gl_;
        
        mpv_ = nullptr;
        mpv_gl_ = nullptr;
        
        std::thread([mpv, mpv_gl]() {
            Logger::getInstance().info("Async cleanup thread started");
            
            if (mpv_gl) {
                Logger::getInstance().info("Freeing mpv render context (async)...");
                mpv_render_context_free(mpv_gl);
                Logger::getInstance().info("mpv render context freed (async)");
            }
            
            if (mpv) {
                Logger::getInstance().info("Terminating mpv core (async)...");
                mpv_terminate_destroy(mpv);
                Logger::getInstance().info("mpv core terminated (async)");
            }
        }).detach();
    }
    
    Logger::getInstance().info("MpvPlaybackEngine::cleanup finished (main thread)");
}

void MpvPlaybackEngine::updateVideo() {
    if (!mpv_gl_) return;

    // Process events
    while (true) {
        mpv_event* event = mpv_wait_event(mpv_, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        
        switch (event->event_id) {
            case MPV_EVENT_END_FILE: {
                mpv_event_end_file* end_file = (mpv_event_end_file*)event->data;
                if (end_file->reason == MPV_END_FILE_REASON_EOF) {
                    // Only auto-advance if file ended naturally
                    eofReached_ = true;
                    Logger::getInstance().info("MPV_EVENT_END_FILE (EOF) detected");
                    notify();
                } else {
                    Logger::getInstance().info("MPV_EVENT_END_FILE (Reason: " + std::to_string(end_file->reason) + ") - ignored");
                }
                break;
            }
            case MPV_EVENT_PROPERTY_CHANGE:
                break;
            default:
                break;
        }
    }

    // Check if we need to redraw
    uint64_t flags = mpv_render_context_update(mpv_gl_);
    if (flags & MPV_RENDER_UPDATE_FRAME) {
        // Get expected video size
        int64_t w = 0, h = 0;
        mpv_get_property(mpv_, "video-out-params/w", MPV_FORMAT_INT64, &w);
        mpv_get_property(mpv_, "video-out-params/h", MPV_FORMAT_INT64, &h);
        
        if (w <= 0 || h <= 0) {
            // Fallback or early exit
            mpv_get_property(mpv_, "video-params/w", MPV_FORMAT_INT64, &w);
            mpv_get_property(mpv_, "video-params/h", MPV_FORMAT_INT64, &h);
            if (w <= 0 || h <= 0) {
                Logger::getInstance().warn("Video update frame received but dimensions invalid: " + std::to_string(w) + "x" + std::to_string(h));
                return;
            }
        }

        int width = (int)w;
        int height = (int)h;
        int stride = width * 4; // ARGB/RGBA usually 4 bytes per pixel

        // Log occasionally
        static int frameCount = 0;
        if (frameCount++ % 300 == 0) {
             Logger::getInstance().debug("Rendering video frame: " + std::to_string(width) + "x" + std::to_string(height));
        }

        // Resize buffer if needed (using member vector would be better for performance, 
        // but for safety in this refactor we can just use a static or member if we had one.
        // For now, let's use a static buffer to avoid reallocation every frame)
        static std::vector<uint8_t> pixelBuffer;
        if (pixelBuffer.size() != (size_t)(stride * height)) {
            pixelBuffer.resize(stride * height);
        }

        // Render to memory buffer
        int size[] = {width, height};
        int pitch = stride;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_SW_SIZE, size},
            {MPV_RENDER_PARAM_SW_FORMAT, (void*)"rgba"},
            {MPV_RENDER_PARAM_SW_STRIDE, &pitch},
            {MPV_RENDER_PARAM_SW_POINTER, pixelBuffer.data()},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        if (mpv_render_context_render(mpv_gl_, params) >= 0) {
             // Upload to Texture
            if (videoWidth_ != width || videoHeight_ != height || texture_ == 0) {
                videoWidth_ = width;
                videoHeight_ = height;
                
                if (texture_ != 0) glDeleteTextures(1, &texture_);
                glGenTextures(1, &texture_);
                glBindTexture(GL_TEXTURE_2D, texture_);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            
            glBindTexture(GL_TEXTURE_2D, texture_);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());
            
            // Should report swap to mpv
            mpv_render_context_report_swap(mpv_gl_);
        } else {
             Logger::getInstance().error("mpv_render_context_render failed");
        }
    }
}

void* MpvPlaybackEngine::getVideoTexture() {
    if (texture_ == 0) return nullptr;
    return (void*)(intptr_t)texture_;
}

void MpvPlaybackEngine::getVideoSize(int& width, int& height) {
    width = videoWidth_;
    height = videoHeight_;
}

// Implement standard IPlaybackEngine methods
bool MpvPlaybackEngine::play(const std::string& filepath) {
    // Reset EOF flag when starting new file
    eofReached_ = false;
    
    const char* cmd[] = {"loadfile", filepath.c_str(), NULL};
    // Use async command to avoid blocking on audio drain (e.g. pulse timeout)
    int res = mpv_command_async(mpv_, 0, cmd);
    if (res < 0) {
        Logger::getInstance().error("mpv failed to load file (async)");
        return false;
    }
    
    // Resume if was paused
    int flag = 0;
    mpv_set_property_async(mpv_, 0, "pause", MPV_FORMAT_FLAG, &flag);
    return true;
}

void MpvPlaybackEngine::pause() {
    int flag = 1;
    mpv_set_property_async(mpv_, 0, "pause", MPV_FORMAT_FLAG, &flag);
}

void MpvPlaybackEngine::resume() {
    int flag = 0;
    mpv_set_property_async(mpv_, 0, "pause", MPV_FORMAT_FLAG, &flag);
    notify();
}

void MpvPlaybackEngine::stop() {
    const char* cmd[] = {"stop", NULL};
    mpv_command_async(mpv_, 0, cmd);
    notify();
}

void MpvPlaybackEngine::seek(double seconds) {
    std::string secStr = std::to_string(seconds);
    const char* cmd[] = {"seek", secStr.c_str(), "absolute", NULL};
    mpv_command_async(mpv_, 0, cmd);
}

void MpvPlaybackEngine::setVolume(float volume) {
    double vol = volume * 100.0;
    mpv_set_property(mpv_, "volume", MPV_FORMAT_DOUBLE, &vol);
}

PlaybackStatus MpvPlaybackEngine::getState() const {
    if (!mpv_) return PlaybackStatus::STOPPED;
    
    int paused = 0;
    mpv_get_property(mpv_, "pause", MPV_FORMAT_FLAG, &paused);
    
    int idle = 0;
    mpv_get_property(mpv_, "idle-active", MPV_FORMAT_FLAG, &idle);
    
    if (idle) return PlaybackStatus::STOPPED;
    return paused ? PlaybackStatus::PAUSED : PlaybackStatus::PLAYING;
}

double MpvPlaybackEngine::getCurrentPosition() const {
    double pos = 0;
    mpv_get_property(mpv_, "time-pos", MPV_FORMAT_DOUBLE, &pos);
    return pos;
}

double MpvPlaybackEngine::getDuration() const {
    double dur = 0;
    mpv_get_property(mpv_, "duration", MPV_FORMAT_DOUBLE, &dur);
    return dur;
}

float MpvPlaybackEngine::getVolume() const {
    double vol = 0;
    mpv_get_property(mpv_, "volume", MPV_FORMAT_DOUBLE, &vol);
    return (float)(vol / 100.0);
}

bool MpvPlaybackEngine::isFinished() const {
    // Check our tracked state - STRICTLY rely on event flag
    return eofReached_;
}

void MpvPlaybackEngine::attach(IObserver* observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    if (std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void MpvPlaybackEngine::detach(IObserver* observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
}

void MpvPlaybackEngine::notify() {
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto obs : observers_) obs->update(this);
}
