#!/bin/bash

# Fix QtWebEngineProcess helper app after build
# 2026-10-15 重写修法：用「相对软链 + 改写 helper 自身依赖」代替「把全部 Qt 框架
# 复制进 helper」——后者会让 DMG 体积几乎翻倍。
#
# 背景（实测）：用 **Homebrew 的 Qt** 构建时，macdeployqt 只把主程序与各框架的依赖
# 改写成 @executable_path/../Frameworks/...，却**没有处理 QtWebEngineProcess helper**：
#   - 现象：主窗口弹出 "Failed to load the web interface"，控制台里是
#     `dyld: Library not loaded: @executable_path/../Frameworks/QtWebChannel.framework/...`
#     （helper 的 @executable_path 指向它自己的 Contents/MacOS，于是 ../Frameworks
#      落在 helper 自己的 bundle 里，而那里没有框架）；
#   - helper 二进制里还留着 /opt/homebrew/... 的绝对路径，换台机器同样会失败。
# 修法（本脚本）：
#   ① 在 helper 的 Contents 下建一个**指向主包 Frameworks 的相对软链**，
#      这样 helper 及其加载的框架里所有 @executable_path/../Frameworks/... 都能解析；
#   ② 用 install_name_tool 把 helper 自身的 /opt/homebrew/... 绝对依赖改写成
#      @executable_path/../Frameworks/<framework 名>...；
#   ③ 重新 ad-hoc 签名（install_name_tool 会让原签名失效）。
# 用官方 Qt（aqt / Qt 官方安装器）构建时，helper 的依赖已经是 @rpath，本脚本可跳过。

# 2026-10-13 目录整理：脚本随 CMake 工程移入 code/，构建产物仍在仓库根。
# 切回仓库根，保证从任意目录调用都能找到 build/qPCRtools.app
cd "$(dirname "$0")/../.."

APP_BUNDLE="${1:-build/qPCRtools.app}"
HELPER_APP="$APP_BUNDLE/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
HELPER_BIN="$HELPER_APP/Contents/MacOS/QtWebEngineProcess"

if [ ! -d "$HELPER_APP" ]; then
    echo "Warning: Helper app not found at $HELPER_APP"
    echo "Skipping helper app fix..."
    exit 0
fi

echo "Fixing QtWebEngineProcess helper app..."

# ① helper 的 Contents/Frameworks → 主包 Frameworks 的相对软链
#    HELPERS/QtWebEngineProcess.app/Contents/Frameworks
#      → ../../../../../../Frameworks（6 级：Contents→app→Helpers→A→Versions→QtWebEngineCore.framework）
ln -sfn ../../../../../../ "$HELPER_APP/Contents/Frameworks"
echo "  [1/3] Frameworks 软链 -> $(cd "$HELPER_APP/Contents" && python3 -c 'import os;print(os.path.realpath("Frameworks"))')"

# ② 把 helper 自身的绝对依赖（Homebrew Qt 会留 /opt/homebrew/... ）改写成
#    @executable_path/../Frameworks/<framework>/...（经上面的软链解析到主包）
changed=0
while IFS= read -r dep; do
    [ -n "$dep" ] || continue
    rest=$(printf '%s' "$dep" | sed -E 's#^.*/([^/]+\.framework/.*)$#\1#')
    [ "$rest" = "$dep" ] && continue
    install_name_tool -change "$dep" "@executable_path/../Frameworks/$rest" "$HELPER_BIN"
    changed=$((changed + 1))
done < <(otool -L "$HELPER_BIN" | awk '/\/opt\/homebrew|\/usr\/local/{print $1}')
echo "  [2/3] 改写 helper 自身绝对依赖：$changed 条"

# ③ 重新 ad-hoc 签名（install_name_tool 会作废原签名）
xattr -cr "$APP_BUNDLE"
codesign --force --deep --sign - "$APP_BUNDLE" >/dev/null 2>&1 && echo "  [3/3] 重新 ad-hoc 签名完成" || {
    echo "  [3/3] 签名失败，请手动执行：xattr -cr \"$APP_BUNDLE\" && codesign --force --deep --sign - \"$APP_BUNDLE\"" >&2
    exit 1
}

echo "Helper app fixed!"
