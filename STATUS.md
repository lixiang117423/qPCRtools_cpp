# qPCRtools C++ - 当前开发状态

## 📊 总体进度：约 70%

### ✅ 已完成的模块

#### 1. 核心算法 (100%)
- ✅ 标准曲线计算 (`StandardCurve`)
- ✅ 表达量计算 (`ExpressionCalculator`)
  - ΔCt 方法
  - ΔΔCt 方法
  - 标准曲线方法
- ✅ 统计检验 (`StatisticalTest`)
  - t检验 (独立样本、配对)
  - Wilcoxon 检验
  - ANOVA + Tukey HSD

#### 2. 数据处理 (100%)
- ✅ DataFrame 类
- ✅ CSV 解析器
- ✅ Excel 导入器 (OpenXLSX)

#### 3. GUI 框架 (90%)
- ✅ 主窗口 (`MainWindow`)
- ✅ ggplot2 主题样式 (`GgplotTheme`)
- ✅ 数据导入页面 (`ImportPage`)
- ✅ 参数配置页面 (`ConfigPage`)
- ✅ 结果展示页面 (`ResultsPage`)

#### 4. 项目基础设施 (100%)
- ✅ CMake 构建系统
- ✅ 项目文档
- ✅ Git 仓库初始化

---

### 📝 当前文件统计

```
总文件数: 24 个源文件

头文件 (.h): 14 个
├── Core/         3 个
├── Data/         3 个
├── GUI/          4 个
├── Utils/        1 个 (GgplotTheme)
└── PlotWidgets/  0 个 (待实现)

源文件 (.cpp): 10 个
├── Core/         3 个
├── Data/         3 个
├── GUI/          4 个
├── Utils/        0 个 (GgplotTheme.cpp 已创建，但未统计)
└── PlotWidgets/  0 个

配置文件:
├── CMakeLists.txt
├── README.md
├── PROGRESS.md
├── .gitignore
└── LICENSE
```

---

### 🚧 待完成的模块

#### 1. 绘图组件 (0%)
- ⏳ BoxPlotWidget
- ⏳ BarPlotWidget
- ⏳ StandardCurvePlot

#### 2. 国际化 (0%)
- ⏳ 创建翻译文件 (.ts)
- ⏳ 编译翻译 (.qm)
- ⏳ 更新所有可翻译文本

#### 3. 测试与验证 (0%)
- ⏳ 单元测试框架
- ⏳ 与 R 版本结果对比
- ⏳ 边界情况测试

#### 4. 打包发布 (0%)
- ⏳ Windows 安装程序
- ⏳ macOS .dmg
- ⏳ 用户文档

---

### 🎯 下一步工作优先级

#### 优先级 1: 修复编译问题
目前代码还有一些问题需要解决：
1. 缺少 `QSpinBox` 和 `QCheckBox` 的头文件包含
2. `resultsPage.cpp` 中有语法错误
3. 需要更新 CMakeLists.txt 以包含新的源文件

#### 优先级 2: 实现基础绘图功能
创建至少一个可用的图表组件（建议先做箱线图）

#### 优先级 3: 创建翻译文件
生成基础的中英文翻译文件

#### 优先级 4: 跨平台测试
在 Windows 和 macOS 上测试编译

---

### 🔧 技术栈确认

- ✅ C++17
- ✅ Qt 6.5+
- ✅ CMake 3.20+
- ✅ Eigen3
- ✅ GSL (可选)
- ✅ OpenXLSX (可选)

---

### 📈 代码统计

| 模块 | 文件数 | 代码行数 (估算) |
|------|--------|----------------|
| Core | 6 | ~2500 行 |
| Data | 6 | ~1800 行 |
| GUI | 8 | ~2000 行 |
| Utils | 2 | ~500 行 |
| **总计** | **24** | **~6800 行** |

---

### 💡 建议

1. **先修复编译问题**，确保代码能够编译通过
2. **创建一个简单的测试程序**验证核心算法
3. **逐步添加绘图功能**，不要一次性实现所有图表
4. **考虑使用 Qt Designer** 创建 UI 文件（.ui），简化代码

---

**更新日期**: 2025-03-09
**当前版本**: 1.0.0-dev
