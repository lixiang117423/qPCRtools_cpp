#!/bin/bash

# Fix QtWebEngineProcess helper app after build
# This script copies all Qt frameworks to the helper app

APP_BUNDLE="${1:-build/qPCRtools.app}"
HELPER_APP="$APP_BUNDLE/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
HELPER_FRAMEWORKS="$HELPER_APP/Contents/Frameworks"

if [ ! -d "$HELPER_APP" ]; then
    echo "Warning: Helper app not found at $HELPER_APP"
    echo "Skipping helper app fix..."
    exit 0
fi

echo "Fixing QtWebEngineProcess helper app..."

# Create Frameworks directory in helper app
mkdir -p "$HELPER_FRAMEWORKS"

# Copy all Qt frameworks from main app to helper app
for framework in "$APP_BUNDLE/Contents/Frameworks"/Qt*.framework; do
    if [ -d "$framework" ]; then
        framework_name=$(basename "$framework")
        echo "Copying $framework_name to helper app..."
        cp -R "$framework" "$HELPER_FRAMEWORKS/"
    fi
done

echo "Helper app fixed! Copied $(ls "$HELPER_FRAMEWORKS" | wc -l) frameworks."
