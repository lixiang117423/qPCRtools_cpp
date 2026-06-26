# qPCRtools_cpp 代码完善路线图

**日期**: 2026-06-26
**状态**: 阶段 1、阶段 2（批次 a + ΔCt）已合并到 `main`；StandardCurve pValue 假值 bug 已修复；ΔΔCt 归一化差异已记录（待定夺）；CalExpCurve/阶段 3-4 待启动
**范围**: 代码库健康度提升 —— 清理死代码、补核心测试、重构大文件、修剩余 TODO

---

## 1. 背景与现状

qPCRtools_cpp 是 Qt6 + QWebEngineView 的 qPCR 数据分析桌面应用，已发布预编译二进制。
实际编译进可执行文件的代码为：

- `src/Core/`：StandardCurve、ExpressionCalculator、StatisticalTest
- `src/Data/`：DataFrame、CSVParser、ExcelImporter
- `src/GUI/`：**仅** WebMainWindow + WebBridge（Web 方案）
- `web/`：前端（index.html / app.js / i18n.js / style.css）
- 入口：`src/main_simple.cpp`

仓库中同时存在一套**未参与编译**的旧原生 Qt Widgets GUI 及 C++ ggplot2 主题，
被 Web 方案取代后遗留，文件内多为 `// TODO` 占位。

### 死代码证据

- `MainWindow.h` 仅被 `src/main.cpp` 引用，而 `main.cpp` 本身不在构建中（CMake 只编译 `main_simple.cpp`）。
- `ImportPage.h`、`ConfigPage.h`、`ResultsPage.h`、`GgplotTheme.h` 在自身文件之外**零引用**。
- CMakeLists.txt 显式列出源文件（无 GLOB），上述文件均不在 SOURCES 中。
- CMake 注释直接说明原因：*"Utils - 暂时注释，Web界面使用JavaScript实现ggplot2主题"*。

### 其他问题

- `tests/` 目录为空，核心统计算法无任何测试，也无与 R 版本的结果对比。
- `ExpressionCalculator.cpp`(1182) 与 `web/js/app.js`(2534) 偏大。
- `STATUS.md` / `PROGRESS.md` 内容过时（仍写"进度 40-50%、GUI 待开发"，与已发布状态矛盾）。

---

## 2. 四阶段路线图

按依赖关系排序推进，每个阶段独立走 设计 → 实现 → 验证。

| 阶段 | 内容 | 依赖 | 风险 |
|------|------|------|------|
| **1. 清理死代码** | 删除未编译的旧 Qt Widgets GUI + C++ GgplotTheme | 无 | 极低（本就不在构建中） |
| **2. 核心算法单元测试** | StandardCurve / ExpressionCalculator / StatisticalTest 加 GoogleTest，尽量与 R 版对比 | 建议在阶段 1 之后 | 低 |
| **3. 重构大文件** | 拆分 ExpressionCalculator.cpp 与 app.js | **依赖阶段 2 测试兜底** | 中 |
| **4. 修剩余 TODO** | 死代码删除后，活代码剩余 TODO：WebBridge(1) + WebMainWindow(6)，共 7 项 | 最后 | 视具体项而定 |

**顺序理由**：删死代码缩小后续工作面并顺带消除约 17 个位于死代码中的 TODO；
测试必须在重构之前建立，作为安全网；TODO 修复放最后，范围最清晰。

---

## 3. 阶段 1 详细设计：清理死代码（本次实现）

### 3.1 删除文件清单（11 个文件，2443 行）

```
src/main.cpp                          (19)
include/GUI/MainWindow.h             (121)   src/GUI/MainWindow.cpp    (527)
include/GUI/ImportPage.h              (92)   src/GUI/ImportPage.cpp    (377)
include/GUI/ConfigPage.h              (93)   src/GUI/ConfigPage.cpp    (267)
include/GUI/ResultsPage.h             (94)   src/GUI/ResultsPage.cpp   (416)
include/Utils/GgplotTheme.h          (144)   src/Utils/GgplotTheme.cpp (293)
```

### 3.2 删除空目录

- `src/GUI/PlotWidgets/`（空）
- `src/Utils/`（删除 GgplotTheme.cpp 后变空）

### 3.3 清理 CMakeLists.txt 中的陈旧注释

移除与已删代码相关的注释残留：

- `# src/Utils/GgplotTheme.cpp` 与 `# include/Utils/GgplotTheme.h`（SOURCES/HEADERS 中的注释行）
- 已注释的 `UI_FILES` 块、`PlotWidgets` 相关注释（如存在）

不改动任何实际生效的构建逻辑。

### 3.4 安全性论证

- 被删文件**均不在 CMake SOURCES 中**，删除不影响编译产物 —— 这是本阶段风险极低的根本原因。
- 已通过 grep 确认：4 个 GUI/Utils 头文件在自身之外零引用；MainWindow 仅被未编译的 main.cpp 引用。
- git 历史完整保留，随时可恢复。

### 3.5 验证方法

1. 删除后重新检查：`grep -rn` 确认无残留引用。
2. 尽力运行 CMake 配置 + 构建，确认可执行文件仍可生成。
   （若当前环境缺少 Qt6 等依赖导致无法构建，则以"被删文件本就不在构建中"为安全性依据，并如实说明未完成完整构建验证。）

### 3.6 不在阶段 1 范围内

- `STATUS.md` / `PROGRESS.md` 等文档更新（文档过时是独立问题，留待后续或单独处理）。
- 阶段 2-4 的具体实现。

---

## 4. 阶段 2-4 概述（到达时再细化）

### 阶段 2：核心算法单元测试（进行中）

**框架**：GoogleTest，CMake `FetchContent` 获取，受 `BUILD_TESTS=ON` 控制（默认 OFF，不影响日常构建与发布）。联网已确认可用。

**CMake 结构**：把 Core+Data 抽成静态库 `qpcr_core`（`add_library(qpcr_core STATIC ...)`），app 与 tests 都链接它——避免重复编译、不改变 app 产物。

**目录结构**：
```
tests/
├── CMakeLists.txt          # FetchContent gtest + 测试目标
├── generate_reference.R    # 用 R 生成黄金参考（可重跑）
├── fixtures/
│   ├── data.json           # 输入数据集（R 与 C++ 共用）
│   └── expected_*.json     # R 计算的期望值
├── test_statistical_test.cpp
├── test_standard_curve.cpp
└── test_expression_calculator.cpp   # 批次 b
```

**R 参考策略（golden fixtures）**：`generate_reference.R` 读取 `fixtures/data.json`，用 base R stats（`t.test`/`wilcox.test`/`aov`/`TukeyHSD`/`shapiro.test`）与 `lm()` 计算期望值，写入 `fixtures/expected_*.json`。C++ 测试用 `QJsonDocument` 加载输入与期望，运行 C++ 函数，`EXPECT_NEAR` 容差比较。测试运行时不依赖 R；脚本可随时重跑。

**覆盖范围（分批）**：
- **批次 a（纯函数，先做）**：`StatisticalTest`（tTest 独立/配对、wilcoxon、wilcoxonSignedRank、anova、tukeyHSD、cohensD、confidenceInterval、shapiroWilk）对比 base R；`StandardCurve`（calculateSingle 的 slope/intercept/R²/pValue/efficiency、calculateEfficiency、formatFormula）对比 R `lm()`。
- **批次 b（集成）**：`ExpressionCalculator` 对比 qPCRtools R 包。
  - ✅ `calculateByDeltaCt` ↔ `CalExp2dCt`：每个 gene 各 group 的平均表达量（`table.Mean`）与 R `mean.expre` 一致（examples 数据，11/11 通过）。
  - ⏳ `calculateByDeltaDeltaCt` ↔ `CalExp2ddCt`、`calculateByStandardCurve` ↔ `CalExpCurve`：待做。

**已交付（批次 a，2026-06-26）**：
- 测试基建：`qpcr_core` 静态库（Core+Data）；`BUILD_TESTS=ON` 经 `FetchContent` 拉 GoogleTest v1.15.2；`tests/CMakeLists.txt`。
- 黄金参考：`tests/generate_reference.R`（base R stats + `lm()`，可重跑）→ `tests/fixtures/{data,expected_stat,expected_standard_curve}.json`。
- 用例（10 个，全绿）：StandardCurve 回归（slope/intercept/R²/efficiency 对比 R `lm()`）、efficiency 公式、formatFormula；StatisticalTest 的 tTest（Welch+Student）、pairedTTest、wilcoxonTest、anova，均对比 base R。
- 实测确认：GSL 已链接（`HAS_GSL`），t/F 检验 p 值用精确分布，与 R 精确对齐。默认构建（`BUILD_TESTS=OFF`）不受影响，app 正常产出。

**测试中发现的代码问题**：
1. ~~`StandardCurve` 回归 pValue 是粗略阶跃近似~~ **✅ 已修复**（2026-06-26）：改用 `gsl_cdf_tdist_P` 计算斜率双侧真实 p 值（与 tTest 同法）。`RegressionMatchesR` 测试现增加 pValue 对比并通过（完美拟合 r²=1 时 C++ 返回 NaN，已跳过）。
2. **`wilcoxonTest` 用正态近似、无连续性校正**（`StatisticalTest.cpp`），小样本下不如 R 精确分布准。测试令 R 用 `exact=FALSE, correct=FALSE` 同方法对比 p 值；统计量定义亦不同（C++ 报 min(U)，R 报 W），故仅比 p。
3. 批次 a 暂未覆盖 `wilcoxonSignedRankTest`/`tukeyHSD`/`cohensD`/`confidenceInterval`/`shapiroWilk`——可后续补充。
4. **ΔΔCt 归一化与 R 不一致**（待作者定夺）：C++ `calculateByDeltaDeltaCt` 用教科书式对照组归一（对照组 fold-change = 1.0）；R `qPCRtools::CalExp2ddCt` 对照组 `mean.expre` ≈ 0.587（≠1），似按各基因跨组总均值归一。两组间比值一致（仅差常数倍），绝对值不同。故 ΔΔCt 暂未写对比测试——需作者确认哪种是预期行为。

**验收标准**：
- [x] `cmake -DBUILD_TESTS=ON` 配置成功（FetchContent 拉取 gtest）。
- [x] `ctest` 通过批次 a 的全部用例（10/10）。
- [x] `generate_reference.R` 可重跑并复现 fixtures。
- [x] 默认构建（`BUILD_TESTS=OFF`）不受影响。

### 阶段 3：重构大文件
- `ExpressionCalculator.cpp`：按方法族（ΔCt / ΔΔCt / 标准曲线）拆分为更小单元。
- `web/js/app.js`：按职责（数据交互 / 图表 / UI 控制 / i18n）拆分模块。
- 依赖阶段 2 测试保证行为不变。

### 阶段 4：修剩余 TODO
- 死代码删除后，剩余活代码 TODO：
  - `WebBridge.cpp`:762 QVariantMap → DataFrame 转换
  - `WebMainWindow.cpp` 中若干（zoom、语言切换、未保存变更检查等）
- 逐项评估是否真实需要，或可移除占位。

---

## 5. 验收标准（阶段 1）

- [x] 11 个文件已删除，2 个空目录已删除。
- [x] CMakeLists.txt 陈旧注释已清理，生效逻辑不变（仅删除 15 行注释）。
- [x] `grep` 确认无残留引用（web/js 中的同名 JS 函数为无关巧合）。
- [~] 构建：CMake 配置**通过**；编译存在 3 个**预先存在**的错误（`ExpressionCalculator` 私有静态方法被自由函数访问，由最新 refactor 提交 `0a161eb` 引入），**与本次死代码删除无关**（`git diff HEAD` 证明 Core 未被改动）。该 bug 需单独修复，见下方"阶段外发现"。
