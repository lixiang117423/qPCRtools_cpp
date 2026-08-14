# qPCRtools C++ — 当前状态（2026-06）

R 包 [qPCRtools](https://github.com/lixiang117423/qPCRtools) 的 C++/Qt6 移植。
GUI 用 Qt WebEngine，前端是 web 技术（HTML/CSS/JS）；核心算法是 C++，通过 WebBridge 暴露给前端。

## 架构

- **核心算法** (`src/Core/`，静态库 `qpcr_core`)：C++17 + Eigen3，可选 GSL
- **数据处理** (`src/Data/`)：DataFrame、CSV、Excel（OpenXLSX，可选）
- **GUI** (`src/GUI/`)：WebMainWindow + WebBridge（WebChannel），加载 `web/`
- **前端** (`web/`)：jQuery + Bootstrap + ECharts；逻辑拆成 `app.js` / `templates.js` / `standard_curve.js` / `i18n.js`

## R 函数映射

| R 函数 | C++ 实现 | 状态 |
|--------|----------|------|
| `CalCurve()` | `StandardCurve` 类 | ✅ 对齐 R |
| `CalExp2dCt()` | `ExpressionCalculator::calculateByDeltaCt` | ✅ 对齐 R |
| `CalExp2ddCt()` | `calculateByDeltaDeltaCt` | ✅ 对齐 R（全数据均值归一化，对照≠1）|
| `CalExpRqPCR()` | `calculateByStandardCurve`（效率法） | ✅ 对齐 R（min.meanCq/参考基因几何均值/最小组均值归一化）|
| `CalExpCurve()` | —（斜率截距法 `(Cq-Intercept)/Slope`） | ⏳ 未移植 |
| `CalRTable()` / `CalExpRqPCR` 的 GeNorm 自动选参考基因 | 部分 | ⏳ |

> 注意：`calculateByStandardCurve` 注释曾误标为 `CalExpCurve`，实际实现的是效率法 `CalExpRqPCR`（前端 `standardCurveExp` 走 Eff 列，正是此法）。已在 2026-06 纠正。

## 测试

- `tests/`：GoogleTest 套件，用与 R 的「黄金参考」对比（fixtures 由 `tests/generate_reference.R` 生成）
  - `test_standard_curve.cpp`、`test_statistical_test.cpp`、`test_expression_calculator.cpp`（ΔCt / ΔΔCt / CalExpRqPCR 三套）
- `tests/js/smoke.test.js`：jsdom 前端冒烟安全网（重构 `app.js` 用）
- CI：`.github/workflows/ci.yml`（每次 push/PR 构建并跑测试）；`.github/workflows/build.yml` 仅发版（打 tag 出 EXE/DMG）

## 构建

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DQt6_DIR=/opt/homebrew/lib/cmake/Qt6 -DEigen3_DIR=/opt/homebrew/share/eigen3/cmake
cmake --build build
# 测试（需联网首次拉取 GoogleTest，或用 -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<本地路径>）
cmake -S . -B build-tests -DBUILD_TESTS=ON [同上 Qt/Eigen] && cmake --build build-tests && ctest --test-dir build-tests
```

依赖：Qt6（Core/Widgets/WebEngine/WebChannel/PrintSupport/LinguistTools）、Eigen3、GSL（强烈建议，否则统计的置信区间走正态近似 fallback）、可选 OpenXLSX。

## 最近的工作（2026-06 修复与优化）

### 修复的 bug（统计正确性）
- `ExpressionCalculator::performTTest`：删除 z 阈值阶梯近似（旧实现 p 值乘 2，纯错），委托 `StatisticalTest::tTest`（Welch，与 R t.test 默认一致）
- `performWilcoxonTest` / `performANOVA`：p 值硬编码 0.05 → 委托真实检验；`performANOVA` 现在输出 Tukey 字母标记
- `StatisticalTest`：无 GSL 时 ANOVA p 值占位 0.05 → 用正则化不完全 Beta（Lentz 连分数）实现精确 t/F 分布与分位数；`incompleteBeta` 旧梯形积分重写
- Wilcoxon 秩和/符号秩：并列值用平均秩 + R 一致方差校正；统计量与 R 一致（U1 / V 约定）
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

**版本**：1.1.0-dev
