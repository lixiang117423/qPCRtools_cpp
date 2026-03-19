# GitHub Actions 自动构建指南

## 📖 什么是 GitHub Actions？

GitHub Actions 是 GitHub 提供的 CI/CD（持续集成/持续部署）服务，可以：
- ✅ 自动编译代码
- ✅ 运行测试
- ✅ 创建发布包
- ✅ 部署到生产环境

**最重要的是：完全免费（公开项目）！**

## 🚀 快速开始

### 第一步：检查文件

确保项目中有以下文件：

```
qPCRtools_cpp/
├── .github/
│   └── workflows/
│       └── build.yml          # GitHub Actions 配置
├── vcpkg.json                 # Windows 依赖配置
├── CMakeLists.txt             # CMake 配置
├── web/                       # Web 资源文件
│   ├── index.html
│   ├── css/
│   └── js/
└── ...
```

### 第二步：推送代码

```bash
# 添加所有文件
git add .github/workflows/build.yml vcpkg.json

# 提交
git commit -m "Add GitHub Actions for automated builds"

# 推送到 GitHub
git push origin main
```

### 第三步：触发构建

有两种方式触发构建：

#### 方式 1：打标签发布（推荐）

```bash
# 创建版本标签
git tag v1.0.0

# 推送标签（会自动触发构建）
git push origin v1.0.0
```

#### 方式 2：手动触发

1. 访问 GitHub 项目页面
2. 点击 "Actions" 标签
3. 选择 "Build qPCRtools" 工作流
4. 点击 "Run workflow" 按钮
5. 选择分支，点击 "Run workflow"

### 第四步：查看构建进度

1. 访问：`https://github.com/你的用户名/qPCRtools_cpp/actions`
2. 点击最近的构建任务
3. 可以看到实时日志

### 第五步：下载构建产物

#### 从 Actions 下载（测试版本）

1. 在 Actions 页面点击完成的任务
2. 滚动到页面底部的 "Artifacts" 部分
3. 下载 `qPCRtools-windows` 或 `qPCRtools-macos`

#### 从 Releases 下载（正式版本）

1. 访问：`https://github.com/你的用户名/qPCRtools_cpp/releases`
2. 找到对应的版本（如 v1.0.0）
3. 下载：
   - `qPCRtools-windows.zip` - Windows 版本
   - `qPCRtools-macos.dmg` - macOS 版本

## 📋 构建时间

| 平台 | 构建时间 | 说明 |
|------|----------|------|
| Windows | 10-15 分钟 | 首次构建较慢（缓存后加速） |
| macOS | 5-8 分钟 | 相对较快 |

**总计**：约 15-20 分钟（两个平台并行构建）

## 🔄 自动化工作流

```
推送代码/标签
    ↓
GitHub 自动检测
    ↓
同时启动 Windows 和 macOS 构建
    ↓
Windows Runner                macOS Runner
    ↓                              ↓
安装 Qt6                      安装 Qt6
安装 vcpkg                    安装 Homebrew 依赖
编译 .exe                     编译 .app
收集依赖 DLL                  收集依赖 .framework
复制 web 资源                 复制 web 资源
创建 ZIP                      创建 DMG
    ↓                              ↓
    └──────────┬──────────────────┘
               ↓
        上传到 GitHub Releases
               ↓
          用户下载使用
```

## 🛠️ 配置说明

### 工作流触发条件

```yaml
on:
  push:
    branches: [ main ]      # 推送到 main 分支时
    tags:
      - 'v*'               # 推送标签时（如 v1.0.0）
  pull_request:             # PR 时
    branches: [ main ]
  workflow_dispatch:        # 手动触发
```

### 环境变量

```yaml
env:
  QT_VERSION: '6.5.3'       # Qt 版本
```

可以修改为：
- `6.5.0`
- `6.6.0`
- `6.6.1` 等

### 依赖管理

**Windows**：使用 vcpkg（在 `vcpkg.json` 中配置）
```json
{
  "dependencies": [
    "eigen3",
    "gsl"
  ]
}
```

**macOS**：使用 Homebrew
```bash
brew install eigen gsl
```

## 📦 构建产物

### Windows (qPCRtools-windows.zip)

```
qPCRtools-windows.zip
└── qPCRtools/
    ├── qPCRtools.exe           # 主程序
    ├── Qt6Core.dll             # Qt 依赖
    ├── Qt6Gui.dll
    ├── Qt6Widgets.dll
    ├── Qt6WebEngineCore.dll
    ├── ... (其他 Qt DLL)
    ├── web/                    # Web 资源
    │   ├── index.html
    │   ├── css/
    │   └── js/
    ├── platforms/              # Qt 插件
    └── QtWebEngineProcess.exe  # WebEngine 进程
```

**用户安装**：
1. 解压 ZIP
2. 双击 `qPCRtools.exe`
3. 或创建快捷方式到桌面

### macOS (qPCRtools-macos.dmg)

```
qPCRtools-macos.dmg  (挂载后)
└── qPCRtools.app                # 应用程序包
    └── Contents/
        ├── MacOS/
        │   └── qPCRtools        # 主程序
        ├── Resources/
        │   ├── web/             # Web 资源
        │   └── ...              # 其他资源
        └── Frameworks/          # Qt 框架
```

**用户安装**：
1. 双击 DMG 挂载
2. 拖拽 qPCRtools.app 到 Applications
3. 从 Launchpad 启动

## 🧪 测试构建

### 测试特定平台

在 `.github/workflows/build.yml` 中添加：

```yaml
jobs:
  build-windows:
    if: false  # 添加这行跳过 Windows 构建
```

或使用 `workflow_dispatch` 手动选择：

```yaml
strategy:
  matrix:
    platform: [windows, macos]

on:
  workflow_dispatch:
    inputs:
      platform:
        description: 'Platform to build'
        required: true
        default: 'all'
        type: choice
        options:
          - all
          - windows
          - macos
```

## 🔧 故障排除

### 构建失败

#### 1. Qt 安装失败

**错误**：`Failed to install Qt`

**解决**：
- 检查 Qt 版本是否可用
- 更换为其他版本（如 6.5.0, 6.6.0）

#### 2. vcpkg 超时

**错误**：`vcpkg install timeout`

**解决**：
- 使用缓存：`cache: true`
- 减少依赖：暂时移除 GSL

#### 3. Web 资源缺失

**错误**：`Failed to load web interface`

**解决**：
- 确保 `web/` 文件夹在项目根目录
- 检查文件是否正确复制

### 查看详细日志

1. 点击失败的构建任务
2. 展开失败的步骤
3. 查看完整日志
4. 搜索错误关键词

## 📊 构建统计

在 GitHub 上可以查看：
- ✅ 成功次数
- ❌ 失败次数
- ⏱️ 平均构建时间
- 📦 构建产物大小

访问：`https://github.com/你的用户名/qPCRtools_cpp/actions`

## 🎯 最佳实践

### 1. 版本管理

```bash
# 开发版本
git tag v1.0.0-dev
git push origin v1.0.0-dev

# 测试版本
git tag v1.0.0-beta.1
git push origin v1.0.0-beta.1

# 正式版本
git tag v1.0.0
git push origin v1.0.0
```

### 2. 发布说明

在标签信息中添加发布说明：

```bash
git tag -a v1.0.0 -m "Release v1.0.0

新功能：
- ΔCt 分析方法
- 统计检验

Bug 修复：
- 修复 web 界面加载问题
- 修复文件权限问题"
```

### 3. 持续集成

每次推送代码都会自动构建，可以：
- 尽早发现编译错误
- 确保代码质量
- 自动运行测试

## 🆚 对比：手动 vs GitHub Actions

| 特性 | 手动编译 | GitHub Actions |
|------|----------|----------------|
| 需要 Windows 电脑 | ✅ | ❌ |
| 需要 macOS 电脑 | ✅ | ❌ |
| 自动化 | ❌ | ✅ |
| 多平台同时构建 | ❌ | ✅ |
| 版本管理 | 手动 | 自动 |
| 构建历史 | 无 | 有 |
| 费用 | $0 | $0 |
| 时间成本 | 高 | 低 |

## 💡 进阶功能

### 1. 自动运行测试

```yaml
- name: Run tests
  run: |
    ctest --test-dir build --output-on-failure
```

### 2. 代码质量检查

```yaml
- name: Code quality
  run: |
    clang-format --dry-run --Werror src/*.cpp
```

### 3. 安全扫描

```yaml
- name: Security scan
  uses: aquasecurity/trivy-action@master
  with:
    scan-type: 'fs'
    scan-ref: '.'
    format: 'sarif'
    output: 'results.sarif'
```

### 4. 自动通知

```yaml
- name: Notify on failure
  if: failure()
  uses: actions/github-script@v6
  with:
    script: |
      github.rest.issues.create({
        owner: context.repo.owner,
        repo: context.repo.repo,
        title: 'Build failed',
        body: 'Build failed for commit ${{ github.sha }}'
      })
```

## 📚 相关资源

- [GitHub Actions 文档](https://docs.github.com/en/actions)
- [Qt 安装 Action](https://github.com/jurplel/install-qt-action)
- [vcpkg GitHub Action](https://github.com/lukka/run-vcpkg)
- [构建示例](https://github.com/features/actions)

## 🎉 总结

使用 GitHub Actions 的好处：

1. **零成本**：完全免费
2. **自动化**：推送即构建
3. **多平台**：一次配置，到处运行
4. **专业性**：符合工业标准
5. **可靠性**：官方 Runner，稳定可靠

**现在就开始使用吧！** 🚀
