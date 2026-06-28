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

## 最近的工作（2026-06）

- 重构：`ExpressionCalculator` 按方法拆成独立 TU（DeltaCt / DeltaDeltaCt / StandardCurve / Internal）
- 重构：前端从 `app.js` 抽出 `templates.js`、`standard_curve.js`，加 jsdom 冒烟网
- 修复：ΔΔCt 归一化、CalExpRqPCR 的 4 处与 R 的偏差、`StatisticalTest` 无 GSL 时的 `std::erfInverse` 编译错误、前端若干 CSS bug
- CI：新增测试 workflow

**版本**：1.0.0-dev
