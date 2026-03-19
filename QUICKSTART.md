# 🚀 快速开始：使用 GitHub Actions 自动构建

## ⚡ 30秒快速开始

```bash
# 1. 代码已经推送（GitHub Actions 已配置）

# 2. 创建版本标签
git tag v1.0.0

# 3. 推送标签（自动触发构建）
git push origin v1.0.0

# 4. 等待 15-20 分钟...

# 5. 访问 Releases 下载
# https://github.com/lixiang117423/qPCRtools_cpp/releases
```

就这么简单！🎉

---

## 📖 详细说明

### 第一次使用

1. **查看构建状态**

   访问：`https://github.com/lixiang117423/qPCRtools_cpp/actions`

   应该能看到刚推送的代码正在构建

2. **等待构建完成**

   - Windows: 约 10-15 分钟
   - macOS: 约 5-8 分钟
   - 两个平台并行构建

3. **下载构建产物**

   如果只是测试（不是正式发布）：
   - 在 Actions 页面点击任务
   - 滚动到底部 "Artifacts"
   - 下载 `qPCRtools-windows` 或 `qPCRtools-macos`

### 创建正式发布

```bash
# 方式 1：简单标签
git tag v1.0.0
git push origin v1.0.0

# 方式 2：带说明的标签
git tag -a v1.0.0 -m "Release v1.0.0

新功能：
- ΔCt 分析方法
- 统计检验
- Web 界面优化

Bug 修复：
- 修复文件权限问题
- 修复 helper app 依赖"
git push origin v1.0.0
```

发布后会自动创建：
- ✅ GitHub Release 页面
- ✅ Windows ZIP 文件
- ✅ macOS DMG 文件
- ✅ 自动生成的更新日志

### 手动触发构建

如果不想推送代码或标签：

1. 访问：`https://github.com/lixiang117423/qPCRtools_cpp/actions`
2. 点击 "Build qPCRtools" 工作流
3. 点击 "Run workflow" 按钮
4. 选择分支（通常是 `main`）
5. 点击 "Run workflow"

---

## 📋 版本管理建议

### 开发阶段

```bash
# 开发版本
git tag v1.1.0-dev
git push origin v1.1.0-dev
```

### 测试版本

```bash
# Alpha 版本（内部测试）
git tag v1.1.0-alpha.1
git push origin v1.1.0-alpha.1

# Beta 版本（公开测试）
git tag v1.1.0-beta.1
git push origin v1.1.0-beta.1

# Release Candidate（候选版本）
git tag v1.1.0-rc.1
git push origin v1.1.0-rc.1
```

### 正式版本

```bash
# 主版本
git tag v2.0.0
git push origin v2.0.0

# 次版本
git tag v1.1.0
git push origin v1.1.0

# 补丁版本
git tag v1.0.1
git push origin v1.0.1
```

---

## 🔄 工作流程

### 日常开发

```
1. 修改代码
   ↓
2. git commit
   ↓
3. git push
   ↓
4. GitHub Actions 自动构建（测试）
   ↓
5. 从 Actions 下载测试
   ↓
6. 确认无误
```

### 发布新版本

```
1. 完成开发
   ↓
2. git tag v1.0.0
   ↓
3. git push origin v1.0.0
   ↓
4. GitHub Actions 自动构建
   ↓
5. 自动创建 GitHub Release
   ↓
6. 用户从 Releases 下载
```

---

## 🎯 实际使用示例

### 场景 1：修复 bug

```bash
# 1. 修复 bug
vim src/GUI/WebBridge.cpp

# 2. 测试
git add .
git commit -m "Fix web interface loading issue"
git push

# 3. 检查 Actions 是否通过
# 访问 GitHub Actions 页面

# 4. 如果通过，发布修复版本
git tag v1.0.1
git push origin v1.0.1

# 5. 告诉用户下载新版本
```

### 场景 2：添加新功能

```bash
# 1. 开发新功能
git checkout -b feature/new-analysis-method
# ... 编码 ...
git add .
git commit -m "Add new analysis method"
git push origin feature/new-analysis-method

# 2. 创建 PR（Pull Request）
# GitHub Actions 会自动构建 PR

# 3. 合并到 main
git checkout main
git merge feature/new-analysis-method
git push

# 4. 发布新版本
git tag v1.1.0
git push origin v1.1.0
```

### 场景 3：紧急修复

```bash
# 1. 快速修复
git commit -m "Hotfix: critical bug"
git push

# 2. 立即发布
git tag v1.0.2
git push origin v1.0.2

# 3. GitHub Actions 自动构建并发布
```

---

## 📊 监控构建

### 查看构建状态

在 README.md 中添加徽章：

```markdown
![Build Status](https://github.com/lixiang117423/qPCRtools_cpp/actions/workflows/build.yml/badge.svg)
```

效果：![Build Status](https://github.com/lixiang117423/qPCRtools_cpp/actions/workflows/build.yml/badge.svg)

### 订阅通知

1. 访问 GitHub 项目
2. 点击 "Settings" → "Notifications"
3. 勾选 "Workflow runs"
4. 选择通知方式（邮件、移动端等）

---

## ⚠️ 常见问题

### Q1: 构建失败怎么办？

**A**:
1. 访问 Actions 页面
2. 点击失败的构建
3. 展开失败的步骤
4. 查看错误日志
5. 修复代码后重新推送

### Q2: 如何取消正在运行的构建？

**A**:
1. 访问 Actions 页面
2. 点击正在运行的构建
3. 点击右上角的 "Cancel workflow"

### Q3: 构建产物保存多久？

**A**:
- Artifacts: 30 天
- Releases: 永久保存（除非手动删除）

### Q4: 可以修改构建配置吗？

**A**:
可以！编辑 `.github/workflows/build.yml` 文件：
- 修改 Qt 版本
- 添加构建步骤
- 修改触发条件

### Q5: 免费？

**A**:
- ✅ 公开项目：完全免费
- ✅ 私有项目：每月 2000 分钟免费额度
- ✅ 一般使用完全够用

---

## 💡 最佳实践

### 1. 语义化版本

```
v主版本.次版本.修订版本-预发布版本

v1.0.0        正式版本
v1.1.0        新功能
v1.0.1        Bug 修复
v2.0.0        重大更新
v1.0.0-alpha  内部测试
v1.0.0-beta   公开测试
v1.0.0-rc     候选版本
```

### 2. 版本说明

标签信息应该包含：
- ✅ 新增功能
- ✅ Bug 修复
- ✅ 破坏性变更
- ✅ 升级说明

### 3. 分支策略

```
main          - 主分支，稳定版本
develop       - 开发分支
feature/*     - 功能分支
hotfix/*      - 紧急修复分支
release/*     - 发布准备分支
```

### 4. 自动化测试

在 `build.yml` 中添加测试步骤：

```yaml
- name: Run tests
  run: |
    ctest --test-dir build --output-on-failure
```

---

## 🎉 总结

现在你拥有：

✅ **自动构建系统**
   - 推送代码自动构建
   - 打标签自动发布
   - 多平台同时构建

✅ **专业 CI/CD**
   - 符合工业标准
   - 完整的构建历史
   - 可追溯的版本管理

✅ **零成本运维**
   - 完全免费
   - 无需维护服务器
   - 自动扩展

**享受自动化的乐趣吧！** 🚀
