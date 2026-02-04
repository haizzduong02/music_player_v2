# SDL2 và TagLib Usage trong Architecture

## Overview

**SDL2** và **TagLib** là 2 external libraries quan trọng trong hệ thống, được sử dụng thông qua **Dependency Inversion Principle** (DIP).

---

## 🎵 SDL2 (Simple DirectMedia Layer 2)

### Vai Trò
**Audio/Video Playback Engine** - Chịu trách nhiệm decode và phát media files.

### Vị Trí trong Architecture

```
┌─────────────────────────────────────────┐
│   IPlaybackEngine (Interface)           │ ← Abstract interface
├─────────────────────────────────────────┤
│   + play(filepath): bool                │
│   + pause(): void                       │
│   + resume(): void                      │
│   + stop(): void                        │
│   + getState(): PlaybackStatus          │
└─────────────────────────────────────────┘
                 ▲
                 │ implements
                 │
┌─────────────────────────────────────────┐
│   SDL2PlaybackEngine                    │ ← SDL2 implementation
├─────────────────────────────────────────┤
│   - playbackThread: thread              │
│   - state: PlaybackStatus               │
│   - mutex: mutex                        │
├─────────────────────────────────────────┤
│   + play(): đọc file → SDL2 decode      │
│   + pause(): SDL2_PauseAudio()          │
│   + stop(): cleanup SDL resources       │
└─────────────────────────────────────────┘
```

### File Location
📁 [`inc/service/SDL2PlaybackEngine.h`](file:///wsl.localhost/Ubuntu/home/duong/music_player/inc/service/SDL2PlaybackEngine.h)

### Responsibilities

1. **Initialize SDL2 subsystems**
   ```cpp
   bool SDL2PlaybackEngine::initialize() {
       SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
       // Setup audio specifications
   }
   ```

2. **Decode và stream audio**
   ```cpp
   bool SDL2PlaybackEngine::play(const std::string& filepath) {
       // SDL2 opens file, decodes format (mp3, flac, wav, etc.)
       // Streams to audio device in background thread
   }
   ```

3. **Control playback**
   - `pause()` → `SDL_PauseAudio(1)`
   - `resume()` → `SDL_PauseAudio(0)`
   - `setVolume()` → SDL mixer volume control

4. **Thread management**
   - Playback runs in separate `std::thread`
   - Non-blocking operations
   - Notifies observers via `Subject::notify()`

### Dependencies trong Code

**Header includes** (trong .cpp file):
```cpp
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_mixer.h>  // For audio mixing
```

### Được Sử Dụng Bởi

```
PlaybackController
    ↓ depends on
IPlaybackEngine* (interface)
    ↓ injected as
SDL2PlaybackEngine (concrete)
```

**Dependency Injection trong Application**:
```cpp
class Application {
private:
    std::unique_ptr<SDL2PlaybackEngine> playbackEngine_;
    std::unique_ptr<PlaybackController> playbackController_;
    
public:
    bool init() {
        // 1. Create SDL2 implementation
        playbackEngine_ = std::make_unique<SDL2PlaybackEngine>();
        playbackEngine_->initialize();
        
        // 2. Inject into controller
        playbackController_ = std::make_unique<PlaybackController>(
            playbackEngine_.get(),  // ← SDL2PlaybackEngine injected as IPlaybackEngine*
            playbackState_.get(),
            history_.get(),
            hardware_.get()
        );
    }
};
```

### Supported Formats (via SDL2_mixer)
- **Audio**: MP3, FLAC, WAV, OGG, AAC
- **Video**: MP4, AVI, MKV (via SDL2 video subsystem)

### Threading Model
```
Main Thread (GUI)
    ↓ calls
PlaybackController::play()
    ↓ calls
SDL2PlaybackEngine::play()
    ↓ spawns
Playback Thread
    └─ SDL2 audio callback
       └─ Decodes & streams audio
       └─ Updates position/duration
       └─ Subject::notify() → Views
```

---

## 🏷️ TagLib (Audio Metadata Library)

### Vai Trò
**Metadata Reader/Writer** - Đọc và ghi metadata (ID3, APE tags, etc.) từ audio files.

### Vị Trí trong Architecture

```
┌─────────────────────────────────────────┐
│   IMetadataReader (Interface)           │ ← Abstract interface
├─────────────────────────────────────────┤
│   + readMetadata(filepath)              │
│   + writeMetadata(filepath, data)       │
│   + extractTags(filepath, tags)         │
│   + supportsEditing(filepath)           │
└─────────────────────────────────────────┘
                 ▲
                 │ implements
                 │
┌─────────────────────────────────────────┐
│   TagLibMetadataReader                  │ ← TagLib implementation
├─────────────────────────────────────────┤
│   + readMetadata(): uses TagLib API    │
│   + writeMetadata(): writes ID3/APE    │
│   - isFormatSupported(): checks ext    │
└─────────────────────────────────────────┘
```

### File Location
📁 [`inc/service/TagLibMetadataReader.h`](file:///wsl.localhost/Ubuntu/home/duong/music_player/inc/service/TagLibMetadataReader.h)

### Responsibilities

1. **Read metadata từ audio files**
   ```cpp
   MediaMetadata TagLibMetadataReader::readMetadata(const std::string& filepath) {
       TagLib::FileRef file(filepath.c_str());
       
       MediaMetadata meta;
       meta.title = file.tag()->title().to8Bit();
       meta.artist = file.tag()->artist().to8Bit();
       meta.album = file.tag()->album().to8Bit();
       meta.year = file.tag()->year();
       meta.track = file.tag()->track();
       meta.duration = file.audioProperties()->length();
       meta.bitrate = file.audioProperties()->bitrate();
       meta.sampleRate = file.audioProperties()->sampleRate();
       meta.channels = file.audioProperties()->channels();
       
       return meta;
   }
   ```

2. **Write metadata to audio files**
   ```cpp
   bool TagLibMetadataReader::writeMetadata(
       const std::string& filepath, 
       const MediaMetadata& metadata
   ) {
       TagLib::FileRef file(filepath.c_str());
       
       file.tag()->setTitle(metadata.title);
       file.tag()->setArtist(metadata.artist);
       file.tag()->setAlbum(metadata.album);
       file.tag()->setYear(metadata.year);
       file.tag()->setTrack(metadata.track);
       
       return file.save();
   }
   ```

3. **Extract specific tags**
   ```cpp
   std::map<std::string, std::string> extractTags(
       const std::string& filepath,
       const std::vector<std::string>& tags  // e.g., ["ARTIST", "TITLE"]
   );
   ```

4. **Format support check**
   ```cpp
   bool TagLibMetadataReader::isFormatSupported(const std::string& ext) {
       return ext == ".mp3" || ext == ".flac" || 
              ext == ".ogg" || ext == ".m4a";
   }
   ```

### Dependencies trong Code

**Header includes** (trong .cpp file):
```cpp
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/id3v2tag.h>
#include <taglib/apetag.h>
```

### Được Sử Dụng Bởi

```
LibraryController
    ↓ depends on
IMetadataReader* (interface)
    ↓ injected as
TagLibMetadataReader (concrete)
```

**Dependency Injection trong Application**:
```cpp
class Application {
private:
    std::unique_ptr<TagLibMetadataReader> metadataReader_;
    std::unique_ptr<LibraryController> libraryController_;
    
public:
    bool init() {
        // 1. Create TagLib implementation
        metadataReader_ = std::make_unique<TagLibMetadataReader>();
        
        // 2. Inject into controller
        libraryController_ = std::make_unique<LibraryController>(
            library_.get(),
            fileSystem_.get(),
            metadataReader_.get()  // ← TagLibMetadataReader injected
        );
    }
};
```

### Supported Tag Formats
- **ID3v1** - MP3 legacy tags
- **ID3v2** - MP3 modern tags (v2.3, v2.4)
- **APE** - APE/Musepack tags
- **Vorbis Comments** - OGG/FLAC tags
- **MP4** - M4A/AAC metadata

### Usage Flow
```
User adds file to Library
    ↓
LibraryController::addMediaFilesFromDirectory()
    ↓
For each file:
    ↓
FileSystem scans → finds "song.mp3"
    ↓
MetadataReader::readMetadata("song.mp3")
    ↓
TagLib opens file
    ↓
TagLib reads ID3 tags
    ↓
Returns MediaMetadata{
    title: "Song Title",
    artist: "Artist Name",
    album: "Album",
    ...
}
    ↓
MediaFile object created with metadata
    ↓
Added to Library
```

---

## 🏗️ Dependency Inversion Principle (DIP)

### Why Interfaces?

Cả SDL2 và TagLib đều được **abstracted behind interfaces** để:

1. **Testability** - Mock cho unit tests
   ```cpp
   class MockPlaybackEngine : public IPlaybackEngine {
       // Fake implementation for testing
   };
   
   class MockMetadataReader : public IMetadataReader {
       // Returns hardcoded metadata for tests
   };
   ```

2. **Flexibility** - Có thể swap implementations
   ```cpp
   // Future: Switch to FFmpeg instead of SDL2
   class FFmpegPlaybackEngine : public IPlaybackEngine { ... };
   
   // Future: Use libav instead of TagLib
   class LibavMetadataReader : public IMetadataReader { ... };
   ```

3. **Decoupling** - Controllers không depend on concrete libraries
   ```cpp
   // ✅ GOOD - Depends on abstraction
   class PlaybackController {
   private:
       IPlaybackEngine* engine_;  // Can be SDL2, FFmpeg, mock, etc.
   };
   
   // ❌ BAD - Direct dependency on SDL2
   class PlaybackController {
   private:
       SDL2PlaybackEngine* engine_;  // Tightly coupled!
   };
   ```

---

## 📦 Dependency Graph

```
┌─────────────────────┐
│   Application       │
│   (DI Container)    │
└──────────┬──────────┘
           │
           ├─ creates → SDL2PlaybackEngine
           ├─ creates → TagLibMetadataReader
           ├─ creates → Controllers
           └─ injects dependencies
                    ↓
        ┌──────────────────────┐
        │  PlaybackController  │
        │  LibraryController   │
        └──────────────────────┘
                    ↓ uses
        ┌──────────────────────┐
        │  IPlaybackEngine*    │ ────→ SDL2PlaybackEngine
        │  IMetadataReader*    │ ────→ TagLibMetadataReader
        └──────────────────────┘
```

---

## 🔧 Build Dependencies

### CMakeLists.txt (example)

```cmake
# Find SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Find TagLib
find_package(TagLib REQUIRED)

# Add includes
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${TAGLIB_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(music_player
    ${SDL2_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${TAGLIB_LIBRARIES}
)
```

### Install (Ubuntu/Linux)

```bash
# SDL2
sudo apt install libsdl2-dev libsdl2-mixer-dev

# TagLib
sudo apt install libtag1-dev
```

---

## 📊 Summary Table

| Library | Purpose | Interface | Implementation | Used By |
|---------|---------|-----------|----------------|---------|
| **SDL2** | Audio/Video Playback | `IPlaybackEngine` | `SDL2PlaybackEngine` | `PlaybackController` |
| **TagLib** | Metadata Read/Write | `IMetadataReader` | `TagLibMetadataReader` | `LibraryController` |

### Lifecycle

| Phase | SDL2 | TagLib |
|-------|------|--------|
| **Initialization** | `SDL_Init()` in `SDL2PlaybackEngine::initialize()` | No init needed |
| **Runtime** | Playback thread active | Called on-demand per file |
| **Shutdown** | `SDL_Quit()` in destructor | Auto cleanup |

---

## ✅ Benefits của Architecture Này

1. ✅ **SDL2 isolated** - Chỉ trong `SDL2PlaybackEngine.cpp`
2. ✅ **TagLib isolated** - Chỉ trong `TagLibMetadataReader.cpp`
3. ✅ **Easy to test** - Mock interfaces cho unit tests
4. ✅ **Future-proof** - Swap libraries without touching controllers
5. ✅ **Clean separation** - Business logic không depend on external libs

**Đây là textbook Dependency Inversion Principle implementation!** 🎯
