# qPCRtools_cpp 代码完善路线图

**日期**: 2026-06-26
**状态**: 阶段 1 已完成并提交（分支 `refactor/remove-dead-code`，2 个 commit）；阶段 2-4 待启动
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

### 阶段 2：核心算法单元测试
- 引入 GoogleTest（CMake `BUILD_TESTS` 选项已预留，当前被注释）。
- 用例覆盖：标准曲线回归（斜率/截距/R²/效率）、ΔCt 与 ΔΔCt 表达量、t 检验/Wilcoxon/ANOVA/Tukey。
- 如能取得 R 版 qPCRtools 的参考输出，加入数值对比用例（容差内一致）。

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
