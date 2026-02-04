#include "../../inc/service/MpvPlaybackEngine.h"
#include "../../inc/utils/Logger.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <vector>

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
    
    // Enable video output but don't spawn a window (we render to texture)
    mpv_set_option_string(mpv_, "vo", "libmpv");

    if (mpv_initialize(mpv_) < 0) {
        throw std::runtime_error("Failed to initialize mpv");
    }
    
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
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    // FBO is not used in SW mode, but good to clean if it was somehow created
    if (fbo_ != 0) {
        typedef void (APIENTRY * PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint* framebuffers);
        PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
        if (glDeleteFramebuffers) glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }

    if (mpv_gl_) {
        mpv_render_context_free(mpv_gl_);
        mpv_gl_ = nullptr;
    }
    if (mpv_) {
        mpv_terminate_destroy(mpv_);
        mpv_ = nullptr;
    }
}

void MpvPlaybackEngine::updateVideo() {
    if (!mpv_gl_) return;

    // Process events
    while (true) {
        mpv_event* event = mpv_wait_event(mpv_, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        
        switch (event->event_id) {
            case MPV_EVENT_END_FILE:
                // Handle end of file
                notify();
                break;
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
            if (w <= 0 || h <= 0) return;
        }

        int width = (int)w;
        int height = (int)h;
        int stride = width * 4; // ARGB/RGBA usually 4 bytes per pixel

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
    const char* cmd[] = {"loadfile", filepath.c_str(), NULL};
    int res = mpv_command(mpv_, cmd);
    if (res < 0) {
        Logger::getInstance().error("mpv failed to load file");
        return false;
    }
    
    // Auto-play implicitly
    notify();
    return true;
}

void MpvPlaybackEngine::pause() {
    int flag = 1;
    mpv_set_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag);
    notify();
}

void MpvPlaybackEngine::resume() {
    int flag = 0;
    mpv_set_property(mpv_, "pause", MPV_FORMAT_FLAG, &flag);
    notify();
}

void MpvPlaybackEngine::stop() {
    const char* cmd[] = {"stop", NULL};
    mpv_command(mpv_, cmd);
    notify();
}

void MpvPlaybackEngine::seek(double positionSeconds) {
    // "seek" cmd: seek <target> [flags]
    std::string pos = std::to_string(positionSeconds);
    const char* cmd[] = {"seek", pos.c_str(), "absolute", NULL};
    mpv_command(mpv_, cmd);
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
    // Rudimentary check; "eof-reached" could be used
    int eof = 0;
    mpv_get_property(mpv_, "eof-reached", MPV_FORMAT_FLAG, &eof);
    return eof != 0;
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
