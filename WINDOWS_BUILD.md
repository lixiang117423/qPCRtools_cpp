# Windows 版本构建说明

## 关于 Windows 版本

由于当前开发环境是 macOS，无法直接构建 Windows 的 .exe 文件。Windows 版本需要在 Windows 系统上构建。

## 在 Windows 上构建

### 前提条件

1. **安装 Visual Studio 2022**
   - 下载: https://visualstudio.microsoft.com/downloads/
   - 安装时选择"使用 C++ 的桌面开发"工作负载

2. **安装 Qt 6.x**
   - 下载: https://www.qt.io/download-qt-installer
   - 选择开源版本
   - 安装 Qt 6.10.2 (MSVC 2022 64-bit)
   - 确保安装以下组件:
     - Qt 6.10.2
     - Qt WebEngine
     - Qt WebChannel
     - CMake
     - Ninja (可选)

3. **安装 CMake**
   - 下载: https://cmake.org/download/
   - 或使用 Visual Studio Installer

4. **安装 vcpkg** (用于安装依赖库)
   ```bash
   git clone https://github.com/Microsoft/vcpkg
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

5. **安装依赖库**
   ```bash
   vcpkg install eigen3:x64-windows
   vcpkg install gsl:x64-windows
   vcpkg install openxlsx:x64-windows
   ```

### 构建步骤

1. **打开 "Qt 6.10.2 (MSVC 2022 64-bit)" 命令提示符**
   - 从开始菜单搜索"Qt"
   - 选择"Qt 6.10.2 (MSVC 2022 64-bit)"命令提示符

2. **进入项目目录**
   ```bash
   cd C:\path\to\qPCRtools_cpp
   ```

3. **创建构建目录**
   ```bash
   mkdir build
   cd build
   ```

4. **配置 CMake**
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64 ^
     -DCMAKE_BUILD_TYPE=Release ^
     -DCMAKE_PREFIX_PATH="C:/Qt/6.10.2/msvc2022_64" ^
     -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
   ```

5. **构建项目**
   ```bash
   cmake --build . --config Release
   ```

6. **部署 Qt 库**
   ```bash
   windeployqt.exe --release --no-translations --verbose 2 Release\qPCRtools.exe
   ```

7. **测试运行**
   ```bash
   Release\qPCRtools.exe
   ```

8. **创建 ZIP 包**
   ```powershell
   # 创建临时目录
   mkdir qPCRtools-Windows
   cd qPCRtools-Windows

   # 复制可执行文件
   copy ..\Release\qPCRtools.exe .

   # 复制 Qt 依赖目录
   xcopy /E /I ..\Release\platforms platforms
   xcopy /E /I ..\Release\translations translations 2>nul
   xcopy /E /I ..\Release\bearer bearer 2>nul
   xcopy /E /I ..\Release\iconengines iconengines 2>nul
   xcopy /E /I ..\Release\imageformats imageformats 2>nul
   xcopy /E /I ..\Release\styles styles 2>nul

   # 返回上级目录
   cd ..

   # 创建 ZIP
   powershell Compress-Archive -Path qPCRtools-Windows -DestinationPath qPCRtools-1.0.0-Windows.zip

   # 清理
   rmdir /s /q qPCRtools-Windows
   ```

## 自动化构建脚本

创建文件 `build_windows.bat`:

```batch
@echo off
echo === Building qPCRtools for Windows ===

REM 配置路径
set QT_PATH=C:\Qt\6.10.2\msvc2022_64
set VCPKG_ROOT=C:\vcpkg

REM 清理旧构建
if exist build rmdir /s /q build
mkdir build
cd build

REM 配置 CMake
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

REM 构建
cmake --build . --config Release --parallel

REM 部署 Qt 库
windeployqt.exe --release --no-translations --verbose 2 Release\qPCRtools.exe

echo.
echo === Build complete! ===
echo Executable: build\Release\qPCRtools.exe
echo.
echo To create distribution package, see documentation.
cd ..
```

## 常见问题

### Q: CMake 找不到 Qt？
A: 确保 Qt 路径正确，并在 Qt 命令提示符中运行

### Q: 编译错误找不到头文件？
A: 检查 vcpkg 工具链文件路径是否正确

### Q: 运行时缺少 DLL？
A: 使用 windeployqt 部署所有 Qt 依赖

### Q: 如何签名可执行文件？
A: 需要代码签名证书，使用 signtool.exe:
```bash
signtool.exe sign /f certificate.pfx /p password qPCRtools.exe
```

## 替代方案：使用 GitHub Actions

可以配置 GitHub Actions 自动构建 Windows 版本。见 `.github/workflows` 目录。

## 下一步

1. 在 Windows 机器上按照上述步骤构建
2. 测试可执行文件
3. 创建 ZIP 包
4. 上传到 GitHub Release

如果需要帮助，请提交 Issue。
