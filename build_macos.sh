#!/bin/bash
# =============================================
#  Input Stream - macOS Build Script
#  By Rhythm113
# =============================================

set -e

echo ""
echo "  Building Input Stream for macOS..."
echo "  ====================================="
echo ""

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo "  [!] CMake not found. Install with:"
    echo "      brew install cmake"
    echo "  (Requires Homebrew: https://brew.sh)"
    exit 1
fi

# Check for Xcode command line tools
if ! xcode-select -p &> /dev/null; then
    echo "  [!] Xcode Command Line Tools not found. Install with:"
    echo "      xcode-select --install"
    exit 1
fi

# Configure
echo "  [1/2] Configuring..."
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
echo "  [2/2] Building..."
cmake --build build --config Release -j$(sysctl -n hw.ncpu)

echo ""
echo "  ============================================="
echo "  [+] Build successful!"
echo "  [+] Binary: build/input_stream"
echo ""
echo "  IMPORTANT: On first run, macOS will ask for"
echo "  Accessibility permissions. Grant them in:"
echo "      System Settings > Privacy & Security > Accessibility"
echo "  ============================================="
echo ""
