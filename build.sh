#!/bin/bash

# Music Player Quick Build Script
# This script automates the build process for the music player application

set -e  # Exit on error

echo "========================================="
echo "Music Player - Quick Build Script"
echo "========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on Ubuntu/Debian
if command -v apt-get &> /dev/null; then
    print_info "Detected Debian/Ubuntu system"
    
    # Ask to install dependencies
    read -p "Install build dependencies? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "Installing dependencies..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libtag1-dev \
            libsdl2-dev \
            libsdl2-mixer-dev \
            || print_warning "Some optional dependencies may not have installed"
    fi
else
    print_warning "Not a Debian/Ubuntu system - please install dependencies manually"
    print_info "See docs/BUILD.md for instructions"
fi

# Check for required tools
print_info "Checking build tools..."

if ! command -v cmake &> /dev/null; then
    print_error "CMake not found. Please install CMake 3.15 or higher"
    exit 1
fi

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    print_error "No C++ compiler found. Please install g++ or clang++"
    exit 1
fi

print_info "✓ Build tools found"

# Check for TagLib
if pkg-config --exists taglib; then
    print_info "✓ TagLib found"
else
    print_error "TagLib not found. Please install libtag1-dev"
    exit 1
fi

# Create build directory
print_info "Creating build directory..."
rm -rf build
mkdir build
cd build

# Configure CMake
print_info "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_SDL2=ON \
    -DUSE_IMGUI=OFF \
    || { print_error "CMake configuration failed"; exit 1; }

# Build
print_info "Building project..."
CORES=$(nproc)
print_info "Using $CORES CPU cores"

make -j$CORES || { print_error "Build failed"; exit 1; }

echo ""
echo "========================================="
print_info "Build successful! ✓"
echo "========================================="
echo ""
print_info "Executable location: $(pwd)/music_player"
print_info "Run with: ./build/music_player"
echo ""
