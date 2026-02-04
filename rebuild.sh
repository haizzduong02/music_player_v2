#!/bin/bash
# Fast rebuild script - skips dependency checks
# Usage: ./rebuild.sh [debug|release]

set -e

BUILD_TYPE="${1:-Debug}"
BUILD_DIR="build"

echo "=== Fast Rebuild ($BUILD_TYPE) ==="

# Create build dir if needed (don't delete!)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Only run cmake if CMakeCache doesn't exist
if [ ! -f "CMakeCache.txt" ]; then
    echo "Running CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
fi

# Build with all cores
echo "Building..."
make -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo "Run: ./build/music_player"
