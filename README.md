# 🎵 Music Player Application

A modern, cross-platform music player built with C++17 featuring MVC architecture, thread-safe operations, and extensible design.

## ✨ Features

- 🎼 **Library Management** - Organize your music collection
- 📝 **Playlists** - Create and manage multiple playlists
- ⏯️ **Playback Control** - Play, pause, seek, volume control
- 📜 **History Tracking** - Keep track of played songs
- 🔍 **Search** - Quick search across your library
- 💾 **Persistence** - Save and load your library and playlists
- 🔌 **USB Support** - Scan and import from USB devices
- 🏷️ **Metadata** - Full metadata read/write with TagLib

## 🏗️ Architecture

- **MVC Pattern** - Clean separation of concerns
- **Observer Pattern** - Reactive UI updates
- **Dependency Injection** - Loose coupling via interfaces
- **Thread-Safe** - All models protected with mutexes
- **Modern C++17** - Using latest standard features

## 🚀 Quick Start

### One-Command Build (Ubuntu/Debian)

```bash
./build.sh
```

### Manual Build

```bash
# Install dependencies
sudo apt-get install build-essential cmake pkg-config libtag1-dev

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./music_player
```

## 📋 Requirements

### Essential
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2017+)
- CMake 3.15+
- TagLib

### Optional
- SDL2 + SDL2_mixer (for audio playback)
- ImGui (for graphical interface)
- OpenGL (for ImGui rendering)

See [`docs/BUILD.md`](docs/BUILD.md) for detailed installation instructions.

## 📦 Project Structure

```
music_player/
├── inc/                    # Header files
│   ├── interfaces/         # Abstract interfaces
│   ├── app/               # Application layer
│   │   ├── model/         # Data models
│   │   ├── view/          # UI views
│   │   └── controller/    # Business logic
│   ├── service/           # External services
│   ├── hal/               # Hardware abstraction
│   └── utils/             # Utilities
├── src/                   # Implementation files
│   ├── app/               # Application code
│   ├── service/           # Service implementations
│   └── hal/               # Hardware implementations
├── docs/                  # Documentation
├── CMakeLists.txt         # Build configuration
└── build.sh               # Quick build script
```

## 🔧 Build Options

```bash
# Full build with all features
cmake .. -DUSE_SDL2=ON -DUSE_IMGUI=ON

# Minimal build (TagLib only)
cmake .. -DUSE_SDL2=OFF -DUSE_IMGUI=OFF

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

## 📚 Documentation

- [`BUILD.md`](docs/BUILD.md) - Complete build instructions
- [`HLD_Architecture.md`](docs/02_Design/HLD_Architecture.md) - Architecture design
- [`architecture_diagrams.md`](docs/02_Design/architecture_diagrams.md) - System diagrams

## 🎨 Design Patterns

- **Factory Pattern** - MediaFileFactory, ViewFactory
- **Observer Pattern** - Model-View communication
- **Command Pattern** - Controller operations
- **Singleton Pattern** - Logger utility
- **Dependency Injection** - Throughout the application

## 🧵 Thread Safety

All model classes are thread-safe:
- `std::mutex` for data protection
- `std::lock_guard` for RAII locking
- Separate mutexes for observer operations

## 📝 Key Components

### Models (Data Layer)
- `Library` - Music library with search and indexing
- `Playlist` - Track lists with shuffle and loop
- `PlaybackState` - Current playback status
- `History` - Playback history with deduplication

### Controllers (Business Logic)
- `LibraryController` - Library operations
- `PlaybackController` - Playback control
- `PlaylistController` - Playlist management
- `HistoryController` - History operations
- `USBController` - USB device handling

### Views (UI Layer)
- `LibraryView` - Library browser
- `PlaylistView` - Playlist manager
- `NowPlayingView` - Playback controls
- `HistoryView` - History viewer
- `FileBrowserView` - File system browser

### Services (External Integration)
- `TagLibMetadataReader` - Audio metadata
- `LocalFileSystem` - File operations
- `JsonPersistence` - Data storage
- `SDL2PlaybackEngine` - Audio playback
- `S32K144Interface` - Hardware interface

## 🔍 Implementation Status

✅ **Complete** (26/26 .cpp files - 100%)
- Models: 7 files
- Services: 5 files
- Controllers: 5 files
- Views: 8 files
- Application: 1 file

## 🐛 Known Limitations

- SDL2PlaybackEngine is stub implementation (requires SDL2 integration)
- S32K144Interface is stub implementation (requires hardware SDK)
- ImGui UI requires manual integration
- USB detection is platform-dependent

## 🤝 Contributing

This is a student project demonstrating software architecture principles.

## 📄 License

Educational project - see documentation for details.

## 🎓 Educational Focus

This project demonstrates:
- Clean Architecture principles
- SOLID design principles
- Modern C++ best practices
- Design pattern implementation
- Thread-safe programming
- Dependency management
- Build system configuration

---

**Built with ❤️ using Modern C++**
