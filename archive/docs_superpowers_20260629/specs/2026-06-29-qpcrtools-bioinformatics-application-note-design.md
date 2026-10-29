# qPCRtools Bioinformatics Application Note — 设计 Spec

- **日期**：2026-06-29
- **状态**：已与用户确认整体方向，待 spec 审阅
- **作者**：本 spec 由 brainstorming 流程产出，论文实际作者见第 4 节

---

## 1. 目标

为新版 **qPCRtools**（C++/Qt 桌面应用）撰写一篇 **Bioinformatics (Oxford) Application Note**，取代旧的、不完善的草稿（`biorxiv_manuscript.md`，682 行）。

旧版草稿的硬伤（本 spec 明确规避）：
- **学术诚信红线**：Results 中的等价性数据（规整的 `2.4563`、`<0.001%`）、性能 benchmark（16–48×）、用户调查（"20 名学生/95%"）疑似 AI 生成的占位内容，未经真实运行，绝不能用于正式投稿。
- **定位错位**：核心卖点押在"不用编程/教学友好"，但 Bioinformatics 读者都会编程，该卖点无效。
- **格式不符**：markdown + biorxiv 长结构，非 Application Note 标准结构。

## 2. 核心定位与卖点

**叙事主线**：算法等价 + 功能/统计扩展 + 原生跨平台无依赖。

| 卖点 | 面向 Bioinformatics 读者的价值 |
|---|---|
| 与 R 版算法**可证等价** | 可信度——同一份数据两版结果一致，结果可验证 |
| C++/Qt6 **原生桌面**，消除 R 环境与依赖 | 可达性、可重复性——单文件可执行，无需安装 R/包 |
| **跨平台**（Windows/macOS，Linux 源码可编译） | 部署友好 |
| 统计/功能**扩展**（R 版没有） | 真正的方法学增量（见第 3 节） |

明确放弃旧版"no programming required / student-friendly"作为核心卖点。

## 3. C++ 版与 R 版的精确能力关系（诚信基础）

依据：`include/Core/ExpressionCalculator.h`、`StatisticalTest.h`、`StandardCurve.h` 头文件注释。

### 3.1 等价（可做结果一致性验证）
| C++ 实现 | R 版函数 |
|---|---|
| `StandardCurve.calculate` | `CalCurve`（标准曲线 + 扩增效率） |
| `calculateByDeltaCt` (2⁻ᐩΔCt) | `CalExp2dCt` |
| `calculateByDeltaDeltaCt` (2⁻ᐩΔΔCt) | `CalExp2ddCt` |
| `calculateByStandardCurve`（基于效率的 RqPCR 法） | `CalExpRqPCR` |

### 3.2 C++ 版新增（R 版没有，方法学增量）
- 统计检验：**Wilcoxon（秩和/符号秩）**、**配对 t 检验**、**Welch t 检验**、**Shapiro-Wilk 正态性**、**Cohen's d**
- **geNorm 参考基因选择**（代码：`selectReferenceGenesByGeNorm`）
- IQR 离群点剔除
- Excel (.xlsx) 导入
- 交互式可视化（ECharts）、双语界面（中/英）

### 3.3 C++ 版缺失（论文须诚实声明）
- `CalExpCurve`（标准曲线斜率截距法）——头文件注释明确"尚未移植"
- `CalRTable`（反转录 RNA 体积计算）

> **写作纪律**：第 3.3 节的内容必须在论文 Implementation 中如实写明未实现范围，不得回避。等价性验证只覆盖 3.1 的共有方法。

## 4. 论文元信息

- **标题**：*qPCRtools: a cross-platform C++ desktop application for qPCR data analysis with validated equivalence to its R implementation*
- **作者**：Xiang Li（单作者）
- **通讯**：lixiang117423@gmail.com
- **格式**：LaTeX，使用 Bioinformatics 官方 Application Note 模板（Overleaf）
- **投稿要求**：需提名一位 Associate Editor（投稿时处理）

## 5. 正文结构大纲（Application Note，约 2–4 页）

1. **Abstract**（~200 词，无引用）：qPCR 分析痛点 → 工具概述 → 实现要点 → 等价验证结论 → 可用性与许可。
2. **Introduction**（~1 段）：qPCR 数据分析的步骤与复杂性；R 包 qPCRtools 解决了算法但仍有 R 环境/可重复性/平台门槛；商业软件贵且封闭、web 工具下线/隐私问题；本工具目标 = 算法等价 + 原生跨平台 + 消除依赖。
3. **Implementation**（核心节）：
   - **架构**：C++17 / Qt6 / QWebEngineView + Bootstrap 5.3 + ECharts（web 技术构建交互 UI）；核心算法库 `qpcr_core`（静态库）与 GUI 解耦，便于测试与复用；统计计算用 GSL。
   - **算法**：继承 R 版（标准曲线 + 扩增效率、ΔΔCt、RqPCR 效率法）+ **统计扩展**（Wilcoxon / Shapiro-Wilk / Cohen's d / geNorm）。
   - **范围声明**：CalExpCurve（斜率截距法）与 CalRTable 暂未移植（诚实）。
   - **分发**：Windows/macOS 单文件可执行，无需 R/Python；MIT 许可；源码可 Linux 编译。
4. **Results**：
   - 4.1 **典型分析示例**：用 `examples/` 真实数据走 ΔΔCt 与标准曲线流程，展示结果表 + 可视化。
   - 4.2 **与 R 版的等价性验证**：同一数据在 R 版与 C++ 版逐项对比（见第 7 节）。
5. **Availability**：GitHub 仓库 + releases 二进制 + MIT + README 文档 + `tests/` + **可复现的 R↔C++ 对比脚本作补充材料**（Bioinformatics 极重视可重复性，加分项）。
6. **References**：精简，约 10–15 篇（MIQE、ΔΔCt、Pfaffl 效率模型、R 版原文、ggplot2、关键 qPCR 工具综述等）。

## 6. 图规划（2–3 张）

- **Figure 1 — 软件概览**：界面截图 + 工作流/架构示意（导入 → 配置 → 分析 → 结果）。
- **Figure 2 — 真实数据结果可视化**：标准曲线图（回归线 + R²/效率）+ 表达量 box/bar（带统计显著性标记）。数据来自 `examples/`。
- **Figure 3（可选）— R vs C++ 等价性**：散点图（C++ 结果 vs R 结果，应落在 y=x 线）或逐项数值对比表/热图。

> 截图须来自真实运行的软件，不得合成。

## 7. 等价性验证方案（精确、可落地、可复现）

- **数据**：`examples/` 两套真实数据
  - `cq.csv` + `design.csv`：fos-glo-myc / Beta Actin，3 处理组 × 3 重复（与 R 版论文 Figure 2 同源）
  - `rqpcr_cq.csv` + `rqpcr_design.csv`：水稻多基因（OSPOX8/OsUBQ/...），含 Eff、CK vs Treatment
- **验证范围**：3 个共有方法 — 标准曲线(CalCurve)、ΔΔCt、RqPCR(CalExpRqPCR)
- **执行**：同一数据在 **R 版 qPCRtools** 与 **C++ 版** 各跑一遍（用户已确认具备 R+C++ 对比能力）
- **对比指标**：
  - 表达量数值（每个基因/组）
  - 统计量与 p 值（t / F / Wilcoxon）
  - 标准曲线：斜率、截距、R²、扩增效率
- **呈现**：
  - 逐项数值对比表（R 结果 | C++ 结果 | 相对差）
  - 散点图（C++ vs R，y=x 参考线）
- **可复现**：对比所用的 R 脚本 + C++ 调用方式 + 输入数据，作为补充材料公开。

## 8. 明确删除的旧版内容

- ❌ 虚构用户调查（"20 名学生 / 10 名研究者 / 95%"）——Application Note 不需要，且无真实依据。
- ❌ 虚构性能 benchmark（16–48× 加速）——除非用户真实运行，否则不写。**当前默认：不做性能 benchmark，只做等价验证。**
- ❌ "No programming required" 作为核心卖点。
- ❌ 教学内容（lesson plan、assessment ideas、teaching resources）——与 Application Note 定位不符。

## 9. 决策记录（已确认，2026-06-29）

1. **作者名单**：仅 **Xiang Li（单作者）**。
2. **性能 benchmark**：**维持默认——只做等价验证，不写性能。**
3. **标题**：**采用第 4 节方向**。

## 10. 下一步

spec 经用户审阅通过后 → invoke **writing-plans** skill，产出各章节的具体写作实施计划（章节顺序、每章要点、图表制作任务、验证执行任务、参考文献整理），再进入实际写作。
