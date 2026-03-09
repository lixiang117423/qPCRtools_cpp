# qPCRtools C++ 版本 - 开发进度

## 项目信息

- **项目名称**: qPCRtools C++ 版本
- **基于**: R 包 qPCRtools (https://github.com/lixiang117423/qPCRtools)
- **开发语言**: C++17
- **GUI 框架**: Qt 6.x
- **当前版本**: 1.0.0 (开发中)

---

## 开发进度

### ✅ 已完成 (约 40%)

#### 1. 项目基础设施
- [x] 项目目录结构
- [x] CMakeLists.txt 构建配置
- [x] README.md 项目说明
- [x] 跨平台构建支持

#### 2. 数据处理模块 (Data)
- [x] **DataFrame 类** (`include/Data/DataFrame.h`)
  - 类似 R data.frame 的数据结构
  - 支持 CSV/Excel 导入导出
  - 数据过滤、分组、聚合操作
  - 统计函数 (mean, sd, median, min, max)

- [x] **CSV 解析器** (`include/Data/CSVParser.h`)
  - 标准格式 CSV 解析
  - 自定义分隔符支持
  - 引号包裹字段处理

- [x] **Excel 导入器** (`include/Data/ExcelImporter.h`)
  - OpenXLSX 库集成
  - 多工作表支持
  - 条件编译支持 (可选)

#### 3. 核心算法模块 (Core)
- [x] **标准曲线计算** (`include/Core/StandardCurve.h`)
  - 线性回归拟合
  - 扩增效率计算 (E = dilution^(-1/slope) - 1)
  - R² 和 P值计算
  - 对应 R 函数: `CalCurve()`

- [x] **表达量计算** (`include/Core/ExpressionCalculator.h`)
  - ΔCt 方法实现
  - ΔΔCt 方法实现
  - 标准曲线方法框架
  - 对应 R 函数: `CalExp2dCt()`, `CalExp2ddCt()`, `CalExpCurve()`

- [x] **统计检验** (`include/Core/StatisticalTest.h`)
  - 独立样本 t检验
  - 配对样本 t检验
  - Wilcoxon 秩和检验
  - Wilcoxon 符号秩检验
  - 单因素 ANOVA
  - Tukey HSD 事后检验
  - Cohen's d 效应量
  - 置信区间计算

---

### 🚧 进行中 (约 10%)

#### 4. GUI 框架
- [ ] 主窗口 (`include/GUI/MainWindow.h`)
- [ ] 向导页面框架
- [ ] 菜单栏和工具栏
- [ ] 语言切换功能

---

### 📋 待完成 (约 50%)

#### 5. 向导页面
- [ ] 数据导入页面
- [ ] 参数配置页面
- [ ] 结果展示页面

#### 6. 可视化模块
- [ ] ggplot2 主题样式 (`include/Utils/GgplotTheme.h`)
- [ ] 标准曲线散点图
- [ ] 箱线图 (Box Plot)
- [ ] 柱状图 (Bar Plot) + 误差线
- [ ] 显著性标注

#### 7. 工具类
- [ ] 异常值检测器 (IQR 方法)
- [ ] 导出管理器 (PDF/Excel/图片)

#### 8. 国际化 (i18n)
- [ ] 翻译文件创建
  - [ ] `translations/qpcr_tools_zh_CN.ts`
  - [ ] `translations/qpcr_tools_en_US.ts`
- [ ] 所有界面文本翻译
- [ ] 运行时语言切换

#### 9. 测试与验证
- [ ] 单元测试
- [ ] 与 R 版本结果对比验证
- [ ] 边界情况测试

#### 10. 打包发布
- [ ] Windows 打包脚本
  - [ ] NSIS 安装程序
  - [ ] 便携版 .zip
- [ ] macOS 打包脚本
  - [ ] .app bundle
  - [ ] .dmg 镜像
- [ ] Linux AppImage

---

## 文件统计

### 已创建文件 (14 个)

**头文件 (8 个)**
```
include/
├── Core/
│   ├── ExpressionCalculator.h    # 表达量计算器
│   ├── StandardCurve.h            # 标准曲线计算
│   └── StatisticalTest.h          # 统计检验
└── Data/
    ├── CSVParser.h                # CSV 解析器
    ├── DataFrame.h                # 数据框
    └── ExcelImporter.h            # Excel 导入器
```

**源文件 (6 个)**
```
src/
├── Core/
│   ├── ExpressionCalculator.cpp
│   ├── StandardCurve.cpp
│   └── StatisticalTest.cpp
└── Data/
    ├── CSVParser.cpp
    ├── DataFrame.cpp
    └── ExcelImporter.cpp
```

**配置文件 (2 个)**
- `CMakeLists.txt` - 主构建配置
- `README.md` - 项目说明

---

## 核心功能映射表

| R 函数 | C++ 类 | 状态 |
|--------|--------|------|
| `CalRTable()` | (待实现) | ⏳ |
| `CalCurve()` | `StandardCurve` | ✅ |
| `CalExpCurve()` | `ExpressionCalculator` | ✅ |
| `CalExp2dCt()` | `ExpressionCalculator::calculateByDeltaCt()` | ✅ |
| `CalExp2ddCt()` | `ExpressionCalculator::calculateByDeltaDeltaCt()` | ✅ |
| `CalExpRqPCR()` | (待实现) | ⏳ |

---

## 技术栈

### 已集成
- ✅ Qt6 (Core, Widgets, Charts)
- ✅ Eigen3 (线性代数)
- ✅ GSL (统计计算，可选)
- ✅ OpenXLSX (Excel 支持，可选)

### 待集成
- ⏳ Qt Linguist (国际化)
- ⏳ Qt Charts (图表)
- ⏳ Qt PrintSupport (PDF 导出)

---

## 下一步工作

### 优先级 1: 完成 GUI 基础框架
1. 创建主窗口类
2. 实现向导页面基类
3. 添加菜单栏和工具栏
4. 实现语言切换功能

### 优先级 2: 实现数据导入界面
1. 文件选择对话框
2. 数据预览表格
3. 列映射配置
4. 数据验证

### 优先级 3: 实现核心可视化
1. ggplot2 主题类
2. 标准曲线图表
3. 箱线图组件
4. 柱状图组件

---

## 构建状态

### 依赖检查
```bash
# Qt6
find_package(Qt6 6.5 REQUIRED)

# Eigen3
find_package(Eigen3 3.4 REQUIRED)

# GSL (可选)
find_package(GSL 2.7)

# OpenXLSX (可选)
find_package(OpenXLSX)
```

### 编译状态
- 核心算法模块: ✅ 可编译
- 数据处理模块: ✅ 可编译
- GUI 模块: ⏳ 开发中
- 测试模块: ⏳ 未开始

---

## 预估完成时间

| 模块 | 预估时间 | 状态 |
|------|----------|------|
| 核心算法 | 1 周 | ✅ 已完成 |
| 数据处理 | 3 天 | ✅ 已完成 |
| GUI 框架 | 1 周 | 🚧 进行中 |
| 可视化 | 1 周 | ⏳ 未开始 |
| 国际化 | 2 天 | ⏳ 未开始 |
| 测试验证 | 3 天 | ⏳ 未开始 |
| 打包发布 | 3 天 | ⏳ 未开始 |
| **总计** | **~4 周** | **~50%** |

---

## 备注

- 所有代码遵循 C++17 标准
- 使用 Qt6 进行跨平台 GUI 开发
- 项目结构清晰，便于维护和扩展
- 支持中英文双语界面
- 最终打包体积预计 15-50MB

---

**更新日期**: 2025-01-09
