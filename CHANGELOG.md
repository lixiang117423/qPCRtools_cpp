# CHANGELOG

## 2026-10-15 14:10 — 第②项：Tukey HSD 换真 Tukey（学生化极差分布）；顺带修复输出顺序不确定

按既定顺序推进到第②项（导师意见第 3 条相关的统计正确性）。本轮同时发现并修掉一个此前
未被记录的真实缺陷（结果输出顺序不稳定），一并记录在下面第 2 节。

### 1. `StatisticalTest::tukeyHSD`：从「t + Bonferroni」近似改为真 Tukey（Tukey–Kramer）

- **问题**：原实现虽已算出学生化极差统计量 `Q`，却用 `t` 分布 + Bonferroni（×组对数）算 p 值
  （`StatisticalTest.cpp:718-733`）。两者不是同一个量，p 值系统性偏保守。
- **改法**：新增学生化极差分布上尾的直接实现，p = P(Q_studentizedRange > Q)。
  - 依据 Batista et al. (2017) 的双重积分形式，与 `scipy.stats.studentized_range` 积分核一致
    （SciPy 为 BSD-3-Clause）。**未使用 R 的 `ptukey` 代码**（R nmath 为 GPL，本项目 MIT）。
  - 内层 z 积分：6 段 × 16 点 Gauss–Legendre（[-12,12]，|z|>12 处 φ<2e-32）；
    外层把 χ² 换元为 y=√(χ²/ν) 消掉 u^{ν/2−1} 在 0 处的奇性，按 0.6 等宽分段、每段 12 点。
  - 内层的 `1−k·a^{k−1}` 与尾部分段都用 erf/erfc 写成无相消形式，尾部绝对精度 ~1e-16。
  - df ≥ 1e5 走 df=∞ 极限形式（与 R/SciPy 的渐近切换一致）。
- **精度核对**（2026-10-15，逐点与自适应高精度积分 / 蒙特卡洛对照）：
  - p ≥ 1e-6 区间相对误差 ≤ 3e-13；更小的尾部受双精度相消限制，绝对误差 ~1e-16。
  - 生成 fixture 时同步核对 `ptukey` 与 `TukeyHSD()$p adj`：最大差异 **1.4e-15**。
  - **R 的 `ptukey` 在极小误差自由度处本身不准**：df=2、k=2、q=12 时 R 给 0.0134441，
    而 2×10⁷ 次蒙特卡洛给 0.013639 ± 0.000051、高精度积分给 0.0136061；本实现与后两者一致。
    该情形已单独立测试锁住（`Bugfix.TukeyTinyErrorDegreesOfFreedom`），并在代码注释中写明。
  - 实测单次 p 值耗时 ~0.5 ms（1500 次调用 0.7 s），GUI 每基因调用的代价可接受。
- **测试**：`generate_reference.R` 新增 4 个 Tukey 数据集（3 组等重复 / 4 组不等重复 / 均值接近 /
  差异极大），期望值用 `ptukey` 生成并与 `TukeyHSD()` 交叉核对；新增
  `StatisticalTest.TukeyHSDMatchesR` + `Bugfix.TukeyUsesStudentizedRangeNotBonferroni` +
  `Bugfix.TukeyTinyErrorDegreesOfFreedom` + `Bugfix.TukeyDegenerateInputs`。
  fixture 重生成后与旧 fixture 逐字段比对：`expected_stat.json` 除新增 `tukey` 段外**完全一致**。

### 2. 顺带修复：结果输出顺序不确定（同一次分析的两次运行结果不同）

- **发现过程**：核验第②项时在沙箱里连跑 validation driver 三次，发现
  `cpp_standardcurve.csv` 与 `cpp_rqpcr.csv` **行序、末位浮点在不同运行之间不同**（同一二进制）。
- **根因**：多处用 `QSet<QString>` / `QHash` 的迭代顺序决定遍历与输出顺序，而 Qt 的哈希种子
  每次进程启动都随机（此前只修过 `StandardCurve::calculate(byMean=true)` 的同类问题）。
- **改动**：
  - `StandardCurve::calculate`：基因遍历改为「去重 + 排序」。
  - 新增 `ExpressionCalculator::getUniqueValuesOrdered()`（按输入首次出现顺序去重），替换
    `calculateByStandardCurve`（RqPCR 的 genes/groups/bioreps）、`performANOVA`、`selectReferenceGenesByGeNorm`
    里的 `QSet::values()`；旧的 `getUniqueValues()` 保留但注明只适合包含性判断。
  - `mergeByPosition`：合并表的**列序**改为「Cq 表 → 设计表」首次出现顺序（原为 QSet 迭代序）。
- **效果**：三份 validation 输出连跑三次**逐字节一致**；`cpp_deltadeltaCt.csv` 与已提交版本一致，
  `cpp_rqpcr.csv` 仅末位浮点（1e-16）与 `cpp_standardcurve.csv` 仅行序（现为 Gene1–Gene4 有序）
  与已提交版本不同 —— 已提交的那两份是在旧随机序下生成的，**未覆盖**（等用户决定是否重生成）。
- **测试**：新增 `Bugfix.StandardCurveGeneOrderDeterministic`、
  `ExpressionCalculator.AnovaStatisticsFollowInputOrder`。

### 验证与提交

- `ctest` **47/47 通过**（上一轮 41 → 本轮 +6：4 个 Tukey + 2 个顺序确定化）；
  GUI（`qPCRtools.app`）与等价性 driver（`run_cpp`）均编译通过。
- 三份 validation 输出三次运行逐字节一致（见上）。
- 本文档同批提交并推送 `paper/bioinformatics-application-note`。
- **遗留（下一步）**：③ geNorm 输入改相对量 + 逐步剔除；④ RqPCR 去重/配对；⑤ ΔΔCt 基准决策（要用户拍板）；
  ⑥ SE 公式；⑦ 图 2A 重画 + 公式口径。另有两处待用户决定：0.5 节列的
  `validation/results/*.csv` 是否重生成（仅行序/末位浮点差异）。

## 2026-10-15 08:55 — 按计划推进：第 0 步推送仓库 + 第①项 Wilcoxon 精确检验（含 fixture 脱节修复）

用户指示"按你的逻辑逐步推进"（顺序：先推代码 → 修正确性 → 评估旧版影响 → 补功能 → 改文章）。

### 第 0 步：把本地积压的 13 个提交推上去（公开仓库此前停在 2026-06-28）

- 分 3 批提交后推送（`68e3288..523a59d`，分支 `paper/bioinformatics-application-note`）：
  ① `c64eafa` 目录重构（code/ 工程根、docs/ 编号、archive/ 归档、CI 与脚本路径、.gitignore）；
  ② `8d78054` 投稿就绪的 Application Note + JOSS 短文；
  ③ `523a59d` 项目 CHANGELOG 入库。
- `.gitignore` 新增：`archive/*.docx`（旧 Word 稿仅本地归档）、`软件著作权申请资料/`、
  导师审稿意见 docx、送审用 PDF 副本——这三类**不入公开仓库**（内部/凭据类材料）。
- **CI 实测通过**：`gh workflow run ci.yml --ref paper/bioinformatics-application-note`
  → run **38662150043 成功**（macOS，构建+测试 5m13s）。这同时验证了 `cmake -S code -B build`
  的路径改动与整个目录重构在干净环境可用；8 月 14 日那两次失败的 CI 记录自此有了成功对照。

### 第①项：Wilcoxon 秩和检验改小样本精确分布（导师意见第 1 条后半）

**问题**：原实现只有正态近似。3 vs 3 完全分开时给 p ≈ 0.0495，在 α=0.05 下判为显著；
精确检验的最小 p 是 0.1 —— 属假阳性。

**改动**（`code/src/Core/StatisticalTest.cpp`）：

- 新增 `exactWilcoxonTwoSided()`：按"秩的多重集合"做背包 DP 求秩和精确分布，双侧按
  R 的规则（|w − 均值| ≥ |观测 − 均值|）累加。因为用多重集合，**含并列值同样给精确 p**
  （对应 R ≥ 4.2 的行为；本项目用 R 4.6.1 核对）。
- 启用条件 `n1, n2 ≤ 25`；更大样本仍走正态近似（与 `R exact=FALSE, correct=FALSE` 一致）。
- 抽出 `averageRanks()` 供精确与近似共用，避免两套平均秩规则。

**测试**（`code/tests/test_bugfixes.cpp` 新增 8 例，期望值全部来自 R 4.6.1）：

| 用例 | R 精确 | R 近似（旧实现） | 现值 |
|---|---|---|---|
| 3v3 完全分开 | **0.1** | 0.049535（假阳性） | 0.1 |
| 3v3 部分重叠 | 0.2 | 0.076523 | 0.2 |
| 4v4 完全分开 | 0.0285714 | 0.020921 | 0.0285714 |
| 10v10 完全分开 | 1.0825088e-05 | 1.57e-04 | 1.0825088e-05 |
| 3v3 含并列（1,2,3 vs 3,3,4） | 0.3 | 0.104571 | 0.3 |
| 5v5 含并列 | 0.174603 | 0.110492 | 0.174603 |
| 30v30（走近似，无并列） | — | 0.002097 | 0.002097 |
| 30v30（走近似，含并列） | — | 0.110165 | 0.110165 |

**顺带修掉一处 fixture 与生成脚本脱节**：`tests/fixtures/` 里原有 5 个秩和 + 4 个符号秩用例
（ties1–3、identical、signed1–4）只存在于 fixture 中，`tests/generate_reference.R` 里没有对应条目 ——
一旦照脚本重生成就会**静默删掉这些用例、让两个测试变成空跑**（本轮实际踩到并已回滚）。
处置：把 9 个用例全部补回脚本；脚本的 `qPCRtools` 由硬 `library()` 改为可选依赖（未装时跳过表达量
fixture）；补 `cat(R.version.string)`。重生成后与旧 fixture 逐字段比对：
`data.json` 语义完全一致，秩和 p 值按要求更新为 R 默认（精确），统计量与 4 个符号秩用例值不变。

**验证**：`ctest` **41/41 通过**（含新增 8 例 + 原有与 R 对比的两个 Wilcoxon 测试）；
GUI 与等价性 driver 均编译通过。已提交并推送（`db76aa9`），并触发 CI run 38662778005。

### 下一步（按同一顺序）

② C++ `tukeyHSD` 换真 Tukey（现为 Bonferroni 近似，`StatisticalTest.cpp:624-637`）→
③ geNorm 输入改为相对量 + 逐步剔除 → ④ RqPCR 去重/配对 → ⑤ ΔΔCt 校准基准决策（需用户拍板）→
⑥ SE 公式 → ⑦ 图 2A 重画 + 公式口径写清。

## 2026-10-15 08:43 — 收到导师（Huichuan Huang）2026-10-14 审稿意见；核实其代码审查针对的是**公开仓库的 6 月版本**

用户转来 `manuscript/qPCRtools GUI 审稿意见-黄老师-20261014.docx`（31 KB，纯正文、无批注与修订痕迹，91 段），
已全文提取并逐条对照当前代码核实。**本轮未改动任何代码或稿件**。

### 关键核实结论：版本错配

- 当前分支 `paper/bioinformatics-application-note`，本地 HEAD = `68e3288`；`origin/main` = `575501a`（**2026-06-28**），
  本地领先 **13 个提交**且未推送。
- `git ls-tree -r origin/main | grep -c '^validation/'` = **0** → 公开仓库确实没有 `validation/`，
  与导师所述"公开仓库没有 validation/ 目录、最后一次提交是 6 月 28 日"完全一致。
- 结论：他的代码审查基准 = **2026-06-28 的公开状态**，看不到 2026-08-14 的统计修复与 2026-10-13 的目录整理。

### A. 已在本地修复、但他看不到的

| 他的意见 | 当前代码实况 |
|---|---|
| 第1条 t 检验 p 值为占位（比 1.96/2.58/3.29 阶梯） | 已修：`src/Core/ExpressionCalculator.cpp:85` 的 `performTTest` 委托 `StatisticalTest::tTest(..., equalVariance=false)`（真 Welch） |
| 第1条 Wilcoxon p 值写死 0.05 | 已修：同文件 `:103` 委托 `StatisticalTest::wilcoxonTest`（平均秩 + 并列方差校正） |
| 第1条 效率法处理组永远拿不到 p 值 | 已修：`ExpressionCalculatorStandardCurve.cpp:334-336` 传参顺序 `(controlData, treatData, …, controlGroup, group)` 与结果表合并口径一致 |
| 第1条 无 GSL 时 ANOVA p 值占位 0.05 | 已修：改为正则化不完全 Beta（Lentz 连分数）；全文现仅剩 `alpha=0.05` 之类正常参数 |
| 第3条 公开仓库没有 `validation/` | 本地存在、**未推送**（他"投稿前一定要推"的提醒成立） |
| 第3条 README 的 BibTeX 作者列表错 | 本地 10-13 已按 Crossref 改正（公开版仍是错的） |
| 第3条 清理 `docs/superpowers`、`fix_helper_*.sh`、`PROGRESS.md`、`STATUS.md` | 本地 10-13 已整理（`archive/` + `docs/`），同样未推送 |

### B. 与版本无关、经代码核实**确实仍然存在**的问题

| 他的意见 | 核实结果 |
|---|---|
| 第2条 ΔΔCt 校准基准用全组均值 | 确认仍在，且是**有意对齐 R 版**（`ExpressionCalculatorStandardCurve.cpp:198` 注释写明 control group 不钉到 1）；但稿件公式 (2) 写的是"相对对照组"→ **正文自相矛盾，必须改** |
| 第4条 图2A 回归线用 log4 斜率画在 log10 横轴上 | 确认：`validation/plot_examples.py:33,36,39` 用 `x = np.log10(Conc)`，而 slope/intercept 取自 log4 回归 → 线与点必然错位 |
| 第6条 C++ 的 `tukeyHSD` 实为 Bonferroni 校正的 t 检验 | 确认：`src/Core/StatisticalTest.cpp:624-637` 注释明写"用 t 分布 + Bonferroni 校正近似 Tukey HSD" |
| 第7条 geNorm 用原始 Cq 相除取 log2 | 确认：`ExpressionCalculator.cpp:441-447` 为 `log2(Cq_a / Cq_b)` |
| 第7条 geNorm 只算一轮 M 值取前二 | 确认：函数末尾一次性排序取前 2，无逐步剔除 |
| 第8条 RqPCR 按数值去重 | 确认：`ExpressionCalculatorStandardCurve.cpp:201-224` 有 `uniqueDoubles` lambda，且注释自陈"Replicate that [R behaviour]"；SE 亦按去重后的 n |
| 第1条 Wilcoxon 缺小样本精确检验 | 确认：`StatisticalTest.cpp` 仅正态近似（注释自陈对应 `wilcox.test(exact=FALSE, correct=FALSE)`）；3 vs 3 无结时会给出 ≈0.049，而精确检验最小 0.1 |

### C. 我这边未核实的

- R 包侧各条（`CalExp2ddCt` 基准、`CalExpCurve` 反对数、`cld(level=0.95)`、`ddecreasing` 拼写、`mean(unique())`）——R 包是独立仓库，不在本工作区，**未核**。
- 第2条 SE 公式（"均值 ÷ √n"）：C++ 侧 SE 由 `ExpressionData::seExpression` 提供，本次未逐行追到定义处，**待查**。
- 他给出的 Bioinformatics Application Note 上限（"最多 4 个排版页、约 2600 词或 2000 词 + 1 图；摘要分 Summary / Availability and Implementation / Contact / Supplementary Information"）——**为导师提供口径，官网尚未核实**（此前被 Cloudflare 拦）。

### 待办（他建议的顺序）

先修正确性（Wilcoxon 精确检验、Tukey、geNorm、RqPCR 去重/配对、ΔΔCt 基准决策、SE）→ 评估旧版本影响 →
补功能（恢复作图、分析记录可一键重跑、效率自动判断、质控标记、RDML 导入等）→ 改文章（重新定位、改图2、引言竞品描述、功能对比表）→
仓库清理 + Zenodo DOI + 回复 R 仓库 5 个 issue。

## 2026-10-14 20:57 — 项目分析整理上语雀（89.qPCRtools 总览 + 9 个模块页）

用户要求"把这个项目的分析整理到语雀上去"，参照语雀已有的「87.间作Meta分析」的结构（总览页 + 模块页 + 97/98/99 三张静态页）。
本轮**只写语雀、只读项目文件**，未改动仓库内任何代码或结果；仓库的唯一改动是本条 CHANGELOG。

### 写入结果（语雀知识库「博后项目」id 84563380，namespace `lixiang117423/glg12g`）

- **总览页**：`89.qPCRtools`（原有空页，本轮填写）→ https://www.yuque.com/lixiang117423/glg12g/luh52v7mmext2sah
  （研究目的 / 分析主线 / 模块索引表 / 版本与语料版本表 / 根目录路径 / 关键结论 / 待办 / 子页面）
- **9 个模块页**（全部挂在 89 页面下，depth=2，顺序 01→06→97→98→99）：

| 页面 | doc_id | 链接 |
| --- | --- | --- |
| 01 项目与软件架构 | 288253625 | https://www.yuque.com/lixiang117423/glg12g/yi55d603p2smdwgy |
| 02 核心算法与 R 等价性验证 | 288253631 | https://www.yuque.com/lixiang117423/glg12g/dzrhe4sg9m73boik |
| 03 统计检验实现与测试 | 288253634 | https://www.yuque.com/lixiang117423/glg12g/xy3g9y98viwop05h |
| 04 构建、打包与发布 | 288253638 | https://www.yuque.com/lixiang117423/glg12g/gekf3372z1cgut5e |
| 05 论文与投稿准备 | 288253647 | https://www.yuque.com/lixiang117423/glg12g/yubb91arfm14mglm |
| 06 软件著作权申请材料 | 288253650 | https://www.yuque.com/lixiang117423/glg12g/paczdxzogg6g6chy |
| 97 论文图表对照 | 288253655 | https://www.yuque.com/lixiang117423/glg12g/db96aq84hikt5yc2 |
| 98 环境与软件版本 | 288253657 | https://www.yuque.com/lixiang117423/glg12g/rziedmybcewfwkn6 |
| 99 踩坑与FAQ | 288253661 | https://www.yuque.com/lixiang117423/glg12g/zumvoy7demd1t3x5 |

### 页面内容的口径（全部来自仓库内可核事实，未编造）

- 数字来源：`git log` / `git rev-list --count`、GitHub API（Release 与仓库元数据）、`docs/08_status.md`、
  `code/validation/results/comparison.csv` 与 `check_example_stats.csv`、`manuscript/SUBMISSION_CHECKLIST.md`、
  `软件著作权申请资料/正式资料/申请表信息.txt`，以及本机实测的工具版本（`--version` 输出）。
- 本项目**无超算侧**，各页"超算"一栏统一写"无（全部在本地 Mac 完成）"。
- 事实核对中发现并在页面上如实登记的差异：① `docs/08_status.md` 标注为"2026-06 的工作"，而对应提交
  `ae35a96` 的提交日期是 2026-08-14；② `CHANGELOG.md` 2026-10-13 条目写"94 次提交"，而本地
  `git rev-list --count HEAD` 实测为 107（口径不同，页面按实测值写并注明）；③ 本机 Qt 为 Homebrew 6.12.0、
  CI 固定 6.7.3，已在 98 页与 99 页点明。
- 已知限制一并写进 89 页"关键结论"与 99 页 FAQ：1.1.0 不出图（`displayCharts` 被注释）、
  `CalExpCurve`/`CalRTable` 未移植、界面"Student's t-test"标签实为 Welch、已发布二进制未启用 OpenXLSX
  （不含 Excel 导入）、`shapiroWilkTest` 未实现只返回 NA。

### 未写入语雀的部分（按规范处理）

- **图片需人工贴**：语雀开放 API 无图片上传端点。已在页面里留 3 处 `> 【待贴图】` 占位
  （02 页 Figure 3；05 页 Figure 1、Figure 2），图片由用户本人在编辑器中拖入。
- **两端对齐需人工设置**：API 无 `align` / `justify` 字段，无法通过接口设置，也不会声称已设置。
- 本地草稿、推送脚本、旧文备份与推送记录存于 `/Users/lixiang/tmp/qpcr_yuque/`（临时目录，非仓库内；
  原因是本仓库为 GitHub 公开仓库，不宜放入读取本地凭据的脚本）。语雀为权威副本。

### 待办（用户侧）

1. 在语雀编辑器里贴入 3 张图（路径已在各页来源行给出），并把正文设为两端对齐。
2. JOSS `paper.md` 两处 `[[ ]]`（Research impact 自用证据、AI 披露确认）；Bioinformatics 字数/图表上限；
   基金声明；图表功能是否修好重发 —— 这些同时列在 89 页"待办与待确认"。
3. 本仓库工作区仍有 2026-10-13 两轮目录整理的未提交改动（`git status --short` 138 项，部分已暂存），
   最近一次提交停在 2026-08-14。

## 2026-10-13 20:16 — 修掉 PDF 里引用编号的绿框（hyperref 默认边框）

用户反馈"参考文献的 [6] 有绿色框"。核查：这是 **hyperref 的默认链接边框**，不是 bib 或编号问题——
未加选项时 hyperref 会给所有链接画彩色边框（引用 citecolor 默认绿色、URL 品红/青色，pdfborder 默认 1 pt）。
渲染 PDF 第 2 页确认：**正文中每一处引用 [1]–[11] 都套着绿框**（[6] 只是用户恰好看到的那一处），
参考文献页里带 URL 的条目（[2] ECharts、[10] Qt）则是彩色框。

修复：`manuscript/main.tex` 的 `\usepackage{hyperref}` → `\usepackage[hidelinks]{hyperref}`
（链接仍可点击，但不画框、不变色，投稿标准做法）。重编译并逐页核对：

- `main.pdf` 第 2 页：引用显示为普通 "[4] [3] [6] [8] [7] [5] [1]" ✓ 无框
- `main.pdf` 第 9 页：参考文献列表干净，[2] 的 URL 也无彩色框 ✓
- 页数/体积不变：10 页 / 2.28 MB

备选（若想让链接仍有颜色但无框）：`\usepackage[colorlinks=true, linkcolor=blue, citecolor=blue,
urlcolor=blue, pdfborder={0 0 0}]{hyperref}`。

## 2026-10-13 20:11 — 作者贡献改按 Frontiers 写法；JOSS 稿加入 Huichuan Huang

用户决定三项：

1. **作者贡献段照 Frontiers in Genetics 2022（R 包论文）的写法**，只列 XL 与 HH 两人。
   - 已核对原文：Frontiers 那篇写的是 "XL and HH designed this study and performed the analysis.
     XL, YW, JL, XM, and YL wrote the manuscript. All authors read and approved the final manuscript."
     （来源：Frontiers 文章页 http://doi.org/10.3389/fgene.2022.1002704）
   - `manuscript/sections/backmatter.tex` 改为：
     "XL and HH designed this study and performed the analysis. XL wrote the manuscript.
     Both authors read and approved the final manuscript."
   - 重新编译 `main.pdf` 并核对渲染（该段按预期出现在第 8 页），仍是 10 页 / 2.28 MB。
2. **篇幅：先按现有 `article` 骨架投，不压缩**（正文 2,000 词 / 3 个展示项 / 1 表 / 11 条参考文献，共 10 页）。
   `SUBMISSION_CHECKLIST.md` 第三节第 1 条已记录该决定，并写明若被系统退回可在 15 分钟内出压缩版（约 1,500 词、2 个展示项）。
3. **JOSS 稿加入共同作者**：`paper.md` 的 YAML 作者加 **Huichuan Huang**，两位作者均署单位 1、2
   （State Key Laboratory … in Yunnan；Key Laboratory for Agro-Biodiversity and Pest Control of Ministry of Education, Yunnan Agricultural University）；
   X.L. 现地址写入 Acknowledgements；正文中 "the author" 相应改为 "the authors"（含 AI 使用披露段的人工复核表述）。
   词数 1,484（JOSS 要求 750–1750）。

记录：`SUBMISSION_CHECKLIST.md` 第三节按现状重写（单位/篇幅/贡献三项标记为已解决），并新增"三之二、JOSS 支线"小节。

## 2026-10-13 20:08 — 投稿定稿：加共同通讯作者、嵌入真实 Figure 1、生成 main.pdf

### 作者与单位（用户确认）

- **Xiang Li**：第一作者 **+ 通讯**，单位 1、2；**present address** 为 Yazhouwan National Laboratory, Sanya, Hainan, China。
- **Huichuan Huang**：**共同通讯**，单位 1、2；邮箱 `absklhhc@gmail.com`（与 Frontiers 2022 那篇 R 包论文的通讯邮箱一致，已核对）。
- 单位 1：State Key Laboratory for Conservation and Utilization of Bio-Resources in Yunnan, Yunnan Agricultural University, Kunming, China
- 单位 2：Key Laboratory for Agro-Biodiversity and Pest Control of Ministry of Education, Yunnan Agricultural University, Kunming, China
- 落点：`manuscript/main.tex` 作者块（用 `\parbox{\textwidth}` 承载，避免长单位名出边距）、
  `sections/backmatter.tex`（作者贡献段改为双作者表述并按共同通讯起草；Acknowledgements/Conflict 改复数为 "The authors"）、
  `cover_letter.md`、`SUBMISSION_CHECKLIST.md`。

### Figure 1：改用真实英文界面截图

- 来源：用户此前把图嵌在 `manuscript/main.docx` 里；本轮把它取出为 `manuscript/figures/fig1_home_en.png`
  （2560×1656，英文界面首页，四个功能入口），并转成 RGB（去除 alpha，避免 PDF 透明底问题）。
- `sections/implementation.tex` 的 Figure 1 由原来的三张中文界面拼图改为这一张，图注同步改写
  （说明版本、双语界面、展示英文界面）。
- 原先准备的三张中文截图 `figures/fig1a_import_and_preview.jpg`、`fig1b_analysis_configuration.jpg`、
  `fig1c_results.jpg` 已不被正文引用，**原地保留备选**（未删除）；如需归档或删除请告知。
- 核对：docx 内另两张图与 `figures/fig2_results.png`、`figures/fig3_equivalence.png` **逐字节相同**（sha256 一致），
  说明 docx 与 LaTeX 用的是同一批图。

### `main.docx` 归档

- `git mv manuscript/main.docx archive/manuscript_2026-06_word_version.docx`，避免与 `main.pdf` 混淆；
  `archive/00_README.md` 补登该条（并写明 Figure 1 的来源文件是它）。

### 编译：打通本机 LaTeX 编译通道

- 本机无 TeX Live；`tectonic` 默认需联网抓 bundle（此前多次失败）。
  **解法**：复用 Codex 内置 LaTeX 编译器的缓存目录：
  `TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic main.tex`
  → 离线编译成功（该缓存含 bundles/ 与 formats/，37 MB）。此命令已写入 `SUBMISSION_CHECKLIST.md`。
- 产物：`manuscript/main.pdf`，**10 页 / 2.28 MB**（6 月那版为 9 页，页数是 `article` 骨架原有尺度，非排版错误）。
- 排版修正：作者块与 Results 中的等价性段落各有一处 Overfull hbox（出边距），已用 `\parbox`/`\sloppy` 修掉；
  剩一处 12 pt 的 Overfull 位于作者块末行，渲染检查显示**无可见溢出**，保留。

### 图片查看登记（按"看图后必须补记录"要求）

- 本轮目视查看了 2 张：① `main.docx` 内嵌的 Figure 1 原图（用于确认它是什么、是否适合当 Figure 1 并写图注）；
  ② 编译后 `main.pdf` 第 1 页的渲染图（用于确认作者栏/单位/通讯邮箱/现地址均在版心内、无出边距）。
  用途仅为**核对排版与图件内容**，未把目视结论当作结果数据写入正文。

### 待用户确认

1. 作者贡献段（H.H. 的具体贡献）按惯例起草，请按实际改写。
2. 篇幅：现为 10 页单栏 `article`。若目标刊 Application Note 有严格字数/图表上限（本机未能核实 OUP 官网），
   需要压缩版本可另出。
3. JOSS 那份 `paper.md` 目前仍是单作者（Xiang Li），是否同样加入 Huichuan Huang 作为共同作者？

## 2026-10-13 20:00 — 第二层目录整理：软件本体整体归入 code/

用户要求"再归一个层级：代码归到一个文件夹里"。**关键约束**：`CMakeLists.txt` 内部全部是相对路径
（`src/`、`include/`、`web/`、`tests/`、`examples/`、`translations/`、`macos/Info.plist`），
因此做法是把**整个 CMake 工程整体移入 `code/`**，而不是只搬源码目录——这样工程内部相对路径全部保持有效，
只需改动"从仓库根指向工程"的外部入口。

### 移动（git mv，历史可追）

`code/` ← `CMakeLists.txt`、`vcpkg.json`、`src/`、`include/`、`web/`、`tests/`、`validation/`、
`examples/`、`translations/`、`macos/`、`scripts/`（上一轮已归到 scripts 的 10–13 号脚本随之进入 `code/scripts/`）。
留在仓库根的：`README.md`、`CHANGELOG.md`、`CONTRIBUTING.md`、`LICENSE`、
`paper.md` + `paper.bib`（JOSS 规定须在仓库根）、`.github/`（GitHub 规定须在仓库根）、
`docs/`、`manuscript/`、`archive/`、`软件著作权申请资料/`。

### 外部入口同步修改

| 位置 | 改动 |
|---|---|
| `.github/workflows/ci.yml` | `cmake -B build` → `cmake -S code -B build` |
| `.github/workflows/build.yml` | 两个平台均改 `-S code -B build`；Windows/macOS 部署步骤拷 `web/`、`examples/` 的路径改为 `code/web`、`code/examples` |
| `code/scripts/10_build_local.sh` | 仓库根改为由脚本位置推导（不再写死绝对路径）；`cmake ../code`；拷 `../code/web`、`../code/examples` |
| `code/scripts/11_build_windows.bat` | `cmake ..\code`；`xcopy ..\..\code\web`；头部注明需从仓库根运行 |
| `code/scripts/package.sh`、`macos_deploy.sh` | `cmake ..` → `cmake ../code` |
| `code/scripts/` 其余 9 个脚本（package、macos_deploy、create_friendly_dmg、fix_signature、run_with_logs、install_to_applications、upload_dmg_to_github、upload_to_github、create_release）与 12/13 号 | 统一在开头加 `cd "$(dirname "$0")/../.."`（切回仓库根，保证 `build/` 定位不变、从任意目录调用都可用） |
| `code/validation/compare.py`、`plot_examples.py` | 输出改为 `../manuscript/figures/`；注释里的运行目录改为 `code/` |
| `code/validation/README.md`、`run_R.R`、`run_cpp.cpp`、`check_example_stats.R`、`code/tests/generate_reference.R` | 运行目录说明改为 `code/`；build-val 命令改为 `cmake -S . -B ../build-val` |
| `README.md` | 构建步骤改为 `cmake -S code -B build`；"Project Structure" 目录树重写为含 `code/` 的两级结构 |
| `CONTRIBUTING.md` | 构建/测试命令与 `code/src/Core/`、`code/tests/` 路径 |
| `docs/00_README.md` | 索引重写：顶层 + `code/` 内部两张表；补充"构建产物仍在仓库根 build/"等约定 |
| `docs/03`、`docs/04`、`docs/05`、`docs/06`、`docs/08` | 命令与路径同步（含 Windows 指南里的 `..\code`） |
| `code/scripts/README.md` | 调用方式改为 `bash code/scripts/xxx.sh`（从仓库根） |
| `manuscript/SUBMISSION_CHECKLIST.md` | 代码路径补 `code/` 前缀 |
| `.gitignore` | 修正带斜杠的锚定规则（`tests/js/node_modules/` → `node_modules/`，删除失效的 `tests/build/`、`translations/*.qm`），使规则对 `code/` 下的新路径继续生效 |

### 验证（本地实跑，2026-10-13 19:5x）

| 验证项 | 结果 |
|---|---|
| `cmake -S code -B /tmp/qpcr_verify -DQPCRTOOLS_BUILD_APP=OFF -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH=/opt/homebrew` | 配置通过（GoogleTest 正常拉取） |
| `cmake --build /tmp/qpcr_verify -j8` + `ctest` | **33/33 测试全部通过** |
| `cmake -S code -B /tmp/qpcr_verify_app -DQPCRTOOLS_BUILD_VALIDATION=ON` + 完整构建 | 配置通过；**GUI 应用 `qPCRtools.app` 与等价性 driver `run_cpp` 均编译成功** |
| `cd code && Rscript validation/check_example_stats.R` | 输出与整理前一致（0.5872/1.1095/1.5861，p=0.006027/0.022191） |
| `.gitignore` 新规则 | `code/tests/js/node_modules/`、`code/build/` 已正确忽略；`code/src/`、`code/web/`、`code/CMakeLists.txt` 仍正常跟踪 |

未能在本地验证的：两个 GitHub Actions 工作流（Windows/macOS 发版流程只能在 GitHub 跑），
仅做了路径修改与人工复核，需下次发版时确认。

### 遗留（本轮新建的探针文件，需用户删除）

为验证 `.gitignore` 规则我临时建了两个探针文件：`code/build/__probe/y`（`code/build/` 下只有它）
与 `code/tests/js/node_modules/__probe/x`；删除命令随本次汇报给出。

## 2026-10-13 19:51 — 仓库目录整理（根目录瘦身 + 文档归位 + 历史归档）

用户反馈"项目目录很乱、不知道谁是干什么的"。按 dry-run 流程先出预演清单（含完整对照表）经用户确认后执行。
**未删除任何文件**；仅移动/改名 + 更新引用 + 新增索引文档。执行前用户确认两点：`paper/` 改名为 `manuscript/`；`软件著作权申请资料/` 留在根目录。

### 迁移对照表（旧 → 新）

| 旧路径 | 新路径 |
|---|---|
| `QUICKSTART.md` | `docs/01_quickstart.md` |
| `QUICKSTART_UPLOAD.md` | `docs/02_quickstart_upload.md` |
| `QUICKSTART_RELEASE.md` | `docs/03_quickstart_release.md` |
| `WINDOWS_BUILD.md` | `docs/04_windows_build.md` |
| `CREATE_RELEASE_GUIDE.md` | `docs/05_release_guide.md` |
| `GITHUB_ACTIONS_GUIDE.md` | `docs/06_github_actions.md` |
| `GITHUB_ACTIONS_TROUBLESHOOTING.md` | `docs/07_github_actions_troubleshooting.md` |
| `STATUS.md` | `docs/08_status.md` |
| `biorxiv_manuscript.md` | `archive/manuscript_biorxiv_2026-03.md` |
| `manuscript.docx` | `archive/manuscript_biorxiv_2026-03.docx` |
| `PROGRESS.md` | `archive/PROGRESS_2025-01_obsolete.md` |
| `qPCRtools_en.ts` | `archive/legacy_qpcr_tools_en_stub.ts` |
| `build_local.sh` | `scripts/10_build_local.sh` |
| `build_windows.bat` | `scripts/11_build_windows.bat` |
| `fix_helper_app.sh` | `scripts/12_fix_helper_app.sh` |
| `fix_helper_final.sh` | `scripts/13_fix_helper_final.sh` |
| `paper/` | `manuscript/`（含 sections/、figures/、refs.bib、main.tex/pdf/docx、cover_letter.md、SUBMISSION_CHECKLIST.md） |

移动一律用 `git mv`（或 `mv` 后由 git 识别为 rename），历史可追。

### 引用同步更新

`README.md`（Windows 构建链接 + `paper/` → `manuscript/`）、`CONTRIBUTING.md`、`docs/04_windows_build.md`、
`docs/05_release_guide.md`、`scripts/upload_to_github.sh`（Release 文案里的文档链接）、
`validation/compare.py` 与 `validation/plot_examples.py`（`FIG_OUT` 输出路径改为 `manuscript/figures/`）、
`validation/README.md`、`docs/superpowers/specs/2026-06-26-codebase-improvement-design.md`、
`manuscript/SUBMISSION_CHECKLIST.md`。整理后做了全仓死引用扫描，无残留。

### 新增与改动

- 新增 `docs/00_README.md`：**顶层与 docs/ 每个条目"是什么 + 活跃/历史 + 谁看"的索引表**（直接回答"谁是干撒的"）。
- 新增 `archive/00_README.md`：说明归档内容与"不可复用"原因。
- `README.md` 的 "Project Structure" 小节重写为完整目录树（原来只列了 src/include/web/examples/translations，且未提 tests/validation/docs/manuscript）。
- `.gitignore` 补 `build-*/` 与 `.claude/`：此前 `build-app/`、`build-val/`（合计约 18 MB 构建产物）未被忽略，随时可能被误提交。
- `scripts/12_fix_helper_app.sh`、`scripts/13_fix_helper_final.sh`：原脚本用相对路径 `build/qPCRtools.app`，
  移入 `scripts/` 后加 `cd "$(dirname "$0")/.."`，保证从任意目录调用仍能定位（脚本行为不变）。
- `scripts/README.md` 增补新编号脚本的用途与调用方式，并注明本目录其余脚本尚未统一编号。

### 待用户本人清理（AI 不代删）

`build/`、`build-app/`、`build-tests/`、`build-val/`、`qPCRtools.app/`、空目录 `packaging/`、`resources/`、
`.DS_Store`、`.claude/settings.local.json`，合计约 83 MB；删除命令已随本次汇报给出。

### 说明

- 本次只编号 `docs/` 与 `archive/` 内的文件；`src/include/web/tests/validation/examples/translations/macos/scripts`
  不套 `NN_` 编号——这些名字被 CMake、CI 与打包脚本写死。`docs/superpowers/` 保留英文目录名同理（外部工作流约定）。
- 2026-10-13 之前的 CHANGELOG 条目与 `docs/superpowers/plans/*` 里出现的 `paper/` 路径为当时真实路径，作为历史记录保留未改写。

## 2026-10-13 19:44 — 改稿复核：修正"出图功能"与"p 值"两处与事实不符的表述

在改稿后做逐句回核（稿件说法 ↔ 代码/真实输出）时发现两处必须改的地方：

### 1) 稿件声称的绘图功能在 1.1.0 里是关闭的

- 证据：`web/js/app.js:1195` 调用被注释（`// displayCharts(results); // Charts feature temporarily disabled`）；
  图表导出按钮的事件绑定同样被注释（`app.js:1452-1453`）；`web/index.html` 内无图表容器，
  仅引入 `js/echarts.min.js`。因此当前版本**不出图**。
- 影响：论文摘要与 Implementation 原写"ggplot2-inspired interactive figures"、Implementation 原写
  "Visualization follows a ggplot2-inspired aesthetic..."，与事实不符；论文 Figure 2 实际由
  `validation/plot_examples.py`（matplotlib）依据 C++ 计算结果绘制，不是软件输出。
- 处置：摘要改为"one-click export of both the summary table and the per-replicate raw data"；
  Implementation 明确写"Version 1.1.0 does not render figures... figures of this paper were produced
  from the application's numerical output by the plotting script shipped in the repository"；
  Figure 2 图注补一句来源说明；架构段 ECharts 的表述改为"bundled for the figure view"。
- 建议（待用户定）：把图表功能修好并重新发布（则 Figure 2 可用软件出图，JOSS 的 feature-complete 门禁也更稳），
  或维持现状按现在的措辞投稿。

### 2) Results 里的 p 值写错（重要）

- 原稿写"both significant at $p < 10^{-3}$"。实测：软件结果表截图给出 0.0060 与 0.0222（Welch 两样本 t 检验）。
- 独立复算：新增 `validation/check_example_stats.R`（不依赖 R 包、不依赖软件，从 Cq 原始值按 ΔΔCt 定义算起），
  R 4.6.1（`/opt/homebrew/bin/Rscript`，2026-10-13 19:40 运行）实测：
  group 0 mean=0.5872 sd=0.1214；0.5 mean=1.1095 sd=0.1192，p=0.006027；1 mean=1.5861 sd=0.3229，p=0.022191
  ——与软件截图完全一致，并与 `validation/results/comparison.csv` 的表达量一致。
  同时确认归一化口径：按**全样本** ΔCt 均值归一化，故对照组均值不等于 1（0.5872）。
- 处置：Results 改为 `p = 0.006` 与 `p = 0.022`，补一段说明归一化口径，并注明该组数字可由
  `validation/check_example_stats.R` 零依赖复现；`validation/README.md` 增加对应小节与预期数值表。
- 输出文件：`validation/check_example_stats.R`、`validation/results/check_example_stats.csv`

### 3) 全篇语法校验

- 本地无 TeX Live；`tectonic` 0.17.0 因 bundle 需联网下载无法完成（已试代理 7897 仍不可用，进程已停止）。
- 改用 Codex 内置 LaTeX 编译器对展平副本（图片替换为占位框、去掉 \bibliography）做全篇校验：
  **编译通过（kind=success）**，正文与所有 section 结构无误。

### 4) 其他

- `paper/SUBMISSION_CHECKLIST.md` 增补"第四节 软件端问题（t-test 标签与 Welch 不一致；图表功能关闭）"
  与"第五节 本轮修正的数值错误"。
- 正文词数复核：abstract 226 / introduction 469 / implementation 679 / results 537 / availability 98
  = 正文约 2,009 词 + 3 个展示项 + 1 张表；Application Note 的字数上限仍待用户核对（见清单第三节第 1 项）。

## 2026-10-13 19:33 — 论文改稿：bioRxiv 被拒诊断 + 双线投稿准备（Bioinformatics AN 主线 / JOSS 支线）

背景：用户于 2026-10 反馈 bioRxiv 投稿"审核不通过"（无其他信息，属预印本筛选而非同行评审），
要求评估改稿重投或改投他刊。

### 诊断（证据来自仓库内文件与官方文档）

- 拟投稿的那个版本（`biorxiv_manuscript.md`，682 行；对应 `manuscript.docx`）含**无任何产出支撑的内容**：
  用户调查（L334 起"20 名本科生/10 名研究者"）、性能 benchmark（L246、L535 的 16–48×）、
  Table 2 的规整数值（L517/L522，与 `validation/results/comparison.csv` 真值不符）、
  Supplementary S1–S5（L582 起，文件不存在）、正文引用 Figure 1–5 而实际只有 fig2/fig3 两张、
  稿内自带"请引用本 bioRxiv 预印本"（L653）。标题为"No Programming Required"并附与商业软件的功能对比表。
- bioRxiv 官方投稿指南（2026-10-13 实读）："bioRxiv is a source of technical information and should not be
  used to distribute other kinds of content such as news, **product advertisements**, or policy statements"，
  且全部稿件过 basic screening（offensive / non-scientific content）。上述文本与该定位冲突。
- 参考文献硬错（Crossref 核验）：旧稿 L469 "西瓜花叶病毒 … 2009;161(2):210-213" 实为 Pepino mosaic virus,
  J Virol Methods 162(1-2):46-55（DOI 10.1016/j.jviromet.2009.07.008）；L~ Taylor MIQE 实践文实为
  Methods 2010;50(4):S1-S5（DOI 10.1016/j.ymeth.2010.01.005），旧稿写 2019。
- 结论：旧稿不可投任何渠道，**保留原地、不删除、不再复用**，其内容已被 `paper/` 取代（该决定 2026-06-29 的 spec 已记录）。

### 目标期刊核实（2026-10-13）

- **JOSS**（joss.readthedocs.io + joss.theoj.org，官方文档）：投稿前 4 道 screening 门禁（公开开发史 >6 个月、
  有研究影响力的证据、良好开源实践、迭代式开发）；论文须为 `paper.md`，**750–1750 词**，必需章节
  Summary / Statement of need / State of the field / Software design / Research impact statement /
  AI usage disclosure / Acknowledgements / References；**强制 AI 使用披露**。
- 仓库现状对照：公开 2026-03-09 起、94 次提交、2 个 tag release、CI+tests+文档+CHANGELOG ✅；
  缺 CONTRIBUTING（本轮已补）、GitHub 描述为空、0 issue / 0 PR / 0 fork —— "研究影响力"这道门禁是主要风险。
- **Bioinformatics (OUP) Application Note**：官网 Instructions to Authors 受 Cloudflare 保护（curl 403、
  Wayback 不可达、内置浏览器 http 触发安全验证），**字数/图表上限未能核实**；不绕过安全验证（按规则转人工）。
- 顺带核实：R 包 qPCRtools（Front Genet 2022, DOI 10.3389/fgene.2022.1002704）被引 **32 次**（OpenAlex，2026-10-13），
  FWCI 1.66；单位英文名以 **Yazhouwan National Laboratory** 为主流写法（OpenAlex 780 条 vs "Yazhou Bay National Laboratory" 8 条）。

### 稿件修改（`paper/`，Bioinformatics Application Note 主线）

- `main.tex`：作者改为单作者 Xiang Li（Yazhouwan National Laboratory, Sanya, Hainan, China）；头部补投稿要点。
- `sections/abstract.tex`：重写；**删除 Shapiro–Wilk 的说法**（`src/Core/StatisticalTest.cpp:715` 该检验未实现、
  只 `qWarning` 后返回 NA）；改为陈述界面实际提供的方法。
- `sections/introduction.tex`：加入 Statement of need 段（受众=没有 R 环境的学生/湿实验室人员），
  以 R 包 >30 次被引说明算法口径已被接受。
- `sections/implementation.tex`：① Figure 1 占位符**替换为三张真实界面截图**（V1.1.0 实机截图：导入数据 /
  分析配置 / 结果展示，数值与 `comparison.csv` 一致）；② 配对 t 检验、Wilcoxon 符号秩检验改为
  "核心库实现、测试覆盖，界面暂未提供"；③ Excel 支持改为"链接可选 OpenXLSX 时支持 .xlsx"
  （`.github/workflows/build.yml` 未安装 OpenXLSX，已发布二进制不含该功能）。
- `sections/availability.tex`：写明版本 1.1.0、免注册下载、可选依赖 OpenXLSX。
- 新增 `paper/cover_letter.md`（投稿信草稿）与 `paper/SUBMISSION_CHECKLIST.md`（含 11 条参考文献核验结果与
  7 项待用户确认事项）。
- 新增图件：`paper/figures/fig1a_import_and_preview.jpg`、`fig1b_analysis_configuration.jpg`、
  `fig1c_results.jpg`（源图取自软著操作手册 docx 内嵌的 11 张 1920×1050 实机截图，按内容包围盒裁去底部空白）。

### 稿件新增（JOSS 支线）

- 新增 `paper.md`（1446 词，含全部 JOSS 必需章节，AI 披露段与 Research impact 段留有待用户确认/补充的 `[[ ]]`）、
  `paper.bib`（8 条，全部 Crossref 核过）、`CONTRIBUTING.md`（JOSS 单作者项目要求：贡献流程 + 治理/支持说明）。
- `README.md`：修正 Citation 段的 BibTeX 作者名单（原为错误名单，已改为 Crossref 记录：
  Li, Xiang; Wang, Yingmin; Li, Jingyu; Mei, Xinyue; Liu, Yixiang; Huang, Huichuan）；
  新增 Contributing/support/governance 与论文/验证包索引小节。

### 图片查看登记（按"看图后必须补记录"要求）

- 本轮为挑选论文 Figure 1 面板，目视查看了软著操作手册内嵌的 UI 截图（原图 image1/2/4/5/6/7/8.png 中的
  6 张，及裁剪后的 `paper/figures/fig1a/1c`）。用途仅为**确认截图内容与真实数值一致**（用于论文插图），
  未把任何目视观察当作结果数据写入正文；正文数值仍全部来自 `validation/results/comparison.csv`。

### 待用户处理

见 `paper/SUBMISSION_CHECKLIST.md` 第三节；关键三项：Application Note 字数/图表上限（我未能核实）、
JOSS research impact 段的自用证据、JOSS AI 披露段确认。

## 2026-10-08 20:12 — 生成软件著作权申请资料（进行中）

- 使用 `software-certificate-skill` 为 qPCRtools C++ 生成软件著作权申请资料。
- 完成项目证据扫描与业务理解：识别 7 个真实功能模块（数据导入与预览、标准曲线计算、ΔCt、ΔΔCt、组间统计检验、结果导出、模板下载与多语言界面）。核对源码后确认“图表可视化”在当前版本未启用（`web/js/app.js` 中 `displayCharts` 被注释），已从业务模型中移除，未写入材料。
- 在本机用当前源码重新构建应用（out-of-source：`/tmp/qpcr_build_app`，Qt 6.11.2 + CMake Release，Excel 导入因未安装可选库 OpenXLSX 未启用），并用 Computer Use 实际操作软件取证，取得 11 张真实界面截图（1920×1050），全部通过 skill 的确定性校验（`state=captured`，11/11 pass，0 质量告警）。
- 运行环境与结果文件：运行时目录 `/private/var/folders/t4/w5lxpw4j3lvfq7bs3f2x_wlr0000gn/T/software-certificate-skill/b7687dfe5c16de35f1c3/`；截图收据 `软件著作权申请资料/computer-use-session.json`。
- 待办：著作权人（名称+证件号）与开发完成日期待确认（由用户向导师确认以个人或单位名义申报）；源码清单选择器对含 `build-app/`、`build-tests/`、`build-val/` 与 `.app` 包的项目存在缺陷，需确认修复方式。

## 2026-10-08 20:40 — 修补 software-certificate-skill 源码选择器（经用户批准，方案 A）

- 修补外部技能 `~/.codex/skills/software-certificate-skill/scripts/auto_select_source.py`：① 目录排除改为按形状识别构建产物与平台包（`build-*`/`build_*`/`cmake-build*`、`*.app`/`*.bundle`/`*.framework`/`*.xcodeproj`/`*.xcworkspace`/`*.egg-info`），并新增 `*.min.js`/`*.min.css`/`*.map` 排除；② 修正测试判定（原逻辑把 `src/Core/StatisticalTest.cpp` 误判为测试文件而落选）；③ 为入口/桌面与 Web 界面层/数据层/Web 目录增加合理优先级；④ `--target-lines` 默认 4200 → 9000。
- 效果：源码材料由"选中 GoogleTest 第三方源码与 `build-*` 产物、漏掉 GUI 与前端"修正为 26 个真实应用源文件（入口 `src/main_simple.cpp`、Web 前端、Qt GUI、核心算法、数据层），共 9355 行，无第三方/生成代码。
- 记录与回滚：原始文件 md5 `1f9ba4e4c8eb1dcb139456e8dccdacd4`，逐行差异留档 `/Users/lixiang/.codex/2026-10-08_software-certificate-skill_auto_select_source.patch`；`validate_skill.py` 通过（0 error / 0 warning）。

## 2026-10-08 20:53 — 生成软件著作权申请资料（完成）

- 登记事实：著作权人 = 云南农业大学（法人、统一社会信用代码 `1253000043120331X7`）、开发完成日期 2026-10-01、版本 V1.1.0、单独开发、原创、未发表、原始取得、全部权利。
  - 统一社会信用代码由公开渠道核对：爱企查、启信宝、工商名录（mingluji）以及一个 `.edu.cn` 公示页面（昆明学院科技处公示中写明"云南农业大学（统一社会信用代码：1253000043120331X7）"）一致；另有疑似事业单位登记信息页显示登记机关为云南省事业单位登记管理局、开办资金 9650 万元、有效期至 2026-11-22。**未能直连登记机关原始页面，权威原件仍为《事业单位法人证书》，已提醒用户照证书复核。**
- 同一缺陷的另外两个脚本也一并修补（经用户批准，同一批范围）：`scripts/scan_project.py`（md5 `916c4093…` → `45ab5f7a…`）与 `scripts/analyze_project.py`（md5 `53f1ba06…` → `1b9b3659…`）。两者原来都只排除名为 `build` 的目录，使 googletest 第三方源码与生成代码被计入语言统计：申请表出现 TypeScript（实为 Qt `.ts` 翻译 XML）与 C（实为 CMake 编译器探测文件）。修正后扫描文件数 365→58、源码行数 118845→10842、编程语言由 "C++、C/C++、Python、JavaScript、HTML、C、TypeScript" 修正为 "C++、JavaScript、C/C++、HTML、Python"。
- 操作手册内容质量门禁首轮未过（重复句式比 0.2198 > 0.20）：通过为 7 个功能模块补充互不相同的 `manual_titles` 与 `prerequisites`/`restrictions` 后降至 0.1818，通过。
- 交付结果（`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/软件著作权申请资料/正式资料/`）：申请表信息.txt（29 字段、0 问题）、操作手册 docx+pdf（22 页，19 个内容逻辑页、11 张真实截图）、代码(前30页) docx+pdf（30 页）、代码(后30页) docx+pdf（30 页，每页 50 条有效源码行，末页 7 行）。材料一致性校验 **RELEASE_READY=true，15 项全通过、0 失败**。
- 源码材料：26 个真实应用源文件、8438 行有效代码、完整语料 112 个逻辑页（入口 + Web 前端 + Qt GUI + 核心算法 + 数据层），无第三方/生成代码。

## 2026-10-08 21:08 — 修复操作手册 PDF 中文乱码（重新出正式资料）

- 现象（用户反馈）：`..._操作手册.pdf` 中文大量乱码；代码两卷 PDF 正常。
- 定位：`pdffonts` 显示手册 PDF 只内嵌了 Latin 字体（LiberationSerif / LinuxLibertineG / FrankRuhlHofshi / OpenSymbol），**一枚中文字体都没有**，所以中文全部渲染成方框（文本层可正常提取，故不是编码问题）；代码两卷由技能内置渲染器生成，内嵌 STHeitiTC-Light，正常。
- 根因：DOCX 主题要求 `SimSun`（宋体）与 `Microsoft YaHei`（微软雅黑），macOS 无此两款字体；而 LibreOffice 无头模式走 fontconfig，其自带的 libfontconfig（0x2012002）与 Homebrew fontconfig 生成的缓存（0x2012003）版本不一致，默认配置下拒绝使用缓存并**枚举不到任何系统中文字体**，连显式指定 "PingFang SC"/"Heiti SC"/"Arial Unicode MS" 也全部回退到 LinuxLibertineG。实测在同一台机器上把 fontconfig 配置显式指给 LibreOffice 后即可解析（`SimSun → 宋体-简 Songti SC`、`Microsoft YaHei → 苹方 PingFang SC`）。
- 修复：新增 `/Users/lixiang/.codex/softcert-soffice-zh.sh`（导出 `FONTCONFIG_FILE=/opt/homebrew/etc/fonts/fonts.conf`、`FONTCONFIG_PATH=/opt/homebrew/etc/fonts` 后 exec soffice），并在运行技能时 `export SOFFICE=/Users/lixiang/.codex/softcert-soffice-zh.sh`（技能 `convert_document.py` 会读取 `SOFFICE` 环境变量）。未改动 DOCX 主题字体名，Word/WPS 端仍按宋体/微软雅黑显示。
- 结果：手册 PDF 由 22 页变为 25 页（中文字形度量不同），内嵌 15 个中文字体（STSongti-SC、PingFangSC、HiraginoSans、NotoSerif/NotoSans 等，`uni=yes`）；第 1–6 页非白像素占比 0.9% / 2.2% / 7.0% / 6.6% / 5.8% / 6.0%，无空白页。三份 PDF 全部内嵌中文字体；材料一致性校验仍为 **RELEASE_READY=true，15 项全通过**。
- 后续注意：在本机重出软著资料时需带上 `SOFFICE=/Users/lixiang/.codex/softcert-soffice-zh.sh`，否则手册中文会再次变成方框。
