# Threading Strategy & Ownership Documentation

## Thread Safety Strategy

### Thread Ownership Map

| Component | Owned By Thread | Access Pattern |
|-----------|----------------|----------------|
| **Models** | Main Thread | Write: Main, Read: All (locked) |
| **Views** | Main Thread (ImGui) | Main thread only |
| **Controllers** | Main Thread | Main thread only |
| **SDL2PlaybackEngine** | Playback Thread | Multi-threaded (mutex) |
| **S32K144Interface** | Listener Thread | Multi-threaded (mutex) |
| **Subject notifications** | Any Thread | Multi-threaded (mutex) |

### Synchronization Rules

#### 1. Subject Pattern (Observer Notifications)
```cpp
class Subject {
private:
    std::vector<IObserver*> observers_;
    mutable std::mutex mutex_;  // Protects observers_ list
    
    // Thread-safe: Can be called from ANY thread
    void notify() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto* obs : observers_) {
            obs->update(this);  // ⚠️ Crosses thread boundary
        }
    }
};
```

**Important**: Observer `update()` may execute on non-main thread!

#### 2. Model State Access
```cpp
class PlaybackState : public Subject {
private:
    // All state protected by Subject::mutex_
    // Access through getters/setters only
    
public:
    // Thread-safe write
    void setCurrentTrack(std::shared_ptr<MediaFile> track) {
        // Lock inherited from Subject
        currentTrack_ = track;
        notify();  // Locked notification
    }
    
    // Thread-safe read
    std::shared_ptr<MediaFile> getCurrentTrack() const {
        // Return shared_ptr for safe access
        return currentTrack_;
    }
};
```

#### 3. View Updates (ImGui Requirement)
```cpp
class NowPlayingView : public IObserver {
public:
    void update(void* subject) override {
        // ⚠️ May be called from playback/hardware thread!
        // ImGui operations MUST run on main thread
        
        // Strategy: Set dirty flag, update on next render()
        std::lock_guard<std::mutex> lock(updateMutex_);
        needsUpdate_ = true;
    }
    
    void render() override {
        // Called on main thread by ImGui
        if (needsUpdate_) {
            // Safe to update ImGui widgets here
            refreshUI();
            needsUpdate_ = false;
        }
    }
};
```

### Cross-Thread Communication

#### Pattern 1: Playback Thread → Main Thread
```
SDL2PlaybackEngine (playback thread)
  ↓ notify()
PlaybackState  
  ↓ update()
NowPlayingView (sets dirty flag)
  ↓ render() on next frame
ImGui (main thread) - ✅ SAFE
```

#### Pattern 2: Hardware Thread → Main Thread  
```
S32K144Interface (listener thread)
  ↓ notify()
PlaybackController::update()
  ↓ modify PlaybackState
PlaybackState::notify()
  ↓ update()
Views (set dirty flags)
  ↓ render()
ImGui (main thread) - ✅ SAFE
```

### Mutex Hierarchy (Prevent Deadlock)

1. **Level 1**: Subject::mutex_ (lowest)
2. **Level 2**: View update flags (higher)
3. **Level 3**: Application lifecycle (highest)

**Rule**: Always lock from highest to lowest, never reverse.

---

## Memory Ownership Strategy

### Application as Owner (Dependency Injection)

```cpp
class Application {
private:
    // Application OWNS everything via unique_ptr
    std::unique_ptr<SDL2PlaybackEngine> playbackEngine_;
    std::unique_ptr<LocalFileSystem> fileSystem_;
    std::unique_ptr<Library> library_;
    std::unique_ptr<LibraryController> libraryController_;
    std::unique_ptr<LibraryView> libraryView_;
    
public:
    bool init() {
        // 1. Create services
        playbackEngine_ = std::make_unique<SDL2PlaybackEngine>();
        fileSystem_ = std::make_unique<LocalFileSystem>();
        
        // 2. Create models
        library_ = std::make_unique<Library>(persistence_.get());
        
        // 3. Create controllers with INJECTED dependencies
        libraryController_ = std::make_unique<LibraryController>(
            library_.get(),         // ← NON-OWNING pointer
            fileSystem_.get(),      // ← NON-OWNING pointer
            metadataReader_.get()   // ← NON-OWNING pointer
        );
        
        // 4. Create views
        libraryView_ = std::make_unique<LibraryView>(
            libraryController_.get(),  // ← NON-OWNING pointer
            library_.get()             // ← NON-OWNING pointer
        );
        
        // 5. Wire observers
        library_->attach(libraryView_.get());
    }
    
    ~Application() {
        // Destruction order CRITICAL for safety:
        // 1. Views (observers) destroyed first
        libraryView_.reset();
        
        // 2. Then controllers
        libraryController_.reset();
        
        // 3. Finally models (subjects)
        library_.reset();
    }
};
```

### Ownership Rules

| Component Type | Ownership | Pointer Type |
|---------------|-----------|--------------|
| Services (SDL2, FileSystem) | Application owns | `unique_ptr` |
| Models (Library, Playlist) | Application owns | `unique_ptr` |
| Controllers | Application owns | `unique_ptr` |
| Views | Application owns | `unique_ptr` |
| **Dependencies in constructors** | **Non-owning refs** | **Raw pointer** |
| MediaFile in collections | Shared | `shared_ptr` |
| Observer registrations | Non-owning refs | Raw pointer |

### Why Raw Pointers for Dependency Injection?

```cpp
// ✅ CORRECT - Non-owning dependency
class LibraryController {
public:
    LibraryController(
        Library* library,           // ← Application owns, we just reference
        IFileSystem* fileSystem,    // ← Application owns
        IMetadataReader* reader     // ← Application owns
    ) : library_(library), 
        fileSystem_(fileSystem),
        metadataReader_(reader) {
        // No ownership transfer!
    }
    
private:
    Library* library_;              // ← NON-OWNING
    IFileSystem* fileSystem_;       // ← NON-OWNING
    IMetadataReader* metadataReader_; // ← NON-OWNING
};

// ❌ WRONG - Would imply ownership transfer
class LibraryController {
    std::unique_ptr<Library> library_;  // ❌ Who owns it now?
};

// ❌ WRONG - Shared ownership when not needed
class LibraryController {
    std::shared_ptr<Library> library_;  // ❌ Adds ref-counting overhead
};
```

### MediaFile Shared Ownership

```cpp
class Library {
private:
    // MediaFiles are SHARED between Library, Playlists, History
    std::vector<std::shared_ptr<MediaFile>> mediaFiles_;
    
public:
    void addMedia(std::shared_ptr<MediaFile> file) {
        mediaFiles_.push_back(file);  // Shared ownership
    }
};

class Playlist {
private:
    std::vector<std::shared_ptr<MediaFile>> tracks_;  // Same files, shared
};
```

**Benefit**: 
- MediaFile destroyed only when no Library/Playlist/History references it
- No dangling pointers
- Automatic cleanup

---

## Error Handling Guidelines

### When to Use bool Return

```cpp
// ✅ Simple success/failure
virtual bool play(const std::string& filepath) = 0;
virtual bool loadFromFile(const std::string& path, std::string& data) = 0;
virtual bool mountUSB(const std::string& device) = 0;
```

### When to Use Empty/Invalid Return

```cpp
// ✅ Can return "empty" value on error
MediaMetadata readMetadata(const std::string& filepath);  
// Returns MediaMetadata with empty strings on error

std::shared_ptr<MediaFile> getMediaFile(size_t index);
// Returns nullptr on invalid index
```

### When to Add getLastError()

```cpp
class SDL2PlaybackEngine {
public:
    bool play(const std::string& filepath);
    
    // For debugging/logging
    std::string getLastError() const { return lastError_; }
    
private:
    std::string lastError_;
};
```

### Exception Policy

**Do NOT throw exceptions across C/C++ library boundaries** (SDL2, TagLib)

```cpp
// ❌ WRONG
bool play(const std::string& filepath) {
    if (!fileExists(filepath)) {
        throw std::runtime_error("File not found");  // ❌ Avoid
    }
}

// ✅ CORRECT
bool play(const std::string& filepath) {
    if (!fileExists(filepath)) {
        lastError_ = "File not found";
        return false;  // ✅ Return error code
    }
}
```

---

## Implementation Checklist

### Before Writing .cpp Files

- [ ] Understand thread ownership from this document
- [ ] Know when to use locks (hint: when calling Subject::notify())
- [ ] Remember Views use dirty flags for cross-thread updates
- [ ] Never lock multiple mutexes (use mutex hierarchy)

### During Implementation

- [ ] Add logging to error paths
- [ ] Test with ThreadSanitizer (TSAN)
- [ ] Verify destruction order in Application
- [ ] Check observer detach in destructors

### Testing Strategy

- [ ] Unit tests: Mock interfaces with raw pointers ✅
- [ ] Integration tests: Full Application lifecycle
- [ ] Thread safety tests: Concurrent access
- [ ] Memory tests: Valgrind/LeakSanitizer

---

## Summary

✅ **Thread Safety**: Mutex in Subject protects all cross-thread notifications  
✅ **Ownership**: Application owns, components reference  
✅ **Smart Pointers**: Used correctly (unique for ownership, shared for MediaFile, raw for DI)  
✅ **Error Handling**: bool returns sufficient, add getLastError() for debugging  

**Architecture is SOLID and follows best practices!** 🎉
