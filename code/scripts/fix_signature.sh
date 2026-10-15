#!/bin/bash

# Fix code signing for qPCRtools.app

set -e

# 2026-10-13 目录整理：脚本随 CMake 工程移入 code/，构建产物仍在仓库根。
# 切回仓库根，保证从任意目录调用都能找到 build/
cd "$(dirname "$0")/../.."

APP_BUNDLE="build/qPCRtools.app"

echo "=== Fixing qPCRtools.app Code Signature ==="

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "❌ App bundle not found: ${APP_BUNDLE}"
    exit 1
fi

echo "Removing existing signatures..."
codesign --remove-signature "${APP_BUNDLE}" 2>/dev/null || true

echo "Signing frameworks and libraries..."
find "${APP_BUNDLE}/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*Qt*" \) -exec codesign --force --deep --sign - {} \; 2>/dev/null || true
find "${APP_BUNDLE}/Contents/Frameworks" -type d -name "*.framework" -exec codesign --force --deep --sign - {} \; 2>/dev/null || true
find "${APP_BUNDLE}/Contents/Helpers" -type d -name "*.app" -exec codesign --force --deep --sign - {} \; 2>/dev/null || true
find "${APP_BUNDLE}/Contents/PlugIns" -type d -name "*.appex" -exec codesign --force --deep --sign - {} \; 2>/dev/null || true

echo "Signing main app bundle..."
codesign --force --deep --sign - "${APP_BUNDLE}"

echo "Verifying signature..."
if codesign -v "${APP_BUNDLE}" 2>&1; then
    echo "✅ Code signature verified successfully"
else
    echo "⚠️  Code signature verification had warnings"
    echo "App may still work, but consider getting an Apple Developer certificate"
fi

echo ""
echo "=== Testing App ==="
if [ -x "${APP_BUNDLE}/Contents/MacOS/qPCRtools" ]; then
    echo "✅ App is executable"
    echo ""
    echo "To test the app:"
    echo "  open ${APP_BUNDLE}"
    echo ""
    echo "If the app still crashes, try:"
    echo "  1. Right-click the app and select 'Open'"
    echo "  2. Or run: xattr -cr ${APP_BUNDLE}"
else
    echo "❌ App is not executable"
    exit 1
fi

echo ""
echo "=== Done! ==="
