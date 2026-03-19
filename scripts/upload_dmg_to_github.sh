#!/bin/bash

# Upload DMG to GitHub Release
# Usage: ./upload_dmg_to_github.sh <github_token>

set -e

VERSION="1.0.0"
DMG_FILE="build/qPCRtools-${VERSION}-macOS.dmg"
REPO="lixiang117423/qPCRtools_cpp"

if [ -z "$1" ]; then
    echo "Usage: $0 <github_token>"
    echo ""
    echo "Get a token from: https://github.com/settings/tokens"
    echo "Required scopes: repo (full control of private repositories)"
    exit 1
fi

GITHUB_TOKEN="$1"

if [ ! -f "${DMG_FILE}" ]; then
    echo "❌ DMG file not found: ${DMG_FILE}"
    exit 1
fi

echo "=== Uploading qPCRtools DMG to GitHub ==="
echo "Repository: ${REPO}"
echo "Version: ${VERSION}"
echo "File: ${DMG_FILE}"
echo ""

# Check if release exists
echo "Checking if release exists..."
RELEASE_RESPONSE=$(curl -s \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    "https://api.github.com/repos/${REPO}/releases/tags/v${VERSION}")

RELEASE_ID=$(echo "${RELEASE_RESPONSE}" | grep -m 1 '"id"' | head -1 | sed 's/.*: \([0-9]*\).*/\1/')

if [ -z "${RELEASE_ID}" ] || [ "${RELEASE_ID}" = "null" ]; then
    echo "Release v${VERSION} does not exist. Creating it..."

    # Create release
    CREATE_RESPONSE=$(curl -s \
        -X POST \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        -H "Content-Type: application/json" \
        "https://api.github.com/repos/${REPO}/releases" \
        -d "{
            \"tag_name\": \"v${VERSION}\",
            \"target_commitish\": \"main\",
            \"name\": \"qPCRtools ${VERSION}\",
            \"body\": \"qPCRtools ${VERSION} for macOS\\n\\n## Changes\\n\\n- Fixed web interface loading issues\\n- Fixed file permissions on web resources\\n- Fixed QtWebEngineProcess helper app\\n- App now launches successfully from DMG\\n\\n## Installation\\n\\n1. Download the DMG file\\n2. Open the DMG\\n3. Drag qPCRtools to your Applications folder\\n4. Right-click and open if you see a security warning\\n\\n## System Requirements\\n\\n- macOS 10.15 or later\\n- Apple Silicon (M1/M2/M3/M4) or Intel Mac\",
            \"draft\": false,
            \"prerelease\": false
        }")

    RELEASE_ID=$(echo "${CREATE_RESPONSE}" | grep -m 1 '"id"' | head -1 | sed 's/.*: \([0-9]*\).*/\1/')
    echo "✅ Release created with ID: ${RELEASE_ID}"
else
    echo "✅ Release exists with ID: ${RELEASE_ID}"
fi

echo ""
echo "Uploading DMG file..."

# Upload asset
UPLOAD_RESPONSE=$(curl -s \
    -X POST \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    -H "Content-Type: application/octet-stream" \
    "https://uploads.github.com/repos/${REPO}/releases/${RELEASE_ID}/assets?name=$(basename ${DMG_FILE})" \
    --data-binary "@${DMG_FILE}")

echo ""
if echo "${UPLOAD_RESPONSE}" | grep -q '"state": "uploaded"'; then
    echo "✅ DMG uploaded successfully!"
    echo ""
    echo "Download URL:"
    echo "https://github.com/${REPO}/releases/download/v${VERSION}/$(basename ${DMG_FILE})"
else
    echo "❌ Upload failed!"
    echo "${UPLOAD_RESPONSE}"
    exit 1
fi

echo ""
echo "=== Done! ==="
