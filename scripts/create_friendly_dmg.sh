#!/bin/bash

# Create user-friendly DMG with drag-and-drop installation

set -e

VERSION="1.0.0"
PROJECT_NAME="qPCRtools"
BUILD_DIR="build"
APP_BUNDLE="${PROJECT_NAME}.app"
DMG_NAME="${PROJECT_NAME}-${VERSION}-macOS.dmg"
SOURCE_APP="${BUILD_DIR}/${APP_BUNDLE}"

# Check if app exists
if [ ! -d "${SOURCE_APP}" ]; then
    echo "❌ App bundle not found: ${SOURCE_APP}"
    echo "Please run deployment first: bash scripts/macos_deploy.sh"
    exit 1
fi

echo "=== Creating User-Friendly DMG ==="

# Create temporary directory for DMG contents
DMG_TEMP_DIR="dmg_temp"
rm -rf ${DMG_TEMP_DIR}
mkdir -p ${DMG_TEMP_DIR}

# Copy app to temp directory
echo "Copying app bundle..."
cp -R "${SOURCE_APP}" "${DMG_TEMP_DIR}/"

# Create symbolic link to Applications
echo "Creating Applications shortcut..."
ln -s /Applications "${DMG_TEMP_DIR}/Applications"

# Create DMG
echo "Creating DMG image..."
hdiutil create -volname "${PROJECT_NAME}" \
    -srcfolder ${DMG_TEMP_DIR} \
    -ov -format UDZO \
    -imagekey zlib-level=9 \
    "${BUILD_DIR}/${DMG_NAME}"

# Clean up
rm -rf ${DMG_TEMP_DIR}

echo ""
echo "✅ DMG created successfully!"
echo "File: ${BUILD_DIR}/${DMG_NAME}"
echo ""
echo "Installation instructions for users:"
echo "1. Double-click the DMG file to open it"
echo "2. A window will appear with qPCRtools app and Applications folder"
echo "3. Drag qPCRtools to the Applications folder"
echo "4. Eject the DMG when done"
echo ""
echo "To test the DMG:"
echo "  open ${BUILD_DIR}/${DMG_NAME}"
