#!/bin/bash

# Install qPCRtools to /Applications directory

set -e

DMG_FILE="build/qPCRtools-1.0.0-macOS.dmg"
APP_NAME="qPCRtools.app"
INSTALL_DIR="/Applications"

echo "=== Installing qPCRtools to /Applications ==="

# Check if DMG exists
if [ ! -f "${DMG_FILE}" ]; then
    echo "❌ DMG file not found: ${DMG_FILE}"
    echo "Please run deployment first: bash scripts/macos_deploy.sh"
    exit 1
fi

# Check if app is already running
if pgrep -x "qPCRtools" > /dev/null; then
    echo "⚠️  qPCRtools is currently running. Closing it..."
    killall qPCRtools 2>/dev/null || true
    sleep 2
fi

# Mount DMG
echo "Mounting DMG..."
hdiutil attach "${DMG_FILE}" -readonly -nobrowse -mountpoint /tmp/qpcrtools_dmg

# Copy app to Applications
echo "Installing to ${INSTALL_DIR}..."
# Remove old version if exists
if [ -d "${INSTALL_DIR}/${APP_NAME}" ]; then
    echo "Removing old version..."
    rm -rf "${INSTALL_DIR}/${APP_NAME}"
fi

# Copy new version
cp -R "/tmp/qpcrtools_dmg/${APP_NAME}" "${INSTALL_DIR}/"

# Unmount DMG
echo "Cleaning up..."
hdiutil detach /tmp/qpcrtools_dmg 2>/dev/null || true

echo ""
echo "✅ Installation complete!"
echo ""
echo "You can now launch qPCRtools from:"
echo "  1. Spotlight Search (Cmd+Space) → 'qPCRtools'"
echo "  2. Launchpad → qPCRtools"
echo "  3. /Applications folder"
echo ""
echo "Or run: open /Applications/qPCRtools.app"
