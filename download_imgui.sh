#!/bin/bash
##############################################################################
# ImGui Manual Download Helper
# Use this if automatic download fails
##############################################################################

echo "=== ImGui Manual Download Instructions ==="
echo ""
echo "Your network has SSL issues. Here are 3 options:"
echo ""
echo "OPTION 1: Download from a working computer"
echo "  1. On a computer with working internet:"
echo "     wget https://github.com/ocornut/imgui/archive/refs/heads/master.zip"
echo "  2. Transfer master.zip to this computer"
echo "  3. Run: unzip master.zip && mv imgui-master external/imgui"
echo ""
echo "OPTION 2: Use git with SSL disable (NOT RECOMMENDED for production)"
echo "  git -c http.sslVerify=false clone https://github.com/ocornut/imgui.git external/imgui"
echo ""
echo "OPTION 3: Compile WITHOUT ImGui (headless mode)"
echo "  The program will compile but UI will not render."
echo "  Useful for testing core functionality."
echo ""
read -p "Do you want to try OPTION 2 (disable SSL)? [y/N]: " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Attempting download with SSL verification disabled..."
    git -c http.sslVerify=false clone --depth 1 https://github.com/ocornut/imgui.git external/imgui
    
    if [ -f "external/imgui/imgui.h" ]; then
        echo "✓ ImGui downloaded successfully!"
        echo "Now run: ./build_production.sh"
    else
        echo "❌ Download failed. Please use OPTION 1 or 3."
    fi
else
    echo "Please use OPTION 1 or OPTION 3"
fi
