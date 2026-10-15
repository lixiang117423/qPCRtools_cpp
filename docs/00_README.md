# 仓库目录说明 | Repository layout

> 2026-10-13 整理。这张表回答一个问题：**每一项是干什么的、现在还有没有用**。
> 「状态」列里 **活跃** = 日常会用/要维护；**历史** = 已完成或被取代，只作留档。
>
> 结构总览：**软件本体全在 `code/`**（CMake 工程根），**投稿材料在 `manuscript/`**，
> **软著材料在 `软件著作权申请资料/`**，文档在 `docs/`，历史件在 `archive/`；
> `paper.md` + `paper.bib` 因 JOSS 规定必须留在仓库根。

## 顶层

| 路径 | 是什么 | 状态 | 谁会看它 |
|---|---|---|---|
| `README.md` | 项目总入口：下载、功能、构建、引用方式 | 活跃 | 所有人 |
| `CHANGELOG.md` | 变更与操作记录（时间倒序，含每次整理/改稿/发布） | 活跃 | 你（回溯用） |
| `CONTRIBUTING.md` | 贡献流程、bug 报告要求、决策与发版规则、AI 使用披露要求 | 活跃 | 外部贡献者 |
| `LICENSE` | MIT 许可 | 活跃 | — |
| `code/` | **软件本体**（= CMake 工程根，下列内容都在它下面） | 活跃 | 开发/构建 |
| `docs/` | 全部文档，本文件是索引 | 活跃 | 所有人 |
| `manuscript/` | 论文（Bioinformatics Application Note）：LaTeX 源、图、投稿信、投稿清单 | 活跃 | 投稿 |
| `paper.md`、`paper.bib` | 短文（JOSS 投稿用）。**JOSS 要求放在仓库根目录，不要移动** | 活跃 | 投稿 |
| `archive/` | 已取代/历史文件（旧稿、过时进度文档、废弃占位文件），只作留档，**不要复用** | 历史 | 回溯 |
| `软件著作权申请资料/` | 软著申请正式材料（申请表信息、操作手册、代码前后 30 页） | 已完成 | 交付/存证 |
| `.github/workflows/` | CI 配置（`ci.yml` 测试、`build.yml` 发版）。GitHub 硬性要求放在仓库根，**不要移动** | 活跃 | CI |

## code/ 内部（软件本体）

| 路径 | 是什么 |
|---|---|
| `CMakeLists.txt`、`vcpkg.json` | 构建配置（依赖：Qt6、Eigen3、GSL、可选 OpenXLSX）。**构建命令从仓库根执行：`cmake -S code -B build`** |
| `src/`、`include/` | C++17 源码与头文件。`src/Core/` 是**数值核心**（静态库 `qpcr_core`），GUI 与前端不碰算法 |
| `web/` | 界面（HTML/CSS/JS + ECharts），由 Qt WebEngine 加载；`web/calsc.*` 是标准曲线示例数据 |
| `tests/` | GoogleTest 单元测试 + JS 冒烟测试；黄金基准由 `tests/generate_reference.R` 生成（在 `code/` 下运行） |
| `validation/` | **R 与 C++ 等价性验证包**：`run_R.R`、`run_cpp.cpp`、`compare.py`、`check_example_stats.R`；论文里的等价值、图 2/图 3 与示例数据统计量由它产出（输出到 `../manuscript/figures/`） |
| `examples/` | 示例数据（`cq.csv`、`design.csv`、`rqpcr_*`），软件内"加载示例数据"与测试都用它 |
| `translations/`、`macos/` | Qt 翻译文件；macOS `Info.plist`（CMake 指定使用，**不要移动**） |
| `scripts/` | 构建、打包、上传 Release 的脚本（见 `scripts/README.md`）；构建产物仍在**仓库根**的 `build/` |

## docs/ 内部

| 文件 | 是什么 |
|---|---|
| `00_README.md` | 本索引 |
| `01_quickstart.md` | 本机构建 + 运行（macOS/Linux） |
| `02_quickstart_upload.md` | 把安装包上传到 GitHub Release |
| `03_quickstart_release.md` | 发版上手（打 tag、建 Release） |
| `04_windows_build.md` | Windows 构建完整步骤与常见问题 |
| `05_release_guide.md` | 完整发布指南（含 Release 文案模板） |
| `06_github_actions.md` | GitHub Actions 工作流说明（`ci.yml` 测试、`build.yml` 发版） |
| `07_github_actions_troubleshooting.md` | CI 失败排查 |
| `08_status.md` | **当前状态**：架构、R 函数映射表、2026-06 的修复与优化清单 |
| `superpowers/plans/`、`superpowers/specs/` | 2026-06 写论文时的会话计划与设计 spec（当时用 `paper/` 路径，现已改名为 `manuscript/`） |

## archive/ 内容

| 文件 | 是什么 |
|---|---|
| `manuscript_biorxiv_2026-03.md` / `.docx` | 2026-03 那版投稿稿（含无产出支撑的数据，bioRxiv 未通过筛选）。**不可复用**，说明见 `CHANGELOG.md` 2026-10-13 条目 |
| `PROGRESS_2025-01_obsolete.md` | 2025-01 的开发进度快照，文件自身已注明过时 |
| `legacy_qpcr_tools_en_stub.ts` | 早期空的 Qt 翻译占位文件（CMake 只用 `translations/` 下那两个） |

## 约定

- `docs/` 与 `archive/` 内文件名用 `NN_` 顺序编号；**`code/` 下的目录不编号**——这些名字（`src/`、`web/`、`tests/`、`translations/`、`macos/` 等）被 CMake、CI 与打包脚本写死。
- 软件的构建/测试/验证命令**以 `code/` 为工作目录**（或用 `cmake -S code -B build` 从仓库根调用）；构建产物落在仓库根的 `build/`、`build-val/`、`build-tests/`。
- `docs/superpowers/` 是 2026-06 写论文时的会话计划与 spec，属历史记录，里面的路径（如 `paper/`）是当时的写法，未改写。
- 定位任何东西优先看 `docs/08_status.md`（软件现状）与 `CHANGELOG.md`（最近发生了什么）。
