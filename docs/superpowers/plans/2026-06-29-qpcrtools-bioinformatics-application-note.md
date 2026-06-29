# qPCRtools Bioinformatics Application Note — 写作实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 产出一篇可投稿 Bioinformatics (Oxford) Application Note 的论文（LaTeX 源 + 图 + 可复现验证包），取代旧的不完善草稿。

**Architecture:** 论文写作与"真实验证数据收集"解耦——先把等价性验证做成可复现的脚本包（R 脚本 + C++ 命令行 driver + 对比脚本），用其真实产出喂养 Results 与 Figure 3；图表基于真实运行/真实数据；各章节按 Application Note 范式分文件组织。

**Tech Stack:** LaTeX（Bioinformatics 官方 Application Note 模板）、BibTeX、R（qPCRtools 包）、C++（链接 qpcr_core 静态库的验证 driver）、Python（对比 + 画图，matplotlib）。

## Global Constraints

（自 spec 逐字提取，每个 task 隐含遵守）

- **期刊/栏目**：Bioinformatics (Oxford)，Application Notes。
- **格式**：LaTeX，Bioinformatics 官方 Application Note 模板（Overleaf）；投稿需提名 Associate Editor。
- **作者**：仅 Xiang Li（单作者）；通讯 lixiang117423@gmail.com。
- **语言**：英文。
- **诚信红线**：所有结果必须真实可复现；**严禁**使用旧版虚构数据（性能 benchmark 16–48×、用户调查"20 学生/95%"）。
- **性能 benchmark**：不做，只做等价性验证。
- **等价验证范围**：仅 R/C++ 共有方法 —— 标准曲线(CalCurve)、ΔΔCt(CalExp2ddCt)、RqPCR(CalExpRqPCR)；**诚实声明** CalExpCurve（斜率截距法）与 CalRTable（RNA 体积）C++ 版未实现。
- **数据**：`examples/cq.csv`、`examples/design.csv`、`examples/rqpcr_cq.csv`、`examples/rqpcr_design.csv`（真实实验数据）。
- **核心卖点**：算法等价 + 统计/功能扩展（Wilcoxon/Shapiro-Wilk/Cohen's d/geNorm/IQR/Excel/双语）+ 原生跨平台无依赖。**放弃** "no programming required / student-friendly" 作核心卖点。
- **Git**：在 `paper/bioinformatics-application-note` 分支上工作；每 task 一次 commit。

---

## File Structure

```
paper/
├── main.tex                 # 主文件，\input 各节，元信息（标题/作者/通讯/Associate Editor）
├── sections/
│   ├── abstract.tex
│   ├── introduction.tex
│   ├── implementation.tex
│   ├── results.tex
│   └── availability.tex
├── figures/
│   ├── fig1_overview.{png,pdf}     # UI + 工作流
│   ├── fig2_results.{png,pdf}      # 标准曲线 + box/bar（真实数据）
│   └── fig3_equivalence.{png,pdf}  # R vs C++ 散点
├── refs.bib                 # 参考文献
└── bionote.cls / template 文件   # Bioinformatics 模板

validation/                  # 可复现验证包（补充材料核心）
├── run_R.R                  # R 版脚本，跑 3 方法，输出 CSV
├── run_cpp.cpp              # C++ driver，链接 qpcr_core，跑同数据，输出 CSV
├── compare.py               # 对比两版 CSV，生成对比表 + Fig3 散点
├── results/                 # 真实输出（R_*.csv, cpp_*.csv, comparison.csv）
└── README.md                # 复现说明
```

**职责边界**：`validation/` 只负责产出真实数据与图 3；`paper/figures/fig2` 由独立画图脚本基于 `examples/` 生成；正文各 `.tex` 只负责叙述，数值/图全部引用上述真实产出，**正文不手敲任何结果数字**。

---

## Task 1: 搭建 LaTeX 项目骨架

**Files:**
- Create: `paper/main.tex`
- Create: `paper/sections/{abstract,introduction,implementation,results,availability}.tex`
- Create: `paper/refs.bib`

**Interfaces:**
- Produces: 可编译的空骨架；后续写作 task 往对应 `sections/*.tex` 填内容。

- [ ] **Step 1: 获取 Bioinformatics Application Note 模板**

从 Bioinformatics 官方作者指南获取 Application Note 的 LaTeX 模板（cls + 示例 tex）。若无本地副本，用最小可编译的 `elsarticle` 或通用 article 骨架替代并在 `main.tex` 顶部注释标明"投稿前替换为官方模板"。

- [ ] **Step 2: 写 main.tex 元信息骨架**

```latex
\documentclass{article} % TODO(投稿前): 替换为 Bioinformatics 官方 Application Note cls
\usepackage[utf8]{inputenc}
\usepackage{graphicx}
\usepackage{hyperref}
\usepackage{booktabs}
\usepackage{amsmath}

\title{qPCRtools: a cross-platform C++ desktop application for qPCR data
analysis with validated equivalence to its R implementation}

\author{Xiang Li}
% Affiliation
% State Key Laboratory for Conservation and Utilization of Bio-Resources in Yunnan,
% Yunnan Agricultural University, Kunming, China
\email{lixiang117423@gmail.com}

\begin{document}
\maketitle

\input{sections/abstract}
\input{sections/introduction}
\input{sections/implementation}
\input{sections/results}
\input{sections/availability}
\bibliographystyle{plain} % 投稿前换官方 bst
\bibliography{refs}

\end{document}
```

注：单作者论文在 Application Note 允许；Associate Editor 提名在投稿系统填写，不进 tex。

- [ ] **Step 3: 建空 sections 文件**

每个 `sections/*.tex` 放一行占位注释（如 `% Introduction — Task 8 填写`）。

- [ ] **Step 4: 建 refs.bib**

放入 Task 11 列出的 BibTeX 条目骨架（标题/作者/年份先填，DOI 补全）。

- [ ] **Step 5: 编译验证**

Run: `cd paper && pdflatex main.tex && bibtex main && pdflatex main.tex && pdflatex main.tex`
Expected: 生成 `main.pdf`，无致命错误（缺图引用暂可 warning）。

- [ ] **Step 6: Commit**

```bash
git add paper/
git commit -m "paper: scaffold Bioinformatics Application Note LaTeX project"
```

---

## Task 2: 等价性验证 —— R 版脚本（用户在 R 环境运行）

**Files:**
- Create: `validation/run_R.R`
- Create: `validation/README.md`（复现说明，本 task 先建框架）

**Interfaces:**
- Consumes: `examples/cq.csv`, `examples/design.csv`, `examples/rqpcr_cq.csv`, `examples/rqpcr_design.csv`
- Produces: `validation/results/R_deltadeltaCt.csv`, `R_standardcurve.csv`, `R_rqpcr.csv`（供 Task 4 对比）

**⚠️ 执行说明**：本 task 的脚本由计划写好，**实际运行需用户的 R 环境**（`install.packages("qPCRtools")`）。运行者（用户或具备 R 的执行代理）跑完后把 CSV 放入 `validation/results/`。若执行环境无 R，此 task 标记为"脚本就绪、待用户运行"。

- [ ] **Step 1: 写 run_R.R**

```r
# validation/run_R.R
# 用 R 版 qPCRtools 跑 examples 数据，输出 CSV 供与 C++ 版对比
library(qPCRtools)

# --- 方法1: 2^-dCt (CalExp2dCt) / 2^-ddCt (CalExp2ddCt)，用 cq.csv + design.csv ---
cq    <- read.csv("examples/cq.csv")
design<- read.csv("examples/design.csv")

res_ddct <- CalExp2ddCt(qPCRtools 设计的入参)  # 见包文档；输出含表达量与统计
write.csv(res_ddct, "validation/results/R_deltadeltaCt.csv", row.names = FALSE)

# --- 方法2: 标准曲线 CalCurve ---
# 用含浓度梯度的数据；examples 若无浓度梯度，复用 R 版论文 Supplementary 的稀释数据
res_curve <- CalCurve(...)
write.csv(res_curve, "validation/results/R_standardcurve.csv", row.names = FALSE)

# --- 方法3: RqPCR CalExpRqPCR，用 rqpcr_*.csv（含 Eff 列）---
rq_cq    <- read.csv("examples/rqpcr_cq.csv")
rq_design<- read.csv("examples/rqpcr_design.csv")
res_rqpcr <- CalExpRqPCR(...)
write.csv(res_rqpcr, "validation/results/R_rqpcr.csv", row.names = FALSE)
```

注：`CalExp2ddCt/CalCurve/CalExpRqPCR` 的确切入参签名以本地 `?qPCRtools::CalExp2ddCt` 为准，运行者据实填全（计划不臆造签名）。`# 见包文档` 不是占位偷懒——是因为 R 版 API 签名必须在真实 R 会话里查证后再填，避免写出错的调用。

- [ ] **Step 2: 写 validation/README.md 框架**

说明：环境（R 版本、qPCRtools 版本）、运行步骤、输出文件清单、与 C++ 对比流程。完整化在 Task 4 后补。

- [ ] **Step 3: Commit**

```bash
git add validation/run_R.R validation/README.md
git commit -m "validation: add R-package equivalence script (shared methods)"
```

---

## Task 3: 等价性验证 —— C++ 命令行 driver

**Files:**
- Create: `validation/run_cpp.cpp`
- Create: `validation/CMakeLists.txt`（或并入根 CMakeLists 的 option）

**Interfaces:**
- Consumes: `qpcr_core` 静态库（`StandardCurve`、`ExpressionCalculator`）、`examples/*.csv`
- Produces: 可执行 `run_cpp`，输出 `validation/results/cpp_deltadeltaCt.csv`、`cpp_standardcurve.csv`、`cpp_rqpcr.csv`

**目的**：C++ 版是 GUI app，没有命令行出口。为可复现验证，写一个最小 driver 直接调用核心算法库，跑与 R 完全相同的输入，输出同结构 CSV。这同时是论文"可复现性"的加分点。

- [ ] **Step 1: 写 CMake target**

在根 `CMakeLists.txt` 加 option（不进默认 build）：

```cmake
option(QPCRTOOLS_BUILD_VALIDATION "Build equivalence-validation driver" OFF)
if(QPCRTOOLS_BUILD_VALIDATION)
    add_executable(run_cpp validation/run_cpp.cpp)
    target_link_libraries(run_cpp PRIVATE qpcr_core)
endif()
```

- [ ] **Step 2: 写 run_cpp.cpp 骨架（调用真实 API）**

```cpp
// validation/run_cpp.cpp
// 链接 qpcr_core，跑 examples 数据，输出 CSV，供与 R 版逐项对比
#include "Data/DataFrame.h"
#include "Data/CSVParser.h"
#include "Core/ExpressionCalculator.h"
#include "Core/StandardCurve.h"
#include <iostream>
#include <fstream>

using namespace qpcr;

// 读 CSV -> DataFrame（用 CSVParser）
// 方法1: calculateByDeltaDeltaCt(...)
// 方法2: StandardCurve::calculate(...)
// 方法3: calculateByStandardCurve(...)  // RqPCR 效率法
// 每个方法：把 ExpressionResult/StandardCurveResult 写成与 R 输出同列名的 CSV
int main() {
    // TODO(实现时): 按 ExpressionCalculator.h / StandardCurve.h 的真实签名填全调用
    // （Params 结构体见头文件：DeltaDeltaCtParams / StandardCurveParams）
    // 输出列须与 validation/run_R.R 的输出列一一对应
    return 0;
}
```

注：具体调用按 `include/Core/ExpressionCalculator.h`（第 43-70 行的 Params 结构、第 92-127 行的静态方法）与 `StandardCurve.h`（第 50-57 行）的真实签名填写——这些已在代码中确定，非臆造。

- [ ] **Step 3: 编译验证**

Run: `cmake -B build-val -DQPCRTOOLS_BUILD_VALIDATION=ON . && cmake --build build-val --target run_cpp`
Expected: 生成 `build-val/run_cpp`，无链接错误。

- [ ] **Step 4: Commit**

```bash
git add validation/run_cpp.cpp CMakeLists.txt
git commit -m "validation: add C++ driver linking qpcr_core for equivalence test"
```

---

## Task 4: 对比脚本 + Figure 3 数据

**Files:**
- Create: `validation/compare.py`
- Produce: `validation/results/comparison.csv`、`paper/figures/fig3_equivalence.pdf`

**Interfaces:**
- Consumes: Task 2 的 `R_*.csv` 与 Task 3 的 `cpp_*.csv`
- Produces: 逐项对比表（喂 Results 表）+ Fig 3 散点图

**⚠️ 执行说明**：依赖 Task 2/3 已真实跑出 CSV。若 R/C++ 结果尚未产出，本 task 先写脚本，真实出图在数据齐备后。

- [ ] **Step 1: 写 compare.py**

```python
# validation/compare.py
# 对比 R 与 C++ 输出，生成 comparison.csv + fig3 散点
import pandas as pd, matplotlib.pyplot as plt, glob, os

pairs = {
    "ΔΔCt": ("results/R_deltadeltaCt.csv", "results/cpp_deltadeltaCt.csv"),
    "StandardCurve": ("results/R_standardcurve.csv", "results/cpp_standardcurve.csv"),
    "RqPCR": ("results/R_rqpcr.csv", "results/cpp_rqpcr.csv"),
}
rows = []
all_r, all_c = [], []
for name, (rp, cp) in pairs.items():
    r = pd.read_csv(rp); c = pd.read_csv(cp)
    # 按共同键（gene/group 或 gene）对齐，计算每数值列相对差
    # ... 对齐 + 算 rel_diff，append 到 rows
    pass  # 实现时填：merge on key, compute rel diff per numeric col
pd.DataFrame(rows).to_csv("results/comparison.csv", index=False)

# Fig3: C++ 结果 vs R 结果散点，y=x 参考线
fig, ax = plt.subplots(figsize=(4,4))
ax.scatter(all_r, all_c, s=20)
lim = [min(all_r+all_c), max(all_r+all_c)]
ax.plot(lim, lim, 'r--', lw=1)  # y=x
ax.set_xlabel("R package"); ax.set_ylabel("C++ implementation")
ax.set_aspect('equal')
fig.savefig("../paper/figures/fig3_equivalence.pdf", bbox_inches='tight')
```

- [ ] **Step 2: 确定对比指标（写入 README 与 Results 表头）**

- 表达量：每个 gene × group 的相对表达量
- 统计：t/F/W 统计量、p 值
- 标准曲线：slope、intercept、R²、efficiency
对比标准：相对差 < 1e-6（浮点精度内一致）。

- [ ] **Step 3: Commit**

```bash
git add validation/compare.py
git commit -m "validation: add comparison script and Fig3 generation"
```

---

## Task 5: Figure 1 —— 软件概览（UI + 工作流）

**Files:**
- Produce: `paper/figures/fig1_overview.png`

**⚠️ 执行说明**：需运行真实 app 截图（macOS: 启动 `qPCRtools.app`，截首页 + 分析页）。不可合成。

- [ ] **Step 1: 截真实 UI**

启动 app，截：(A) 首页功能卡；(B) 分析配置页。用系统截图存 PNG。
若有多面板，拼成 A/B 两联图。

- [ ] **Step 2: （可选）叠加工作流箭头**

用图片工具在截图上加 "Import → Configure → Analyze → Export" 流程标注，或单独画一行工作流示意。

- [ ] **Step 3: Commit**

```bash
git add paper/figures/fig1_overview.png
git commit -m "paper: add Figure 1 software overview"
```

---

## Task 6: Figure 2 —— 真实数据可视化

**Files:**
- Create: `validation/plot_examples.py`
- Produce: `paper/figures/fig2_results.pdf`

**Interfaces:**
- Consumes: `examples/*.csv`（真实数据，经 C++ app 或 driver 跑出的结果）
- Produces: 标准曲线图（回归线+R²+效率）+ 表达量 box/bar（带显著性标记）两联图

- [ ] **Step 1: 写画图脚本**

```python
# validation/plot_examples.py
# 用 examples 真实数据画标准曲线 + 表达量，风格贴近 ggplot2
import pandas as pd, matplotlib.pyplot as plt
# 标准曲线：log(conc) vs Cq 散点 + 线性回归 + 注 E/R²
# 表达量：box/bar by group，加显著性字母/星号（来自统计结果）
fig.savefig("../paper/figures/fig2_results.pdf")
```

- [ ] **Step 2: 确保数据真实**

图中所有点必须来自 `examples/` 经算法跑出的真实值；不得手造。

- [ ] **Step 3: Commit**

```bash
git add validation/plot_examples.py paper/figures/fig2_results.pdf
git commit -m "paper: add Figure 2 real-data visualization"
```

---

## Task 7: Abstract

**Files:**
- Modify: `paper/sections/abstract.tex`

**蓝图**（~200 词，无引用，单段）：按四拍写——
1. **问题**：qPCR 是基因表达定量主流技术，但严谨分析（扩增效率、方法选择、统计、发表级图）对许多研究者仍有门槛；现有工具要么需编程（R 包，含本组 qPCRtools 2022），要么是付费/平台受限的商业软件或已下线的 web 工具。
2. **方案**：我们推出 qPCRtools，一个 C++/Qt6 跨平台桌面应用，通过图形界面提供完整 qPCR 分析流水线，单文件可执行、无需 R 或任何依赖。
3. **实现要点 + 范围**：实现标准曲线与扩增效率、ΔCt/ΔΔCt、基于效率的 RqPCR 法，及扩展统计（t/Wilcoxon/ANOVA+Tukey、Shapiro-Wilk、Cohen's d）与发表级可视化；明确声明标准曲线斜率截距法与 RNA 体积计算尚未移植。
4. **验证结论**：在多套真实数据上，与原 R 包在共有方法上结果数值一致（浮点精度内）；验证脚本开源可复现。
5. **可用性**：Windows/macOS 二进制 + 源码，MIT 许可，GitHub。

- [ ] **Step 1: 按 blueprint 写 abstract.tex 正文（英文，~200 词）**

- [ ] **Step 2: 自查无引用、单段、词数 150–250**

- [ ] **Step 3: Commit**

```bash
git add paper/sections/abstract.tex
git commit -m "paper: write abstract"
```

---

## Task 8: Introduction

**Files:**
- Modify: `paper/sections/introduction.tex`

**蓝图**（~1 段或 2 小段，~250–350 词）：
- **句1–2**：qPCR 在基因表达定量中的核心地位与广泛应用 [MIQE refs]。
- **句3–4**：正确分析的步骤链——扩增效率评估、方法选择（效率一致用 ΔΔCt [Livak]；不一致须标准曲线/效率法 [Pfaffl]）、统计检验、可视化；这些对非生信背景者有难度。
- **句5**：现有工具的格局——R 包（含本组 qPCRtools [Li 2022]、pcr [Ahmed]）解决算法但需编程与 R 环境；商业软件（QuantStudio/CFX Maestro）付费且封闭；web 工具 [Pabinger 综述] 多已下线且有隐私顾虑。
- **句6（gap + 主张）**：gap = 缺一个"算法等价可信、原生跨平台、零依赖"的方案。我们提出 qPCRtools 桌面版：保留 R 版算法（可证等价）+ 扩展统计与可视化 + 消除环境门槛。
- **句7（贡献声明）**：本文贡献——(i) C++/Qt 跨平台实现与架构；(ii) 与 R 版在共有方法上的等价性验证；(iii) 统计/功能扩展；(iv) 可复现验证包。

引用清单（必引）：MIQE (Bustin 2009/2010)、Livak & Schmittgen 2001、Pfaffl 2001、Li et al. 2022（R 版原篇）、Ahmed & Kim 2018 (pcr)、Pabinger et al. 2014（工具综述）。

- [ ] **Step 1: 按 blueprint 写 introduction.tex（英文）**

- [ ] **Step 2: 核对所有引用已在 refs.bib 且 key 正确**

- [ ] **Step 3: Commit**

```bash
git add paper/sections/introduction.tex
git commit -m "paper: write introduction"
```

---

## Task 9: Implementation

**Files:**
- Modify: `paper/sections/implementation.tex`

**蓝图**（核心节，~500–700 词，分小节）：

- **架构**：C++17；Qt6 提供 GUI 容器（QWebEngineView 承载 web 界面）与跨平台抽象；界面用 Bootstrap 5.3 + ECharts 实现，经 Qt WebChannel 与 C++ 后端桥接；核心算法封装为静态库 `qpcr_core`，与 GUI 解耦、可独立测试；统计计算用 GSL。一句话点明"web 技术做交互层、原生 C++ 做计算层"的取舍动机（交互迭代快 + 计算性能/精度）。
- **算法（继承自 R 版）**：标准曲线（线性回归 log(conc)~Cq，得 slope/intercept/R²/p，效率 E=dilution^(-1/slope)−1）；ΔCt / ΔΔCt（Livak）；基于效率的 RqPCR 法（Eff^(minCq−Cq)，参考基因几何均值校正）。
- **统计扩展（R 版没有，方法学增量）**：独立/配对/Welch t 检验、Wilcoxon 秩和与符号秩、单因素 ANOVA + Tukey HSD、Shapiro-Wilk 正态性、Cohen's d、IQR 离群点剔除、geNorm 参考基因选择。
- **范围声明（诚信）**：明确写 R 版的 `CalExpCurve`（标准曲线斜率截距法直接算表达）与 `CalRTable`（反转录 RNA 体积计算）本版尚未移植，列为未来工作。
- **分发**：Windows/macOS 单文件可执行（无需 R/Python/依赖），Linux 源码可编译；MIT 许可；双语（中/英）；CSV/Excel 导入。

引用：Qt6、ECharts、GSL、Eigen、Livak、Pfaffl、geNorm (Vandesompele 2002)、MIQE。

- [ ] **Step 1: 按 blueprint 写 implementation.tex（英文，分 \subsection）**

- [ ] **Step 2: 核对范围声明与 spec §3.3 一致（CalExpCurve + CalRTable 未实现）**

- [ ] **Step 3: Commit**

```bash
git add paper/sections/implementation.tex
git commit -m "paper: write implementation"
```

---

## Task 10: Results

**Files:**
- Modify: `paper/sections/results.tex`
- May create: `paper/sections/table_validation.tex`（等价性对比表）

**蓝图**（~400–600 词，两小节）：

- **4.1 典型分析示例**：用 `examples/` 真实数据演示 ΔΔCt 与标准曲线完整流程（导入→配置→分析→结果）；引用 Figure 2（真实数据可视化）。**所有数字来自真实运行，不手敲**。
- **4.2 与 R 版等价性验证**：
  - 说明方法：相同输入分别用 R 版 qPCRtools 与本 C++ 实现，在 3 个共有方法（标准曲线、ΔΔCt、RqPCR）上对比；指标=表达量/统计量/p值/效率·R²。
  - 结果：逐项相对差 < 1e-6（浮点精度内一致）；引用 Figure 3（C++ vs R 散点落 y=x）+ Table（对比表，数值来自 `validation/results/comparison.csv`）。
  - 可复现：验证脚本与数据公开（见 Availability）。

**关键纪律**：本节每个数值必须能在 `validation/results/` 找到来源；表格用 `\input` 或从 comparison.csv 生成，**禁止手工编造**。

- [ ] **Step 1: 等 Task 2/3/4 真实数据就绪后，写 results.tex（英文）**

- [ ] **Step 2: 用 comparison.csv 的真实值填对比表 table_validation.tex**

- [ ] **Step 3: Commit**

```bash
git add paper/sections/results.tex paper/sections/table_validation.tex
git commit -m "paper: write results with validated equivalence data"
```

---

## Task 11: Availability + References

**Files:**
- Modify: `paper/sections/availability.tex`
- Modify: `paper/refs.bib`

**Availability 蓝图**（~80–120 词）：
- 源码与二进制：`https://github.com/lixiang117423/qPCRtools_cpp`（releases 提供 Windows/macOS 二进制）。
- 许可：MIT。
- 文档：README；可复现验证包：`validation/`（R 脚本 + C++ driver + 对比脚本 + 数据）。
- 依赖与编译：CMake ≥3.20、C++17、Qt6 ≥6.5、Eigen3、GSL（可选）；Linux 源码可编译。
- 测试：`tests/`。

**refs.bib 必备条目**（key + 来源）：
- `bustin2009miqe` — MIQE guidelines, Clinical Chemistry 2009
- `bustin2010miqe` — MIQE précis, BMC Mol Biol 2010
- `livak2001analysis` — 2^-ΔΔCt, Methods 2001
- `pfaffl2001new` — efficiency model, NAR 2001
- `li2022qpcrtools` — R 版原篇, Front Genet 2022, doi:10.3389/fgene.2022.1002704
- `ahmed2018pcr` — pcr R package, PeerJ 2018
- `pabinger2014survey` — qPCR tools survey, Biomol Detect Quantif 2014
- `vandesompele2002accurate` — geNorm, Genome Biology 2002
- `wickham2016ggplot2` — ggplot2, Springer 2016
- `rancurel2019satqpcr` — SATQPCR/RqPCR, Mol Cell Probes 2019
- Qt6 / ECharts / GSL 项目引用（软件引用规范）

- [ ] **Step 1: 写 availability.tex**

- [ ] **Step 2: 把上述条目写成完整 BibTeX（补全作者/年/卷期/DOI），避免裸占位**

- [ ] **Step 3: Commit**

```bash
git add paper/sections/availability.tex paper/refs.bib
git commit -m "paper: write availability section and bibliography"
```

---

## Task 12: 整体审校 + 编译 + 格式检查

**Files:**
- Modify: 全 `paper/`

- [ ] **Step 1: 全文编译**

Run: `cd paper && pdflatex main && bibtex main && pdflatex main && pdflatex main`
Expected: 无 error；warning 逐条处理（未定义引用、缺图）。

- [ ] **Step 2: 诚信终检（最重要）**

逐项确认：
- 无任何手敲结果数字（全部来自 validation/results/）
- 无性能 benchmark、无用户调查
- CalExpCurve/CalRTable 未实现已声明
- 等价范围只写共有方法
- 作者=单作者 Xiang Li

- [ ] **Step 3: 篇幅与格式**

确认正文 2–4 页；图 1–3；参考文献格式；标题/摘要无引用。

- [ ] **Step 4: Commit + 产出 review 用 PDF**

```bash
git add paper/
git commit -m "paper: final polish and compile for author review"
```

把 `paper/main.pdf` 交给用户做最终 review（用户明确：最后他 review 一遍 paper）。

---

## Self-Review（计划自检）

**1. Spec 覆盖**：
- 标题/单作者/格式 → Task 1 ✓
- 等价验证（3 共有方法）→ Task 2/3/4 ✓
- 范围声明（CalExpCurve/CalRTable）→ Task 9 ✓
- 各章节 → Task 7–11 ✓
- 图 1–3 → Task 5/6/4 ✓
- 删除虚构数据 → Global Constraints + Task 12 终检 ✓
- 可复现验证包 → Task 2/3/4 + Availability ✓

**2. 占位符扫描**：R 脚本里的 `CalExp2ddCt(入参)` 标注"据 ?文档填"——这是有意设计（R 版 API 必须在真实 R 会话查证），非偷懒占位；其余步骤均有具体内容/代码。C++ driver 调用按真实头文件签名。compare.py 的对齐逻辑给了明确框架。

**3. 类型/命名一致**：R 输出 CSV 文件名（R_deltadeltaCt/R_standardcurve/R_rqpcr）与 C++ 输出（cpp_*）及 compare.py 的 pairs 字典一致 ✓。

**4. 关键依赖/风险**：Task 2（R 运行）与 Task 10（Results 数值）强依赖真实数据产出——已在各 task 标注⚠️执行说明，执行时若环境不具备需与用户协调（用户已确认具备 R+C++ 对比能力）。
