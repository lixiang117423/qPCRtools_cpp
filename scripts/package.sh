#!/bin/bash

# Package script for qPCRtools
# Creates distributable packages for Windows and macOS

set -e

VERSION=${1:-"1.0.0"}
PROJECT_NAME="qPCRtools"
BUILD_DIR="build"

echo "=== Packaging ${PROJECT_NAME} v${VERSION} ==="

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     MACHINE=Linux;;
    Darwin*)    MACHINE=Mac;;
    MINGW*|MSYS*|CYGWIN*) MACHINE=Windows;;
    *)          MACHINE="UNKNOWN:${OS}"
esac

echo "Detected OS: ${MACHINE}"

if [ "${MACHINE}" = "Mac" ]; then
    echo "=== Creating macOS DMG ==="

    # Build the project
    if [ ! -d "${BUILD_DIR}" ]; then
        mkdir "${BUILD_DIR}"
        cd "${BUILD_DIR}"
        cmake .. -DCMAKE_BUILD_TYPE=Release
    else
        cd "${BUILD_DIR}"
    fi

    make -j$(sysctl -n hw.ncpu)

    # Find Qt installation
    QT_PATH=$(qmake -query QT_INSTALL_PREFIX)
    echo "Qt path: ${QT_PATH}"

    # Get the executable name
    APP_BUNDLE="${PROJECT_NAME}.app"

    # Deploy Qt libraries
    echo "Running macdeployqt..."
    ${QT_PATH}/bin/macdeployqt ${APP_BUNDLE} \
        -appstore \
        -verbose=2

    # Create DMG
    DMG_NAME="${PROJECT_NAME}-${VERSION}-macOS.dmg"
    echo "Creating DMG: ${DMG_NAME}"

    # Create a temporary directory for DMG contents
    TMP_DIR="dmg_temp"
    rm -rf ${TMP_DIR}
    mkdir -p ${TMP_DIR}

    # Copy app bundle to temp directory
    cp -R ${APP_BUNDLE} ${TMP_DIR}/

    # Create DMG
    hdiutil create -volname "${PROJECT_NAME}" \
        -srcfolder ${TMP_DIR} \
        -ov -format UDZO \
        ${DMG_NAME}

    # Clean up
    rm -rf ${TMP_DIR}

    echo "✅ macOS DMG created: ${DMG_NAME}"
    ls -lh ${DMG_NAME}

elif [ "${MACHINE}" = "Windows" ]; then
    echo "=== Creating Windows EXE package ==="

    # Build the project
    if [ ! -d "${BUILD_DIR}" ]; then
        mkdir "${BUILD_DIR}"
        cd "${BUILD_DIR}"
        cmake .. -DCMAKE_BUILD_TYPE=Release
    else
        cd "${BUILD_DIR}"
    fi

    # Build with MSBuild or make
    if command -v msbuild &> /dev/null; then
        msbuild ${PROJECT_NAME}.sln /p:Configuration=Release
    else
        make -j$(nproc)
    fi

    # Deploy Qt libraries
    echo "Running windeployqt..."
    windeployqt.exe --release --no-translations \
        --verbose 2 \
        ${PROJECT_NAME}.exe

    # Create ZIP archive
    ZIP_NAME="${PROJECT_NAME}-${VERSION}-Windows.zip"
    echo "Creating ZIP: ${ZIP_NAME}"

    # Create package directory
    PACKAGE_DIR="${PROJECT_NAME}-Windows"
    rm -rf ${PACKAGE_DIR}
    mkdir ${PACKAGE_DIR}

    # Copy executable and all Qt dependencies
    cp ${PROJECT_NAME}.exe ${PACKAGE_DIR}/
    cp -R platforms ${PACKAGE_DIR}/
    cp -R translations ${PACKAGE_DIR}/ 2>/dev/null || true
    cp -R bearer ${PACKAGE_DIR}/ 2>/dev/null || true
    cp -R iconengines ${PACKAGE_DIR}/ 2>/dev/null || true
    cp -R imageformats ${PACKAGE_DIR}/ 2>/dev/null || true
    cp -R styles ${PACKAGE_DIR}/ 2>/dev/null || true
    cp -R sqldrivers ${PACKAGE_DIR}/ 2>/dev/null || true

    # Create ZIP
    powershell Compress-Archive -Path ${PACKAGE_DIR} -DestinationPath ${ZIP_NAME}

    # Clean up
    rm -rf ${PACKAGE_DIR}

    echo "✅ Windows ZIP created: ${ZIP_NAME}"
    ls -lh ${ZIP_NAME}

else
    echo "❌ This script must be run on macOS or Windows"
    exit 1
fi

echo ""
echo "=== Packaging complete! ==="
echo "Package location: $(pwd)"
