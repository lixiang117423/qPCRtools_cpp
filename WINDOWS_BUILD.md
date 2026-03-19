# Windows 编译指南

## 系统要求

- Windows 10 或更高版本（推荐 Windows 11）
- Visual Studio 2019 或更高版本（含 C++ 开发工具）
- CMake 3.20 或更高版本
- Qt 6.5 或更高版本
- Git（可选，用于克隆代码）

## 第一步：安装 Visual Studio

1. 下载 [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
2. 安装时选择 "使用 C++ 的桌面开发" 工作负载
3. 确保包含：
   - MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - Windows 10 SDK（或 Windows 11 SDK）

## 第二步：安装 CMake

1. 下载 [CMake](https://cmake.org/download/)
2. 安装时选择 "Add CMake to the system PATH"

## 第三步：安装 Qt6

### 使用 Qt 在线安装器（推荐）

1. 下载 [Qt 在线安装器](https://www.qt.io/download-qt-installer)
2. 运行安装器，注册免费 Qt 账户
3. 选择安装 Qt 6.5.x 或更高版本
4. **必须选择以下组件**：
   - Qt 6.x.x -> Desktop (MSVC 2022 64-bit)
     - Qt WebEngine（重要！）
     - Qt WebChannel
     - Qt WebEngineWidgets
   - Qt Creator 10（可选）

默认安装路径：`C:\Qt\6.x.x\msvc2022_64`

## 第四步：安装 vcpkg（用于依赖库）

```powershell
# 在 PowerShell 中运行（管理员模式）
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg integrate install
```

### 安装依赖库

```powershell
# Eigen3（必需）
C:\vcpkg\vcpkg install eigen3:x64-windows

# GSL（可选，用于高级统计功能）
C:\vcpkg\vcpkg install gsl:x64-windows

# OpenXLSX（可选，用于 Excel 支持）
C:\vcpkg\vcpkg install openxlsx:x64-windows
```

## 第五步：获取源代码

```bash
git clone https://github.com/lixiang117423/qPCRtools_cpp.git
cd qPCRtools_cpp
```

或从 GitHub 下载 ZIP 并解压。

## 第六步：编译项目

### 使用命令行

1. 打开 "x64 Native Tools Command Prompt for VS 2022"
   - 开始菜单 -> Visual Studio 2022 -> x64 Native Tools Command Prompt

2. 进入项目目录并编译：
```cmd
cd C:\Users\YourName\Projects\qPCRtools_cpp
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.5.0\msvc2022_64" ^
  -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  ..
cmake --build . --config Release
```

### 使用 Qt Creator（更简单）

1. 打开 Qt Creator
2. 文件 -> 打开文件或项目
3. 选择 `CMakeLists.txt`
4. 点击构建按钮（左下角）

## 第七步：打包部署

### 1. 使用 windeployqt 自动收集依赖

```cmd
cd build\Release
C:\Qt\6.5.0\msvc2022_64\bin\windeployqt.exe qPCRtools.exe
```

### 2. 复制 web 资源

```cmd
xcopy ..\..\web web\ /E /I /Y
```

### 3. 创建发布包

```cmd
mkdir release
copy qPCRtools.exe release\
xcopy web release\web\ /E /I /Y
for /d %i in (*) do @copy /Y "%i" release\
windeployqt release\qPCRtools.exe
```

现在 `release` 文件夹包含所有必需的文件，可以压缩为 ZIP 分发。

## 常见问题

### 1. 找不到 Qt6

**错误**：`Could not find Qt6`

**解决**：
- 检查 Qt 安装路径是否正确
- 确保安装了 Qt WebEngine 组件
- 验证 `CMAKE_PREFIX_PATH` 设置

### 2. 找不到 Eigen3

**错误**：`Could not find Eigen3`

**解决**：
```cmd
C:\vcpkg\vcpkg install eigen3:x64-windows
```

### 3. WebEngine 不工作

**症状**：启动后显示 "Failed to load web interface"

**解决**：
- 确保 Qt WebEngine 已安装
- 运行 `windeployqt` 后检查是否有 `QtWebEngineProcess.exe`
- 检查 `web` 文件夹是否复制到可执行文件目录

### 4. 编译时出现 MFC 或 ATL 错误

**解决**：
- 在 Visual Studio Installer 中确保安装了 "使用 C++ 的桌面开发"
- 重启计算机

## 快速构建脚本

创建 `build_windows.bat`：

```batch
@echo off
echo ========================================
echo Building qPCRtools for Windows
echo ========================================

REM 设置路径（根据实际情况修改）
set QT_PATH=C:\Qt\6.5.0\msvc2022_64
set VCPKG_ROOT=C:\vcpkg

REM 检查路径
if not exist "%QT_PATH%" (
    echo ERROR: Qt not found at %QT_PATH%
    pause
    exit /b 1
)

if not exist "%VCPKG_ROOT%" (
    echo ERROR: vcpkg not found at %VCPKG_ROOT%
    pause
    exit /b 1
)

REM 清理旧构建
if exist build rmdir /s /q build
mkdir build
cd build

echo.
echo [1/3] Configuring project...
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  .. || goto :error

echo.
echo [2/3] Building project...
cmake --build . --config Release --parallel || goto :error

echo.
echo [3/3] Deploying application...
cd Release
%QT_PATH%\bin\windeployqt.exe qPCRtools.exe
xcopy ..\..\web web\ /E /I /Y

echo.
echo ========================================
echo Build complete!
echo Output: build\Release\
echo ========================================
pause
exit /b 0

:error
echo.
echo ========================================
echo BUILD FAILED!
echo ========================================
pause
exit /b 1
```

使用方法：
1. 修改脚本中的路径（Qt、vcpkg）
2. 双击运行 `build_windows.bat`

## 测试清单

在发布前确保测试：

- [ ] 在干净的 Windows 系统上测试（无需开发环境）
- [ ] 检查 web 界面是否正常加载
- [ ] 测试 CSV 文件导入
- [ ] 测试数据分析功能
- [ ] 测试结果导出
- [ ] 检查中英文界面切换
- [ ] 确认所有依赖 DLL 都已包含

## 发布到 GitHub

Windows 版本可以和 macOS 版本一起发布到 GitHub Releases：

```bash
# 创建 ZIP 包
cd release
powershell Compress-Archive -Path * -DestinationPath qPCRtools-1.0.0-Windows.zip

# 使用上传脚本
./scripts/upload_dmg_to_github.sh <token>  # 也可以上传 ZIP
```

## 相关资源

- [Qt 6 文档](https://doc.qt.io/qt-6/)
- [CMake 文档](https://cmake.org/documentation/)
- [vcpkg 文档](https://vcpkg.io/)
- [Windows 开发最佳实践](https://docs.microsoft.com/windows/win32/dlls/dynamic-link-library-best-practices)

## 获取帮助

如果遇到问题：
1. 查看 [GitHub Issues](https://github.com/lixiang117423/qPCRtools_cpp/issues)
2. 提交新 Issue，包含：
   - Windows 版本（winver）
   - Qt 版本
   - Visual Studio 版本
   - 完整错误信息
   - 构建日志
