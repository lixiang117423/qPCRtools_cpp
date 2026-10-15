#!/bin/bash

# Run qPCRtools and capture debug output

# 2026-10-13 目录整理：脚本随 CMake 工程移入 code/，构建产物仍在仓库根。
# 切回仓库根，保证从任意目录调用都能找到 build/
cd "$(dirname "$0")/../.."

APP_BUNDLE="build/qPCRtools.app"

echo "=== Launching qPCRtools with Debug Output ==="
echo ""
echo "To stop logging: Press Ctrl+C"
echo ""

# Check if app exists
if [ ! -d "${APP_BUNDLE}" ]; then
    echo "❌ App bundle not found: ${APP_BUNDLE}"
    exit 1
fi

# Run app and capture logs
${APP_BUNDLE}/Contents/MacOS/qPCRtools 2>&1 | tee /tmp/qpcrtools_debug.log &
APP_PID=$!

echo "App launched (PID: ${APP_PID})"
echo "Debug log: /tmp/qpcrtools_debug.log"
echo ""
echo "View logs in real-time:"
echo "  tail -f /tmp/qpcrtools_debug.log"
echo ""
echo "Filter for errors:"
echo "  grep -i error /tmp/qpcrtools_debug.log"
echo "  grep -i web /tmp/qpcrtools_debug.log"
echo ""
