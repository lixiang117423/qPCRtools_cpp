# 创建 GitHub Release 指南

## 当前状态

✅ macOS DMG 已创建成功: `build/qPCRtools-1.0.0-macOS.dmg` (132MB)
✅ Git 标签已创建并推送: v1.0.0
❌ Windows .exe 需要在 Windows 系统上构建

## 手动创建 GitHub Release

### 步骤 1: 访问 GitHub Releases 页面

点击以下链接或复制到浏览器：

```
https://github.com/lixiang117423/qPCRtools_cpp/releases/new
```

### 步骤 2: 填写 Release 信息

**Tag version:** 选择 `v1.0.0`（已自动创建）

**Release title:**
```
qPCRtools v1.0.0 - 首次正式发布
```

**Description (Release notes):**
```markdown
## qPCRtools v1.0.0 - 首次正式发布

专业的 qPCR 数据分析软件，支持 ΔCt/ΔΔCt 分析方法和多种统计检验。

### 主要功能

✨ **分析方法**
- ΔCt 方法：归一化到参照基因
- ΔΔCt 方法：相对于对照组的相对定量
- 统计检验：t-test, Wilcoxon test, ANOVA

🎨 **用户界面**
- 现代化 Web 界面
- 支持中英文双语
- 直观的数据导入和参数配置
- 实时数据预览

📊 **数据支持**
- CSV 格式导入
- Excel 格式导入
- 数据导出为 CSV/Excel
- ggplot2 风格的可视化

### 系统要求

**macOS:**
- macOS 10.15 或更高版本
- Apple Silicon (M1/M2/M3) 或 Intel 处理器

**Windows:**
- Windows 10 或更高版本
- （Windows 版本即将推出）

### 安装

#### macOS:
1. 下载 `qPCRtools-1.0.0-macOS.dmg`
2. 打开 DMG 文件
3. 将 qPCRtools 拖到应用程序文件夹
4. 右键点击应用选择"打开"（首次运行）

#### Windows:
- Windows 版本正在开发中，详见 [WINDOWS_BUILD.md](https://github.com/lixiang117423/qPCRtools_cpp/blob/main/WINDOWS_BUILD.md)

### 使用指南

1. **导入数据**
   - 导入 Cq 数据文件
   - 导入实验设计文件
   - 或使用内置示例数据

2. **配置分析**
   - 选择分析方法（ΔCt 或 ΔΔCt）
   - 设置参考基因
   - 设置对照组（ΔΔCt 方法）
   - 选择统计检验方法

3. **运行分析**
   - 点击"Run Analysis"
   - 查看结果表格
   - 导出分析结果

### 更新日志

- ✅ 完整的 ΔCt 和 ΔΔCt 分析实现
- ✅ 多种统计检验方法支持
- ✅ Web 界面优化
- ✅ 参数下拉选择功能
- ✅ 中英文双语支持
- ✅ 自动化发布流程

### 已知问题

- Windows 版本尚未完成
- 图表功能暂时禁用

### 贡献

欢迎提交 Issue 和 Pull Request！

### 许可证

MIT License
```

### 步骤 3: 上传 DMG 文件

1. 点击 "Attach binary files" 或 "Attach files"
2. 选择文件: `build/qPCRtools-1.0.0-macOS.dmg`
3. 等待上传完成（文件较大，需要一些时间）

### 步骤 4: 设置为预发布（可选）

如果这是测试版本，勾选 "Set as a pre-release"

### 步骤 5: 发布

点击 **"Publish release"** 按钮

## 验证 Release

发布后，访问以下地址验证：
```
https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.0.0
```

## 下一步

1. ✅ macOS 版本已发布
2. ⏳ Windows 版本需要您在 Windows 机器上构建：
   - 参考 `WINDOWS_BUILD.md`
   - 构建完成后上传 ZIP 文件到同一个 Release
   - 编辑 Release 添加 Windows 下载说明

## Windows 构建快速指南

如果您有 Windows 机器，可以：

1. 克隆仓库到 Windows
2. 按照 `WINDOWS_BUILD.md` 中的步骤构建
3. 运行 `build_windows.bat` 自动构建
4. 上传生成的 ZIP 到 GitHub Release

需要帮助？请提交 Issue！
