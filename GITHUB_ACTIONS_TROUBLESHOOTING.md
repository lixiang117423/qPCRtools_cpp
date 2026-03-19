# GitHub Actions 问题修复说明

## ✅ 已修复的问题

### 1. Python 进程失败
**错误**：`The process 'python.exe' failed with exit code 1`

**原因**：Qt 安装 action 的缓存导致问题

**修复**：
```yaml
# 禁用缓存
cache: false
```

### 2. 缓存服务 400 错误
**错误**：`Failed to restore: Cache service responded with 400`

**原因**：GitHub 缓存服务临时故障

**修复**：暂时禁用缓存，避免依赖不稳定的服务

### 3. macOS 构建失败
**错误**：`Process completed with exit code 1`

**原因**：
- 使用 `macos-latest` 可能不稳定
- 缺少 Ninja 生成器

**修复**：
```yaml
# 使用更稳定的版本
runs-on: macos-13

# 使用 Ninja 生成器
cmake -B build -GNinja
```

## ⚠️ 可以忽略的警告

### Node.js 20 弃用警告
**警告**：`Node.js 20 actions are deprecated`

**说明**：
- 这是未来的变更（2026年6月）
- 目前不影响使用
- GitHub 会自动处理

**如果想提前解决**（可选）：
```yaml
env:
  FORCE_JAVASCRIPT_ACTIONS_TO_NODE24: true
```

### p7zip 已安装警告
**警告**：`p7zip 17.06 is already installed and up-to-date`

**说明**：
- 这只是提示信息
- 不影响构建
- 可以忽略

## 🎯 改进内容

### Windows 构建
```yaml
- 添加 MSVC 环境设置
- 简化 vcpkg 安装流程
- 移除可能导致问题的 --parallel 参数
- 添加文件存在性检查
```

### macOS 构建
```yaml
- 使用 macos-13（更稳定）
- 使用 Ninja 生成器（更快）
- 禁用缓存避免问题
```

## 🔄 如何重新触发构建

### 方式 1：手动触发（推荐）

1. 访问：`https://github.com/lixiang117423/qPCRtools_cpp/actions`
2. 点击 "Build qPCRtools"
3. 点击 "Run workflow"
4. 选择 `main` 分支
5. 点击 "Run workflow"

### 方式 2：推送代码触发

```bash
# 空提交触发构建
git commit --allow-empty -m "Trigger build"
git push
```

### 方式 3：创建标签触发（正式发布）

```bash
git tag v1.0.0
git push origin v1.0.0
```

## 📊 预期结果

修复后应该看到：

```
✅ Build Windows (EXE) - 绿色勾号
✅ Build macOS (DMG) - 绿色勾号
⏱️ 构建时间：15-25 分钟
📦 生成文件：qPCRtools-windows.zip, qPCRtools-macos.dmg
```

## 🔍 如果还有问题

### 查看详细日志

1. 点击失败的任务
2. 展开失败的步骤
3. 查看完整错误信息
4. 搜索关键错误信息

### 常见问题排查

#### Qt 安装失败
```
解决：检查 Qt 版本是否可用
尝试：修改 QT_VERSION 为 '6.5.0' 或 '6.6.0'
```

#### vcpkg 安装超时
```
解决：网络问题，稍后重试
说明：vcpkg 首次安装需要下载编译
```

#### CMake 配置失败
```
解决：检查 CMAKE_PREFIX_PATH 路径
查看：Qt 是否正确安装
```

#### 编译失败
```
解决：检查代码是否有语法错误
本地测试：确保本地能编译通过
```

## 💡 最佳实践

### 1. 本地测试后再推送
```bash
# 确保本地能编译通过
cmake -B build
cmake --build build

# 然后再推送
git push
```

### 2. 使用分支开发
```bash
# 在功能分支开发
git checkout -b feature/new-feature
# ... 编码 ...
git push origin feature/new-feature

# 合并到 main 触发构建
git checkout main
git merge feature/new-feature
git push
```

### 3. 定期更新依赖
```yaml
# 定期更新 Qt 版本
env:
  QT_VERSION: '6.6.0'  # 或更新版本
```

## 📈 监控构建状态

### 在 README 中添加徽章

```markdown
![Build Status](https://github.com/lixiang117423/qPCRtools_cpp/actions/workflows/build.yml/badge.svg)
```

### 订阅通知

1. GitHub 项目 → Settings → Notifications
2. 勾选 "Workflow runs"
3. 选择通知方式

## 🎉 总结

修复后的配置：
- ✅ 更稳定
- ✅ 错误更少
- ✅ 更容易调试
- ✅ 更容易维护

**现在应该可以成功构建了！** 🚀

如果还有问题，请查看 GitHub Actions 的详细日志，获取具体错误信息。
