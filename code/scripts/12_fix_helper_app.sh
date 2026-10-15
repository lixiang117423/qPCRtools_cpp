#!/bin/bash

# Fix QtWebEngineProcess helper app
# Copy all Qt frameworks to the helper app

# 2026-10-13 目录整理：脚本随 CMake 工程移入 code/，构建产物仍在仓库根。
# 切回仓库根，保证从任意目录调用都能找到 build/qPCRtools.app
cd "$(dirname "$0")/../.."

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
