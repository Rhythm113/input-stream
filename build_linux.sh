#!/bin/bash
# =============================================
#  Input Stream - Linux Build Script
#  By Rhythm113
# =============================================

set -e

echo ""
echo "  Building Input Stream for Linux..."
echo "  ====================================="
echo ""

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo "  [!] CMake not found. Install with:"
    echo "      sudo apt install cmake build-essential   (Debian/Ubuntu)"
    echo "      sudo dnf install cmake gcc-c++           (Fedora)"
    echo "      sudo pacman -S cmake base-devel          (Arch)"
    exit 1
fi

# Configure
echo "  [1/2] Configuring..."
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
echo "  [2/2] Building..."
cmake --build build --config Release -j$(nproc)

echo ""
echo "  ============================================="
echo "  [+] Build successful!"
echo "  [+] Binary: build/input_stream"
echo ""
echo "  To run (requires uinput access):"
echo "      sudo ./build/input_stream"
echo "  Or add yourself to the input group:"
echo "      sudo usermod -aG input \$USER"
echo "      (log out and back in, then run without sudo)"
echo "  ============================================="
echo ""
