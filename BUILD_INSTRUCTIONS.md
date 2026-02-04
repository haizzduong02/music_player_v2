# Music Player - Build Instructions

## Quick Start (Production Build)

```bash
./build_production.sh
```

This script will:
1. Check system dependencies
2. Download ImGui library automatically
3. Configure CMake
4. Compile the project
5. Create production executable

## Manual Build

If the automatic script doesn't work:

### 1. Install Dependencies

```bash
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config \
    libtag1-dev libsdl2-dev libsdl2-mixer-dev
```

### 2. Download ImGui Manually

```bash
cd external
wget https://github.com/ocornut/imgui/archive/refs/heads/master.zip
unzip master.zip
mv imgui-master imgui
cd ..
```

### 3. Build

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 4. Run

```bash
./music_player
```

## Build Outputs

- **Executable**: `build/music_player`
- **Size**: ~500KB - 2MB (depending on features)
- **Dependencies**: TagLib, SDL2 (runtime)

## Troubleshooting

### ImGui not found
```bash
# Re-run the production build script
./build_production.sh
```

### SDL2 not found
```bash
sudo apt-get install libsdl2-dev libsdl2-mixer-dev
```

### TagLib not found
```bash
sudo apt-get install libtag1-dev
```

## Features

The production build includes:
- ✅ Full MVC architecture
- ✅ Thread-safe models
- ✅ Observer pattern
- ✅ ImGui-based UI
- ✅ TagLib metadata reading
- ✅ SDL2 audio playback (if available)
- ✅ Library management
- ✅ Playlist support
- ✅ Playback history

## Next Steps

After building successfully:
1. Run the executable: `cd build && ./music_player`
2. Test with actual media files
3. Deploy to target system
