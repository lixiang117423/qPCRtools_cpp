# qPCRtools C++ — 当前状态（2026-06）

R 包 [qPCRtools](https://github.com/lixiang117423/qPCRtools) 的 C++/Qt6 移植。
GUI 用 Qt WebEngine，前端是 web 技术（HTML/CSS/JS）；核心算法是 C++，通过 WebBridge 暴露给前端。

## 架构

> 2026-10-13 目录整理后，下列路径均在 `code/` 之下（`code/src/Core/` 等）；`code/` 是 CMake 工程根。

- **核心算法** (`code/src/Core/`，静态库 `qpcr_core`)：C++17 + Eigen3，可选 GSL
- **数据处理** (`code/src/Data/`)：DataFrame、CSV、Excel（OpenXLSX，可选）
- **GUI** (`code/src/GUI/`)：WebMainWindow + WebBridge（WebChannel），加载 `web/`
- **前端** (`code/web/`)：jQuery + Bootstrap + ECharts；逻辑拆成 `app.js` / `templates.js` / `standard_curve.js` / `i18n.js`

## R 函数映射

| R 函数 | C++ 实现 | 状态 |
|--------|----------|------|
| `CalCurve()` | `StandardCurve` 类 | ✅ 对齐 R |
| `CalExp2dCt()` | `ExpressionCalculator::calculateByDeltaCt` | ✅ 对齐 R |
| `CalExp2ddCt()` | `calculateByDeltaDeltaCt` | ✅ 对照组校准（对照均值≈1；SE = SD/√n）|
| `CalExpRqPCR()` | `calculateByStandardCurve`（效率法） | ✅ 对齐 R（min.meanCq/参考基因几何均值/最小组均值归一化）；**组内汇总有意偏离** R 的 `mean(unique(expression))`，改为对全部生物学重复求均值（见下方说明）|
| `CalExpCurve()` | —（斜率截距法 `(Cq-Intercept)/Slope`） | ⏳ 未移植 |
| `CalRTable()` / `CalExpRqPCR` 的 GeNorm 自动选参考基因 | `ExpressionCalculator::selectReferenceGenesByGeNorm` | ✅ 已实现（2026-10-15 按 geNorm 原文改正：**相对量 + 逐步剔除**；R 包 `find_ref_gene()` 用原始 Cq 相除，两者在部分数据上会选出不同内参——见下方说明）|

> 注意：`calculateByStandardCurve` 注释曾误标为 `CalExpCurve`，实际实现的是效率法 `CalExpRqPCR`（前端 `standardCurveExp` 走 Eff 列，正是此法）。已在 2026-06 纠正。
>
> geNorm 口径差异（2026-10-15）：R 包 `find_ref_gene()` 的稳定性度量是 `log2(Cq_a / Cq_b)`（原始 Cq 相除），
> 而 geNorm 原文（Vandesompele et al. 2002）要求先换算成相对量 `Q = 2^(minCq − Cq)`。C++ 版按原文实现。
> 在论文示例数据（`examples/rqpcr_*.csv`，6 基因 × 16 样品）上两者选出的内参集合相同（`OsRBBI2` / `OsUBQ`），
> 但构造的判别数据集上会不同（测试 `ExpressionCalculator.GeNormSelectsReferenceGenes` 锁住了这一点）。
>
> RqPCR 的生物学重复汇总（2026-10-15）：R 包 `CalExpRqPCR` 用 `mean(unique(expression))` / `sd(unique(expression))`，
> 会把**数值碰巧相同**的独立生物学重复合并掉（1、1、4 的均值由 2.0 变成 2.5）。原始方法论文
> （Hilliou & Tran 2013, SciTePress, DOI 10.5220/0004312002020211）§2.2.4 只需对生物学重复求均值，
> 全文未出现按数值去重 —— 故 C++ 改为对全部重复求均值（`mean`/`sd` 用 n 个重复，`se = sd/√n`），
> 属**有意偏离**。示例数据上二者最大相对差 ~10%（如 `OsWAK91` 3.6824 → 3.3319），旧口径值保留在
> fixture 的 `by_gene_r_original` 中。
>
> RqPCR 方法的**设计前提**（同一次核查发现，需在文档/论文里写明）：Hilliou & Tran 2013 §2.2.2 规定
> "For each biological replicate i … where the highest expression level set to one"，即**在每个生物学重复内部**
> 跨处理取最小 Cq 做标定。这要求实验是**按生物学重复配对的区组设计**（同一编号的 BioRep 在各处理组里是同一份样品/同一株）；
> 若各组样品相互独立而 BioRep 编号只是各自从 1 开始，这一标定会引入不存在的配对结构。SATQPCR 网页版
> （Rancurel et al. 2019 §3.5）默认用"表达量最低的样品"重标定，并允许用户指定样品——建议后续给 GUI/文档
> 增加同类提示或选项（待用户按实际实验设计确认，见 CHANGELOG 2026-10-15 第④项）。

## 测试

- `code/tests/`：GoogleTest 套件，用与 R 的「黄金参考」对比（fixtures 由 `code/tests/generate_reference.R` 生成）
  - `test_standard_curve.cpp`、`test_statistical_test.cpp`、`test_expression_calculator.cpp`（ΔCt / ΔΔCt / CalExpRqPCR 三套）
- `code/tests/js/smoke.test.js`：jsdom 前端冒烟安全网（重构 `app.js` 用）
- CI：`.github/workflows/ci.yml`（每次 push/PR 构建并跑测试）；`.github/workflows/build.yml` 仅发版（打 tag 出 EXE/DMG）

## 构建

```bash
cmake -S code -B build -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DQt6_DIR=/opt/homebrew/lib/cmake/Qt6 -DEigen3_DIR=/opt/homebrew/share/eigen3/cmake
cmake --build build
# 测试（需联网首次拉取 GoogleTest，或用 -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<本地路径>）
cmake -S code -B build-tests -DBUILD_TESTS=ON [同上 Qt/Eigen] && cmake --build build-tests && ctest --test-dir build-tests
```

> 构建目录仍在仓库根（`build/`、`build-tests/`），源码在 `code/`，所以用 `-S code`。
> macOS 本机一键构建脚本：`bash code/scripts/10_build_local.sh`（同样从仓库根调用）。

依赖：Qt6（Core/Widgets/WebEngine/WebChannel/PrintSupport/LinguistTools）、Eigen3、GSL（强烈建议，否则统计的置信区间走正态近似 fallback）、可选 OpenXLSX。

## 最近的工作（2026-06 修复与优化）

### 修复的 bug（统计正确性）
- `ExpressionCalculator::performTTest`：删除 z 阈值阶梯近似（旧实现 p 值乘 2，纯错），委托 `StatisticalTest::tTest`（Welch，与 R t.test 默认一致）
- `performWilcoxonTest` / `performANOVA`：p 值硬编码 0.05 → 委托真实检验；`performANOVA` 现在输出 Tukey 字母标记
- `StatisticalTest`：无 GSL 时 ANOVA p 值占位 0.05 → 用正则化不完全 Beta（Lentz 连分数）实现精确 t/F 分布与分位数；`incompleteBeta` 旧梯形积分重写
- Wilcoxon 秩和/符号秩：并列值用平均秩 + R 一致方差校正；统计量与 R 一致（U1 / V 约定）
- Wilcoxon 秩和：小样本（n1, n2 ≤ 25）改用**精确分布**（2026-10-15）。原实现只有正态近似，
  3 vs 3 完全分开时给 p ≈ 0.0495（假阳性），精确检验的最小 p 是 0.1。精确 p 按"秩的多重集合"
  做背包 DP，含并列值同样精确（对应 R ≥ 4.2 的默认行为）；更大样本仍走正态近似
  （对齐 R 的 `exact = FALSE, correct = FALSE`）。符号秩（配对）目前仍为正态近似。
- Tukey HSD：改用**学生化极差分布**的真 Tukey（Tukey–Kramer）（2026-10-15）。原实现是
  "t 分布 + Bonferroni 校正"，p 值偏保守且与 Tukey 不是同一个量。现按
  `Q = |Δmean| / sqrt(MSE/2·(1/n_i + 1/n_j))`、`p = P(Q_studentizedRange > Q)` 计算，
  与 R 的 `ptukey(..., lower.tail = FALSE)` / `TukeyHSD()` 同量；R 的 `ptukey` 在极小
  误差自由度处本身有偏差（df = 2 时相对偏差 ~1e-2），本实现以高精度积分与蒙特卡洛为准。
- geNorm 自动选参考基因：改用**相对量 + 逐步剔除**（2026-10-15，导师意见第 7 条）。原实现
  （与 R 包 `find_ref_gene()` 相同）用 `log2(Cq_a / Cq_b)` 即**原始 Cq 相除**衡量稳定性，
  且只算一轮 M 就取最小的两个。现按 geNorm 原文：`Q = 2^(minCq − Cq)`、M = 与其余基因两两
  `log2(Q_a/Q_b)` 的 SD 的平均，**每轮剔除 M 最大者直到剩 2 个**。在论文示例数据上两者
  选出的内参相同（`OsRBBI2` / `OsUBQ`），判别数据集上不同（有专门用例锁住）。
- RqPCR 组内汇总不再按数值去重（2026-10-15，导师意见第 8 条上半）。原实现照搬 R 包的
  `mean(unique(expression))`：两个生物学重复表达量碰巧相同会被合并（1、1、4 → 均值 2.5 而非 2.0）。
  现按样本身份去重（行级完全重复按 `Group|BioRep|TechRep|Gene|Cq` 剔除），组内 `mean`/`sd` 用全部
  生物学重复、`se = sd/√n`。示例数据上最大相对差 ~10%，旧口径值保留在 fixture 里。
- `tTest`/`pairedTTest`/ANOVA/Tukey 的除零与空组边界（se=0、dfWithin≤0、空组）
- `generateLetterGroups`：重写字母分配算法（旧实现在非传递性比较下会给显著差异组分配相同字母）；不再解析 `testName` 字符串（改用 `TestResult::group1Name/group2Name`）
- 标曲表达量法：统计结果的 group1/group2 命名与 PValue 合并循环约定相反 → 结果表 PValue 恒为空，已修复
- ΔΔCt：`removeOutliers` 选项此前完全未生效 → 按 R CalExp2ddCt 语义接线（type-7 分位数）；对照组为空时回退到第一组
- ΔCt：ANOVA 此前静默退化成 t.test → 与 ΔΔCt 一致走 ANOVA+字母标记；原始表 Mean/SD 列错位补丁重写
- `removeOutliers`：Q1/Q3 改用 R type-7 线性插值分位数
- `DataFrame::saveCSV` 与 `WebBridge::writeTableToCSV`：字段转义（逗号/引号/换行）、数字用 'g' 格式（旧实现把小 p 值导出成 0.0000）
- `StandardCurve::calculate`（byMean）：QHash 键序不稳定 → 按浓度升序确定输出
- WebBridge：`getAppVersion()` 硬编码 1.0.0 → CMake 版本宏；`setTableData` JSON null 处理；ΔCt 参数校验
- 前端：`navigateToPage` 缺导航链接时不再抛 TypeError；标曲结果 Excel 导出不再把 CSV 内容存成 .xlsx；基因映射字符串比较改数值；结果表动态显示 SE 列；清理日志噪音

### 重构/清理
- `StatisticalTest`：删除未实现的 `tCDF/tQuantile/fCDF` 声明、`shapiroWilkTest` 不再返回伪造 p 值（如实返回 NA）
- `DataFrame`：`columnType` 用弃用的 `QVariant::Type` → `QMetaType::Type`（消除 Qt6 编译警告）；`join` 列序确定化；删除坏死的 `groupBy` 桩
- WebBridge：删除无用的 `dataframeToVariantMap/variantMapToDataframe`
- ΔΔCt：全表重复 filter → 一次遍历索引；ΔCt：每 gene 重复 filter → 提升到 group 层
- 新增回归测试 `tests/test_bugfixes.cpp`（20 个用例）与 Wilcoxon 并列黄金 fixture（R 4.5 生成）；现有 33/33 测试全绿

**版本**：1.3.1（2026-10-29 补丁发布：修复结果图散点与显著性标记未跟随柱子；
多组对同一对照的 Welch/Wilcoxon P 值按基因内 Holm
校正，结果表同时输出 `RawPValue` / `AdjustedPValue` / `PAdjustMethod`；结果图增加单个
生物学重复点、柱间间隔和校正后显著性标记，并移除 SD 图例项。此前 1.2.0 已恢复作图、
加入分析记录与质控预警。1.3.0 的 Windows/macOS Release 与源码 tag 已发布；
1.3.1 修复散点定位并以同版本 Windows/macOS Release 与源码 tag 发布，
真实数据端到端示例位于 `code/examples/real_workflow/`）
