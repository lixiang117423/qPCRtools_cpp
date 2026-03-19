#!/bin/bash

# Simple macOS deployment script for qPCRtools

set -e

VERSION="1.0.0"
PROJECT_NAME="qPCRtools"
BUILD_DIR="build"
APP_BUNDLE="${PROJECT_NAME}.app"

echo "=== Deploying ${PROJECT_NAME} for macOS ==="

# Check if we're in the right directory
if [ ! -d "${BUILD_DIR}" ]; then
    echo "❌ Build directory not found!"
    echo "Please run this script from the project root directory."
    exit 1
fi

cd "${BUILD_DIR}"

# Check if executable exists
if [ ! -f "${PROJECT_NAME}" ]; then
    echo "❌ Executable not found: ${PROJECT_NAME}"
    echo "Please build the project first:"
    echo "  cd build && cmake .. && make"
    exit 1
fi

# Find Qt installation
QT_PREFIX=$(qmake -query QT_INSTALL_PREFIX)
MACDEPLOYQT="${QT_PREFIX}/bin/macdeployqt"

if [ ! -f "${MACDEPLOYQT}" ]; then
    echo "❌ macdeployqt not found at: ${MACDEPLOYQT}"
    exit 1
fi

echo "Qt prefix: ${QT_PREFIX}"
echo "macdeployqt: ${MACDEPLOYQT}"

# Create app bundle structure
echo "Creating app bundle..."
mkdir -p "${APP_BUNDLE}/Contents/MacOS"
mkdir -p "${APP_BUNDLE}/Contents/Resources"
mkdir -p "${APP_BUNDLE}/Contents/Frameworks"

# Create Info.plist
cat > "${APP_BUNDLE}/Contents/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>qPCRtools</string>
    <key>CFBundleIdentifier</key>
    <string>com.qpcrtools.app</string>
    <key>CFBundleName</key>
    <string>qPCRtools</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticTermination</key>
    <true/>
    <key>NSSupportsSuddenTermination</key>
    <true/>
</dict>
</plist>
EOF

# Copy executable
cp "${PROJECT_NAME}" "${APP_BUNDLE}/Contents/MacOS/"

# Copy web resources
echo "Copying web resources..."
mkdir -p "${APP_BUNDLE}/Contents/Resources/web"
cp -R ../web/* "${APP_BUNDLE}/Contents/Resources/web/"

# Fix permissions and remove extended attributes from web files
echo "Fixing web files permissions..."
chmod -R u+rwX,go+rX "${APP_BUNDLE}/Contents/Resources/web/"
xattr -rc "${APP_BUNDLE}/Contents/Resources/web/"

# Deploy Qt libraries
echo "Deploying Qt libraries..."
${MACDEPLOYQT} "${APP_BUNDLE}" \
    -verbose=2 \
    -always-overwrite

# Copy translation files
echo "Copying translation files..."
if ls ../build/*.qm 1> /dev/null 2>&1; then
    mkdir -p "${APP_BUNDLE}/Contents/Resources/translations"
    cp ../build/*.qm "${APP_BUNDLE}/Contents/Resources/translations/"
fi

# Fix QtWebEngineProcess helper app frameworks
echo "Fixing QtWebEngineProcess helper app..."
HELPER_APP="${APP_BUNDLE}/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
if [ -d "${HELPER_APP}" ]; then
    mkdir -p "${HELPER_APP}/Contents/Frameworks"
    # Copy ALL Qt frameworks to helper app
    echo "  Copying all Qt frameworks to helper app..."
    for framework in "${APP_BUNDLE}"/Contents/Frameworks/Qt*.framework; do
        if [ -d "${framework}" ]; then
            fw_name=$(basename "${framework}")
            echo "    Copying ${fw_name}"
            cp -R "${framework}" "${HELPER_APP}/Contents/Frameworks/"
        fi
    done
    # Sign helper app
    codesign --force --deep --sign - "${HELPER_APP}" 2>/dev/null || true
fi

# Fix code signing
echo "Fixing code signature..."
# Remove any existing signature
codesign --remove-signature "${APP_BUNDLE}" 2>/dev/null || true

# Sign all frameworks and libraries
find "${APP_BUNDLE}/Contents/Frameworks" -type f -name "*.dylib" -exec codesign --force --deep --sign - {} \; 2>/dev/null || true
find "${APP_BUNDLE}/Contents/Frameworks" -type d -name "*.framework" -exec codesign --force --deep --sign - {} \; 2>/dev/null || true

# Sign the app bundle
codesign --force --deep --sign - "${APP_BUNDLE}"

# Verify signature
echo "Verifying signature..."
if ! codesign -v "${APP_BUNDLE}" 2>&1; then
    echo "❌ Code signature verification failed"
    echo "This may cause the app to crash on launch"
    echo "Consider getting an Apple Developer certificate for proper signing"
fi

# Test the app bundle
echo "Testing app bundle..."
if [ -x "${APP_BUNDLE}/Contents/MacOS/${PROJECT_NAME}" ]; then
    echo "✅ App bundle is executable"
else
    echo "❌ App bundle is not executable"
    exit 1
fi

# Create DMG
echo "Creating user-friendly DMG..."
DMG_NAME="${PROJECT_NAME}-${VERSION}-macOS.dmg"

# Create a temporary directory
TMP_DIR="dmg_contents"
rm -rf ${TMP_DIR}
mkdir -p ${TMP_DIR}

# Copy app bundle
cp -R "${APP_BUNDLE}" ${TMP_DIR}/

# Create symbolic link to Applications for drag-and-drop installation
echo "Adding Applications shortcut..."
ln -s /Applications "${TMP_DIR}/Applications"

# Create DMG
hdiutil create -volname "${PROJECT_NAME}" \
    -srcfolder ${TMP_DIR} \
    -ov -format UDZO \
    -imagekey zlib-level=9 \
    "${DMG_NAME}"

# Clean up
rm -rf ${TMP_DIR}

echo ""
echo "=== Deployment complete! ==="
echo "📦 App bundle: ${APP_BUNDLE}"
echo "💿 DMG file: ${DMG_NAME}"
echo ""
echo "File sizes:"
ls -lh "${APP_BUNDLE}" 2>/dev/null || true
ls -lh "${DMG_NAME}"
echo ""
echo "User Installation Instructions:"
echo "  1. Double-click the DMG file to open it"
echo "  2. Drag qPCRtools.app to the Applications folder"
echo "  3. Eject the DMG when done"
echo "  4. Launch qPCRtools from Applications or Spotlight"
echo ""
echo "To test the app:"
echo "  open ${APP_BUNDLE}"
echo ""
echo "To test the DMG:"
echo "  open ${DMG_NAME}"
