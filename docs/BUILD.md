# Building the Music Player Application

This guide walks you through building and running the music player from source.

---

## 📋 Prerequisites

### Required Dependencies
- **C++17 compatible compiler** (GCC 8+, Clang 7+, MSVC 2017+)
- **CMake 3.15+**
- **TagLib** - Audio metadata library
- **pkg-config** - For finding libraries

### Optional Dependencies
- **SDL2** - For audio playback (stub implementation if not available)
- **SDL2_mixer** - For enhanced audio format support
- **ImGui** - For GUI (stub implementation if not available)
- **OpenGL** - For ImGui rendering

---

## 🔧 Installing Dependencies

### Ubuntu/Debian
```bash
# Essential dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libtag1-dev

# Optional: SDL2 for audio playback
sudo apt-get install -y \
    libsdl2-dev \
    libsdl2-mixer-dev

# Optional: OpenGL for ImGui
sudo apt-get install -y \
    libgl1-mesa-dev \
    libglu1-mesa-dev
```

### Fedora/RHEL
```bash
# Essential dependencies
sudo dnf install -y \
    gcc-c++ \
    cmake \
    pkgconfig \
    taglib-devel

# Optional: SDL2
sudo dnf install -y \
    SDL2-devel \
    SDL2_mixer-devel
```

### macOS (using Homebrew)
```bash
# Essential dependencies
brew install cmake taglib

# Optional: SDL2
brew install sdl2 sdl2_mixer
```

### Windows (using vcpkg)
```powershell
# Install vcpkg first, then:
vcpkg install taglib:x64-windows
vcpkg install sdl2:x64-windows
vcpkg install sdl2-mixer:x64-windows
```

---

## 🎨 Installing ImGui (Optional)

ImGui is typically vendored into the project:

```bash
# Clone ImGui into third_party directory
mkdir -p third_party
cd third_party
git clone https://github.com/ocornut/imgui.git
cd ..
```

Alternatively, you can build without ImGui - the views will be stub implementations.

---

## 🏗️ Building the Project

### Step 1: Create Build Directory
```bash
cd /path/to/music_player
mkdir build
cd build
```

### Step 2: Configure with CMake
```bash
# Basic build (with TagLib only)
cmake ..

# Build with all features
cmake .. -DUSE_SDL2=ON -DUSE_IMGUI=ON

# Build with specific build type
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with tests
cmake .. -DBUILD_TESTS=ON
```

### Step 3: Compile
```bash
# Use all CPU cores
make -j$(nproc)

# Or specify number of jobs
make -j4
```

### Step 4: (Optional) Install
```bash
sudo make install
```

---

## ▶️ Running the Application

### From Build Directory
```bash
cd build
./music_player
```

### After Installation
```bash
music_player
```

---

## 🔍 Troubleshooting

### TagLib Not Found
```bash
# Check if pkg-config can find it
pkg-config --cflags --libs taglib

# If not, install taglib-dev package
sudo apt-get install libtag1-dev
```

### SDL2 Not Found
```bash
# The app will build without SDL2 but playback will be stub
# To enable, install SDL2:
sudo apt-get install libsdl2-dev libsdl2-mixer-dev
```

### Filesystem Library Errors (GCC < 9)
If you get linking errors with `std::filesystem`:
```bash
# Edit CMakeLists.txt to ensure stdc++fs is linked
# This is already handled in the provided CMakeLists.txt
```

### Missing ImGui
```bash
# Either install ImGui in third_party/imgui
# Or build without it:
cmake .. -DUSE_IMGUI=OFF
```

---

## 🧪 Build Configurations

### Debug Build (with symbols)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Release Build (optimized)
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Minimal Build (TagLib only)
```bash
cmake .. -DUSE_SDL2=OFF -DUSE_IMGUI=OFF
make -j$(nproc)
```

---

## 📁 Expected Build Output

After successful build, you should see:
```
build/
├── music_player          # Main executable
├── compile_commands.json # For IDE integration
└── CMakeFiles/           # Build artifacts
```

---

## 🚀 Quick Start Script

Create a `build.sh` script:
```bash
#!/bin/bash
set -e

echo "Building Music Player..."

# Install dependencies (Ubuntu/Debian)
if command -v apt-get &> /dev/null; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake pkg-config libtag1-dev
fi

# Create build directory
rm -rf build
mkdir build
cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo "Build complete! Run with: ./build/music_player"
```

Make it executable and run:
```bash
chmod +x build.sh
./build.sh
```

---

## 📝 Notes

- **First build** may take a few minutes
- **TagLib is required** - application won't build without it
- **SDL2 and ImGui are optional** - stubs are provided
- Check `logs/` directory for application logs after running
- For development, use Debug build for better error messages
- For production, use Release build for better performance

---

## 🐛 Reporting Build Issues

If you encounter build errors:
1. Check CMake version: `cmake --version` (need 3.15+)
2. Check compiler version: `g++ --version` (need C++17 support)
3. Verify all dependencies are installed
4. Check CMake output for missing libraries
5. Review build logs in `build/CMakeFiles/`
