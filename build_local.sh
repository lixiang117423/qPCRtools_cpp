#!/bin/bash

# Complete build script for macOS local testing
# Author: Claude Code
# Date: 2026-03-19

set -e  # Exit on error

PROJECT_DIR="/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp"
cd "$PROJECT_DIR"

echo "========================================="
echo "Building qPCRtools for macOS (local)"
echo "========================================="

# Step 1: Clean and create build directory
echo ""
echo "[1/7] Cleaning build directory..."
rm -rf build
mkdir build

# Step 2: Configure CMake
echo ""
echo "[2/7] Configuring CMake..."
cd build
cmake -DCMAKE_PREFIX_PATH="/opt/homebrew" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_STANDARD=17 ..

# Step 3: Build
echo ""
echo "[3/7] Building..."
cmake --build . -j8

# Step 4: Copy web resources
echo ""
echo "[4/7] Copying web resources..."
mkdir -p qPCRtools.app/Contents/Resources/web
cp -R ../web/* qPCRtools.app/Contents/Resources/web/

# Step 5: Copy example data
echo ""
echo "[5/7] Copying example data..."
mkdir -p qPCRtools.app/Contents/Resources/examples
cp ../examples/*.csv qPCRtools.app/Contents/Resources/examples/ 2>/dev/null || echo "  (No example files found)"

# Step 6: Fix file permissions
echo ""
echo "[6/7] Fixing file permissions..."
chmod -R u+rwX,go+rX qPCRtools.app/Contents/Resources/
xattr -rc qPCRtools.app/Contents/Resources/ 2>/dev/null || true

# Step 7: Fix helper app
echo ""
echo "[7/7] Fixing QtWebEngineProcess helper app..."
HELPER_APP="qPCRtools.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
if [ -d "$HELPER_APP" ]; then
    HELPER_FRAMEWORKS="$HELPER_APP/Contents/Frameworks"
    mkdir -p "$HELPER_FRAMEWORKS"
    
    for framework in qPCRtools.app/Contents/Frameworks/Qt*.framework; do
        if [ -d "$framework" ]; then
            cp -R "$framework" "$HELPER_FRAMEWORKS/"
        fi
    done
    echo "  Copied $(ls "$HELPER_FRAMEWORKS" | wc -l | tr -d ' ') Qt frameworks to helper app"
else
    echo "  Warning: Helper app not found, skipping..."
fi

# Code signing
echo ""
echo "Signing app..."
codesign --force --deep --sign - qPCRtools.app

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo ""
echo "To run the app:"
echo "  open build/qPCRtools.app"
echo ""
echo "Or from terminal to see debug output:"
echo "  ./build/qPCRtools.app/Contents/MacOS/qPCRtools"
echo ""
