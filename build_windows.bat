@echo off
REM ========================================
REM qPCRtools Windows 构建脚本
REM ========================================

setlocal EnableDelayedExpansion

echo ========================================
echo qPCRtools Windows Build Script
echo ========================================
echo.

REM 设置路径（根据实际情况修改）
set QT_PATH=C:\Qt\6.5.0\msvc2022_64
set VCPKG_ROOT=C:\vcpkg

REM 检查必需工具
echo [0/4] Checking prerequisites...

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM 检查 Qt
if not exist "%QT_PATH%\bin\qmake.exe" (
    echo ERROR: Qt not found at %QT_PATH%
    echo Please install Qt 6.5+ from https://www.qt.io/download-qt-installer
    echo Make sure to install Qt WebEngine component
    pause
    exit /b 1
)

REM 检查 vcpkg
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo WARNING: vcpkg not found at %VCPKG_ROOT%
    echo Some features may not work without required libraries
    echo Install vcpkg: git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
    echo.
    choice /C YN /M "Continue anyway"
    if errorlevel 2 exit /b 1
)

echo ✓ CMake found
echo ✓ Qt found at %QT_PATH%
echo.

REM 清理旧构建
if exist build (
    echo Cleaning old build directory...
    rmdir /s /q build
)
mkdir build
cd build

echo.
echo [1/4] Configuring project...
echo.

REM 生成项目
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  .. || (
    echo.
    echo ========================================
    echo CONFIGURATION FAILED!
    echo ========================================
    echo.
    echo Common issues:
    echo 1. Qt not found - check QT_PATH in this script
    echo 2. Eigen3 not found - run: vcpkg install eigen3:x64-windows
    echo 3. Visual Studio not found - install VS 2022 with C++ tools
    echo.
    pause
    exit /b 1
)

echo.
echo [2/4] Building project...
echo.

REM 编译
cmake --build . --config Release --parallel || (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    pause
    exit /b 1
)

echo.
echo [3/4] Deploying application...
echo.

cd Release

REM 使用 windeployqt 收集依赖
"%QT_PATH%\bin\windeployqt.exe" qPCRtools.exe --release --no-translations || (
    echo WARNING: windeployqt failed, continuing anyway...
)

REM 复制 web 资源
if exist "..\..\web" (
    echo Copying web resources...
    xcopy ..\..\web web\ /E /I /Y >nul
    echo ✓ Web resources copied
) else (
    echo WARNING: web directory not found!
)

echo.
echo [4/4] Creating release package...
echo.

REM 创建发布目录
cd ..\..
if exist release rmdir /s /q release
mkdir release

REM 复制所有文件
echo Copying files to release directory...
xcopy build\Release\* release\ /E /I /Y >nul

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Output location: %CD%\release
echo.
echo To test the application:
echo   cd release
echo   qPCRtools.exe
echo.
echo To create distribution package:
echo   1. Compress 'release' folder to ZIP
echo   2. Upload to GitHub Releases
echo.
pause
