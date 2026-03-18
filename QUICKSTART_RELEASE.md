# 快速发布指南

## 一、macOS 版本发布

### 1. 构建 App Bundle 和 DMG

```bash
# 在项目根目录执行
bash scripts/macos_deploy.sh
```

这个脚本会：
- ✅ 创建 macOS App Bundle (qPCRtools.app)
- ✅ 部署所有 Qt 依赖库
- ✅ 复制 web 资源文件
- ✅ 创建 DMG 磁盘镜像文件

### 2. 测试应用

```bash
# 测试 App Bundle
open build/qPCRtools.app

# 测试 DMG
open build/qPCRtools-1.0.0-macOS.dmg
```

### 3. 提交到 GitHub Release

```bash
# 安装 GitHub CLI (如果还没安装)
brew install gh

# 认证
gh auth login

# 创建 release 草稿
gh release create v1.0.0 \
  --title "qPCRtools v1.0.0" \
  --notes "Release notes here" \
  --draft

# 上传 DMG 文件
cd build
gh release upload v1.0.0 qPCRtools-1.0.0-macOS.dmg
```

## 二、Windows 版本发布

### 1. 准备工作

在 Windows 上需要：
- Visual Studio 2019/2022
- Qt 6.x (MSVC 版本)
- CMake
- Git for Windows 或 Git Bash

### 2. 构建和打包

在 "Qt 命令提示符" 中执行：

```bash
cd C:\path\to\qPCRtools_cpp
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 部署 Qt 依赖
windeployqt.exe --release --no-translations qPCRtools.exe

# 创建 ZIP (使用 PowerShell 或 7-Zip)
powershell Compress-Archive -Path qPCRtools.exe,platforms,translations -DestinationPath qPCRtools-1.0.0-Windows.zip
```

### 3. 测试

- 将 ZIP 复制到一台干净的 Windows 机器
- 解压并运行 qPCRtools.exe
- 验证所有功能正常

### 4. 上传到 GitHub

```bash
# 使用 GitHub CLI
gh release upload v1.0.0 qPCRtools-1.0.0-Windows.zip
```

## 三、版本标签管理

### 创建新版本标签

```bash
# 更新版本号
# 编辑 CMakeLists.txt: project(qPCRtools VERSION 1.0.1 ...)

# 提交更改
git add .
git commit -m "Bump version to 1.0.1"
git push origin main

# 创建标签
git tag -a v1.0.1 -m "Release version 1.0.1"
git push origin v1.0.1
```

## 四、自动化脚本

### package.sh - 跨平台打包脚本

```bash
# macOS
bash scripts/package.sh 1.0.0

# Windows (Git Bash)
bash scripts/package.sh 1.0.0
```

### create_release.sh - 创建 GitHub Release

```bash
bash scripts/create_release.sh 1.0.0
```

## 五、完整发布流程示例

### macOS 完整流程

```bash
# 1. 更新版本号
vim CMakeLists.txt  # 修改 VERSION

# 2. 提交代码
git add .
git commit -m "Release v1.0.0"
git push origin main

# 3. 创建标签
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0

# 4. 构建和打包
bash scripts/macos_deploy.sh

# 5. 测试
open build/qPCRtools-1.0.0-macOS.dmg

# 6. 创建 GitHub Release
bash scripts/create_release.sh 1.0.0

# 7. 上传 DMG
cd build
gh release upload v1.0.0 qPCRtools-1.0.0-macOS.dmg

# 8. 发布
# 访问 https://github.com/lixiang117423/qPCRtools_cpp/releases
# 点击 "Publish release"
```

## 六、常见问题

### Q: macdeployqt 报错找不到 Qt 库？

A: 确保 Qt 在 PATH 中：
```bash
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
```

### Q: Windows 上运行 EXE 提示缺少 DLL？

A: 使用 windeployqt 部署所有依赖：
```bash
windeployqt.exe --release --no-translations --verbose 2 qPCRtools.exe
```

### Q: 如何验证签名？

macOS:
```bash
codesign --verify --verbose build/qPCRtools.app
```

Windows:
- 需要代码签名证书
- 使用 signtool.exe 签名

## 七、发布检查清单

- [ ] 版本号已更新 (CMakeLists.txt)
- [ ] 代码已提交并推送
- [ ] Git 标签已创建
- [ ] macOS DMG 已创建并测试
- [ ] Windows ZIP 已创建并测试
- [ ] GitHub Release 已创建
- [ ] 所有平台文件已上传
- [ ] Release notes 已填写
- [ ] Release 已发布（非草稿状态）

## 八、参考链接

- Qt 部署文档: https://doc.qt.io/qt-6/deployment.html
- GitHub Releases: https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository
- GitHub CLI: https://cli.github.com/

---

**提示**: 首次发布建议先在测试环境验证整个流程，确保所有步骤正常。
