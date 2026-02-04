#!/bin/bash
##############################################################################
# Music Player - Complete Build Script
# This script downloads all dependencies and builds the production executable
##############################################################################

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTERNAL_DIR="${SCRIPT_DIR}/external"
IMGUI_DIR="${EXTERNAL_DIR}/imgui"
BUILD_DIR="${SCRIPT_DIR}/build"

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   Music Player - Production Build     ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

##############################################################################
# Step 1: Check Dependencies
##############################################################################
echo -e "${YELLOW}[1/5] Checking system dependencies...${NC}"

MISSING_DEPS=()

if ! command -v g++ &> /dev/null; then
    MISSING_DEPS+=("g++")
fi

if ! command -v cmake &> /dev/null; then
    MISSING_DEPS+=("cmake")
fi

if ! pkg-config --exists taglib; then
    MISSING_DEPS+=("libtag1-dev")
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo -e "${RED}❌ Missing dependencies: ${MISSING_DEPS[*]}${NC}"
    echo -e "${YELLOW}Install with: sudo apt-get install ${MISSING_DEPS[*]}${NC}"
    exit 1
fi

echo -e "${GREEN}✓ All system dependencies found${NC}"

##############################################################################
# Step 2: Download ImGui
##############################################################################
echo -e "\n${YELLOW}[2/5] Setting up ImGui library...${NC}"

mkdir -p "${EXTERNAL_DIR}"

if [ -d "${IMGUI_DIR}" ] && [ -f "${IMGUI_DIR}/imgui.h" ]; then
    echo -e "${GREEN}✓ ImGui already exists${NC}"
else
    echo "Downloading ImGui..."
    
    # Try multiple download methods
    DOWNLOAD_SUCCESS=false
    
    # Method 1: Git clone (fastest if it works)
    if ! $DOWNLOAD_SUCCESS && command -v git &> /dev/null; then
        echo "  Trying git clone..."
        if git clone --depth 1 https://github.com/ocornut/imgui.git "${IMGUI_DIR}" 2>/dev/null; then
            DOWNLOAD_SUCCESS=true
            echo -e "${GREEN}  ✓ Downloaded via git${NC}"
        fi
    fi
    
    # Method 2: Wget with specific version
    if ! $DOWNLOAD_SUCCESS && command -v wget &> /dev/null; then
        echo "  Trying wget..."
        IMGUI_VERSION="1.90.1"  
        TEMP_FILE="/tmp/imgui_${IMGUI_VERSION}.tar.gz"
        
        if wget --no-check-certificate -q -O "${TEMP_FILE}" \
            "https://github.com/ocornut/imgui/archive/refs/tags/v${IMGUI_VERSION}.tar.gz" 2>/dev/null; then
            mkdir -p "${EXTERNAL_DIR}"
            tar -xzf "${TEMP_FILE}" -C "${EXTERNAL_DIR}" 2>/dev/null
            mv "${EXTERNAL_DIR}/imgui-${IMGUI_VERSION}" "${IMGUI_DIR}" 2>/dev/null
            rm -f "${TEMP_FILE}"
            DOWNLOAD_SUCCESS=true
            echo -e "${GREEN}  ✓ Downloaded via wget${NC}"
        fi
    fi
    
    # Method 3: Curl
    if ! $DOWNLOAD_SUCCESS && command -v curl &> /dev/null; then
        echo "  Trying curl..."
        IMGUI_VERSION="1.90.1"
        TEMP_FILE="/tmp/imgui_${IMGUI_VERSION}.tar.gz"
        
        if curl -k -L -s -o "${TEMP_FILE}" \
            "https://github.com/ocornut/imgui/archive/refs/tags/v${IMGUI_VERSION}.tar.gz" 2>/dev/null; then
            mkdir -p "${EXTERNAL_DIR}"
            tar -xzf "${TEMP_FILE}" -C "${EXTERNAL_DIR}" 2>/dev/null
            mv "${EXTERNAL_DIR}/imgui-${IMGUI_VERSION}" "${IMGUI_DIR}" 2>/dev/null
            rm -f "${TEMP_FILE}"
            DOWNLOAD_SUCCESS=true
            echo -e "${GREEN}  ✓ Downloaded via curl${NC}"
        fi
    fi
    
    if ! $DOWNLOAD_SUCCESS; then
        echo -e "${RED}❌ Failed to download ImGui automatically${NC}"
        echo -e "${YELLOW}Manual download required:${NC}"
        echo "  1. Download: https://github.com/ocornut/imgui/archive/refs/heads/master.zip"
        echo "  2. Extract to: ${IMGUI_DIR}"
        echo "  3. Run this script again"
        exit 1
    fi
fi

# Verify ImGui files
REQUIRED_FILES=("imgui.h" "imgui.cpp" "imgui_draw.cpp" "imgui_tables.cpp" "imgui_widgets.cpp")
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "${IMGUI_DIR}/${file}" ]; then
        echo -e "${RED}❌ Missing ImGui file: ${file}${NC}"
        exit 1
    fi
done

echo -e "${GREEN}✓ ImGui library ready${NC}"

##############################################################################
# Step 3: Configure CMake
##############################################################################
echo -e "\n${YELLOW}[3/5] Configuring build with CMake...${NC}"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if cmake -DCMAKE_BUILD_TYPE=Release .. ; then
    echo -e "${GREEN}✓ CMake configuration successful${NC}"
else
    echo -e "${RED}❌ CMake configuration failed${NC}"
    exit 1
fi

##############################################################################
# Step 4: Compile
##############################################################################
echo -e "\n${YELLOW}[4/5] Compiling project...${NC}"

# Get number of CPU cores for parallel build
CORES=$(nproc 2>/dev/null || echo 4)

if make -j${CORES}; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
else
    echo -e "${RED}❌ Compilation failed${NC}"
    echo -e "${YELLOW}Tip: Check error messages above${NC}"
    exit 1
fi

##############################################################################
# Step 5: Verify Executable
##############################################################################
echo -e "\n${YELLOW}[5/5] Verifying executable...${NC}"

EXECUTABLE="${BUILD_DIR}/music_player"

if [ -f "${EXECUTABLE}" ]; then
    SIZE=$(ls -lh "${EXECUTABLE}" | awk '{print $5}')
    echo -e "${GREEN}✓ Executable created: ${SIZE}${NC}"
    echo -e "${GREEN}  Location: ${EXECUTABLE}${NC}"
else
    echo -e "${RED}❌ Executable not found${NC}"
    exit 1
fi

##############################################################################
# Success Summary
##############################################################################
echo ""
echo -e "${GREEN}╔════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     BUILD SUCCESSFUL! 🎉              ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Executable:${NC} ${EXECUTABLE}"
echo -e "${BLUE}Run with:${NC}   cd build && ./music_player"
echo ""
echo -e "${YELLOW}Note: This is a production build. UI requires SDL2 + OpenGL runtime.${NC}"
echo ""
