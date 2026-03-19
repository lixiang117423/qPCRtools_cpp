#!/bin/bash

# Fix QtWebEngineProcess helper app
# Copy all Qt frameworks to the helper app

APP_BUNDLE="build/qPCRtools.app"
HELPER_APP="$APP_BUNDLE/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
HELPER_FRAMEWORKS="$HELPER_APP/Contents/Frameworks"

echo "Fixing QtWebEngineProcess helper app..."

# Create Frameworks directory in helper app
mkdir -p "$HELPER_FRAMEWORKS"

# Copy all Qt frameworks from main app to helper app
for framework in "$APP_BUNDLE/Contents/Frameworks"/Qt*.framework; do
    framework_name=$(basename "$framework")
    echo "Copying $framework_name to helper app..."
    cp -R "$framework" "$HELPER_FRAMEWORKS/"
done

echo "Helper app fixed!"
echo "Copied frameworks:"
ls "$HELPER_FRAMEWORKS"
