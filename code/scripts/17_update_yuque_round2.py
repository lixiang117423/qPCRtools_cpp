#!/usr/bin/env python3
"""把 2026-10-29 第二轮返修记录合并到既有语雀模块页。"""

from __future__ import annotations

import sys
from pathlib import Path

import requests

TOKEN_FILE = Path("/Users/lixiang/.codex/secrets/yuque_token.txt")
BACKUP_DIR = Path("/Users/lixiang/tmp/qpcr_yuque_backup_20261029")
API = "https://www.yuque.com/api/v2"
NAMESPACE = "lixiang117423/glg12g"

PAGES = {
    "core": 288253631,
    "paper": 288253647,
    "overview": 288250009,
}


def replace_section(
    body: str, heading: str, next_heading: str, replacement: str
) -> str:
    start = body.find(heading)
    end = body.find(next_heading, start + len(heading))
    if start < 0 or end < 0:
        raise ValueError(f"section not found: {heading!r} -> {next_heading!r}")
    return body[:start] + replacement.rstrip() + "\n\n" + body[end:]


def replace_tail(body: str, heading: str, replacement: str) -> str:
    start = body.find(heading)
    if start < 0:
        raise ValueError(f"tail heading not found: {heading!r}")
    return body[:start] + replacement.rstrip() + "\n"


def replace_once(body: str, old: str, new: str) -> str:
    if body.count(old) != 1:
        raise ValueError(f"expected exactly one match, got {body.count(old)}: {old[:80]!r}")
    return body.replace(old, new, 1)


def insert_version(body: str, entry: str) -> str:
    marker = "## 版本记录（倒序，最新在上）\n"
    if body.count(marker) != 1:
        raise ValueError("version marker is missing or duplicated")
    return body.replace(marker, marker + "\n" + entry.rstrip() + "\n", 1)


def update_core(body: str) -> str:
    body = replace_once(
        body,
        "**状态**：✅完成　**上游**：01 项目与软件架构（`qpcr_core` 静态库分层）　**下游**：05 论文与投稿准备（Results 与 Figure 3、Table 1 直接取自本模块产物）",
        "**状态**：✅ 当前验证通过　**上游**：01 项目与软件架构（`qpcr_core` 静态库分层）　**下游**：05 论文与投稿准备（Supplementary Figure S2、Table S1/S2 与 Results 直接取自本模块产物）",
    )
    purpose = """## 1. 目的

用不加载 qPCRtools R 包的独立 base-R 实现复核 C++ 核心算法。验证不只比对最终表达量，
还覆盖标准差、标准误、原始/校正 P 值与标准曲线参数，并把全部脚本留在仓库中可重复运行。
"""
    body = replace_section(body, "## 1. 目的", "## 2. 输入", purpose)

    current = """## ⭐ 当前有效版本：v1.2.0（2026-10-29 独立 base-R 对账 + Holm 校正）

- 验证范围：ΔΔCt、标准曲线、效率法 RqPCR 三个定量方法；当前参照由
  `code/validation/reference_base_r.R` 直接从原始 Cq 表计算，**不加载 qPCRtools R 包**。
- 关键结果：58 个配对值全部通过，最大相对差 **1.9×10⁻¹⁴**；分方法最大相对差为
  ΔΔCt 1.7×10⁻¹⁴、标准曲线 4.2×10⁻¹⁵、RqPCR 1.9×10⁻¹⁴。
- 示例值：三组相对表达量 1.0145 ± 0.2098、1.9169 ± 0.2060、
  2.7403 ± 0.5580；Welch 原始 P = 0.006027 / 0.022191，基因内 Holm 后为
  0.012055 / 0.022191；ANOVA P = 0.0034471，Tukey 字母 b / ab / a。
- 统计口径：多组对同一对照的 Welch/Wilcoxon 按基因内 Holm 校正，原始与校正 P 值均保留；
  ANOVA 仍用 Tukey HSD。主分析在线性相对表达量尺度，ΔCt 尺度作为敏感性分析（Table S2）。
- 对应论文图表：Supplementary Figure S2、Table S1、Table S2；Results
  “Validation against an independent reference”。
- 路径：本地 `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/validation/`；
  结果在 `code/validation/results/reference_comparison.csv`。超算无。
- 边界：数值一致只证明实现符合指定算法，不等于生物学正确；RqPCR 的 `Cq,min`
  在每个 biological replicate 内跨处理取得，因此要求按重复配对的区组设计。
"""
    body = replace_section(body, "## ⭐ 当前有效版本", "## 1. 目的", current)

    inputs = """## 2. 输入

| 数据 | 来源（模块 + 版本） | 本地路径 |
| --- | --- | --- |
| ΔΔCt 示例数据 | 01 项目与软件架构（仓库示例数据） | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/examples/cq.csv`、`design.csv` |
| 效率法（RqPCR）示例数据 | 同上 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/examples/rqpcr_cq.csv`、`rqpcr_design.csv` |
| 标准曲线稀释系列 | 01 项目与软件架构（前端示例页） | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/web/calsc.cq.txt`、`calsc.info.txt` |
| 当前独立参照 | 本模块 v1.2.0，不加载 R 包 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/validation/reference_base_r.R` |
| 严格对账 | 本模块 v1.2.0 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/validation/compare_to_reference.R` |
"""
    body = replace_section(body, "## 2. 输入", "## 3. 方法与软件", inputs)

    methods = """## 3. 方法与软件

| 软件 | 版本 | 用途 |
| --- | --- | --- |
| R | 4.6.1（`/opt/homebrew/bin/Rscript`） | 运行独立 base-R 参照 |
| C++ / CMake | C++17；CMake 4.4.4 | 编译 `run_cpp`，链接 `qpcr_core` |
| Python | 3.13.2（`/Users/lixiang/miniforge3/envs/python/bin/python3`） | 对账与投稿包检查 |
| GoogleTest | 随项目构建 | C++ 回归测试，当前 53/53 |
| Node + jsdom | 当前本机环境 | 前端 smoke test |

**当前验证包（`code/validation/`）**

| 文件 | 作用 |
| --- | --- |
| `reference_base_r.R` | 不加载 qPCRtools R 包，从原始 Cq 表计算三方法及统计量 |
| `run_cpp.cpp` | C++ 侧输出三方法、原始 P、Holm P、ANOVA/Tukey |
| `compare_to_reference.R` | 58 个配对值逐项对账，失败时非零退出 |
| `plot_examples.py` | 生成 Figure S1 |
| `compare.py` | 生成 Figure S2 与 comparison CSV |
| `README.md` | 范围、复现命令与预期结果 |
"""
    body = replace_section(body, "## 3. 方法与软件", "## 4. 运行", methods)

    run_section = """## 4. 运行（当前 v1.2.0）

```bash
cd "/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code"

/opt/homebrew/bin/Rscript validation/reference_base_r.R
/Users/lixiang/tmp/qpcr_round2_build/run_cpp round2
/opt/homebrew/bin/Rscript validation/compare_to_reference.R

ctest --test-dir /Users/lixiang/tmp/qpcr_round2_build --output-on-failure
cd tests/js && node smoke.test.js
```

验证输出：`REFERENCE-COMPARE PASS`；`53/53` C++ 测试通过；`SMOKE PASS`。
"""
    body = replace_section(body, "## 4. 运行", "## 5. 结果", run_section)

    results = """## 5. 结果

**当前独立参照对账（v1.2.0）**

| 方法 | 对比量 | 最大相对差 |
| --- | --- | --- |
| ΔΔCt | 表达量、SD、原始/校正 P | 1.7×10⁻¹⁴ |
| 标准曲线 | 斜率、截距、R²、效率 | 4.2×10⁻¹⁵ |
| RqPCR | 表达量、SD、SE、原始/校正 P | 1.9×10⁻¹⁴ |

**示例数据（`ΔΔCt`，3 组 × 3 生物学重复）**

| 分组 | 均值 ± SD | Welch 原始 P | Holm P | 显著性 |
| --- | --- | --- | --- | --- |
| 0（对照） | 1.0145 ± 0.2098 | — | — | 校准组 |
| 0.5 | 1.9169 ± 0.2060 | 0.006027 | 0.012055 | `*` |
| 1 | 2.7403 ± 0.5580 | 0.022191 | 0.022191 | `*` |

ANOVA：P = 0.0034471；Tukey 字母 b / ab / a；0 vs 0.5 的
`P_adj = 0.052`，0 vs 1 的 `P_adj = 0.0028`。

**标准曲线**：Gene1 R² = 0.7509、E = 0.9540，质量不合格；Gene2/3/4
R² = 0.9991 / 0.9983 / 0.9993，E = 0.9394 / 0.8549 / 0.8731。

> 当前 Figure S2 与 Table S1/S2 见稿件补充材料。数值一致只说明实现符合指定算法，
> 不说明生物学正确性。
"""
    body = replace_section(body, "## 5. 结果", "## 版本记录", results)

    entry = """### 2026-10-29 08:52｜独立 base-R 对账替代“与 R 包等价”主张，加入 Holm 校正

**变更原因**：第二轮审稿要求验证不依赖被移植对象本身，并明确多组比较的校正方法。
**与上一版差异**：当前有效验证从 R 包对 R 包，改为不加载 R 包的独立 base-R 参照；
Welch/Wilcoxon 多组对同一对照按基因内 Holm 校正；ANOVA 继续用 Tukey HSD。

**结果**：
- 58 个配对值全部通过，最大相对差 1.9×10⁻¹⁴。
- ΔΔCt、标准曲线、RqPCR 分方法最大相对差分别为 1.7×10⁻¹⁴、4.2×10⁻¹⁵、1.9×10⁻¹⁴。
- 示例 dose 0.5 的原始/校正 P 为 0.006027/0.012055，显著性由 `**` 变为 `*`；
  dose 1 为 0.022191/0.022191。
- C++ 53/53、JS smoke、独立对账全部通过。

**命令**：
```bash
cd "/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code"
/opt/homebrew/bin/Rscript validation/reference_base_r.R
/Users/lixiang/tmp/qpcr_round2_build/run_cpp round2
/opt/homebrew/bin/Rscript validation/compare_to_reference.R
```

**输出路径**：本地
`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/validation/results/`
（证据主表 `reference_comparison.csv`）　超算无
"""
    body = insert_version(body, entry)
    notes = """## 6. 问题与备注

- 当前验证不覆盖未移植的 `CalExpCurve` 与 `CalRTable`，论文已明确这一范围边界。
- Welch/Wilcoxon 的 Holm 校正在基因内进行，不平跨基因；ANOVA 使用 Tukey HSD。
- RqPCR 的 `Cq,min` 按 biological replicate 跨处理取得，要求按重复配对的区组设计。
- 数值一致只证明实现符合指定算法，不证明生物学正确性。
- 1.3.0 的公开 release 与匹配二进制仍未上传，是投稿阻断项。
"""
    return replace_tail(body, "## 6. 问题与备注", notes)


def update_paper(body: str) -> str:
    body = replace_once(
        body,
        "**状态**：🔄 进行中（主线定稿待投；支线两处 `[[ ]]` 待补）　**上游**：02 核心算法与 R 等价性验证、04 构建打包与发布　**下游**：97 论文图表对照",
        "**状态**：🔄 作者审阅（第二轮返修包完成；公开 release、R 包修补与作者确认未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照",
    )
    purpose = """## 1. 目的

把 qPCRtools 桌面版写成 Bioinformatics (OUP) Application Note，并保持正文数字、图件、
软件版本、可用性和作者声明均可追溯。JOSS 支线已归档，不再平行投稿。
"""
    body = replace_section(body, "## 1. 目的", "## 2. 输入", purpose)

    current = """## ⭐ 当前有效版本：v6（2026-10-29 第二轮审稿返修）

- 目标期刊：**Bioinformatics (OUP) Application Note**；题目为
  “qPCRtools: a cross-platform desktop application for reproducible qPCR
  analysis without programming”。
- 主文：`main_v15_20261029.pdf`（A4，8 页）与 `main_v15_20261029.docx`
  （A4，渲染 10 页）；1 个主图、1 个主表。
- 补充材料：`supplementary_v8_20261029.pdf`（3 页）与
  `supplementary_v8_20261029.docx`（渲染 4 页）；Figure S1/S2、Table S1/S2。
- Cover letter：`cover_letter_v5_20261029.pdf/.docx/.md`，A4 1 页。
- 关键证据：独立 base-R 对账 58 个值最大相对差 1.9×10⁻¹⁴；C++ 53/53；
  JS `SMOKE PASS`；投稿包门禁 `OK`。
- 路径：本地 `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/`；超算无。
- 当前不能投稿：1.3.0 公开 release、R 包修补、真实端到端数据、单位邮箱、基金、
  OUP 模板/4 页限制、AE 与两位作者最终批准尚未完成。
"""
    body = replace_section(body, "## ⭐ 当前有效版本", "## 1. 目的", current)

    inputs = """## 2. 输入

| 数据 | 来源（模块 + 版本） | 本地路径 |
| --- | --- | --- |
| 独立数值验证 | 02 核心算法与 R 等价性验证（v1.2.0） | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/validation/results/reference_comparison.csv` |
| 主图 Figure 1 | 1.3.0 重建版实机结果页 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/figures/fig1_results_page_en_v4_20261029.png` |
| 补充图 Figure S1/S2 | 02 v1.2.0 验证输出 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/figures/fig2_results_v2.pdf`、`fig3_equivalence.pdf` |
| 参考文献库 | Crossref/Bioconductor 核验 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/refs.bib` |
| 第二轮审稿意见 | 用户提供（会话附件；未归档到仓库） | — |
| 已归档 JOSS 旧稿 | 不再平行投稿 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/archive/paper_joss_20261013_archived.md` |
"""
    body = replace_section(body, "## 2. 输入", "## 3. 方法与软件", inputs)

    methods = """## 3. 方法与软件

| 软件 / 服务 | 版本 | 用途 |
| --- | --- | --- |
| Tectonic | 0.17.0 | 编译主文与补充材料 PDF |
| pandoc | 3.12.1 | LaTeX → Word |
| python-docx | 当前 Python 3.13.2 环境 | Word 标题、表格、图片和页边距 |
| LibreOffice | 26.8.1.1 | DOCX 渲染验收 |
| R | 4.6.1 | 独立数值复算 |
| Crossref / Bioconductor | 2026-10-29 实查 | 文献和 EasyqpcR 包核验 |

**当前生成与门禁命令**

```bash
cd "/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript"
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic main.tex
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic supplementary.tex

cd "/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp"
/Users/lixiang/miniforge3/envs/python/bin/python3 code/scripts/14_make_cover_letter.py
/Users/lixiang/miniforge3/envs/python/bin/python3 code/scripts/15_make_manuscript_docx.py
/Users/lixiang/miniforge3/envs/python/bin/python3 code/scripts/16_check_submission_package.py
```
"""
    body = replace_section(body, "## 3. 方法与软件", "## 4. 结果", methods)

    results = """## 4. 结果

| 检查项 | 结果 |
| --- | --- |
| 主文 LaTeX PDF | A4，8 页 |
| 主文 Word | A4，渲染 10 页；标题与作者同页，Table 1 不跨页 |
| 补充材料 LaTeX PDF | A4，3 页 |
| 补充材料 Word | A4，渲染 4 页；Figure S1/S2 均居中，Table S1/S2 不跨页 |
| Cover letter | A4，1 页 |
| 独立数值对账 | 58 个值，最大相对差 1.9×10⁻¹⁴，PASS |
| C++ / JS | 53/53；`SMOKE PASS` |
| 占位符 / Markdown 残留 | 无；补充材料中的 `**` 是显著性符号 |
| 版本声明 | 1.3.0 明确为投稿前待发布；1.2.0 为现有公开二进制 |

**当前主图**

> **【待贴图】** Figure 1 1.3.0 重建版结果页

**Figure 1** 目标基因 `fos-glo-myc` / `Beta Actin`，3 组 × 3 生物学重复；
汇总表显示原始/校正 P 值，图中显示生物重复点与 Holm 校正显著性。
> 来源：`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/figures/fig1_results_page_en_v4_20261029.png`
> 版本：1.3.0　日期：2026-10-29

**文献新增核验**

| 文献 | 标识 | 状态 |
| --- | --- | --- |
| Bustin et al. 2025, MIQE 2.0 | DOI `10.1093/clinchem/hvaf043`，PMID `40272429` | ✅ |
| Ritz & Spiess 2008, qpcR | DOI `10.1093/bioinformatics/btn227` | ✅ |
| Perkins et al. 2012, NormqPCR | DOI `10.1186/1471-2164-13-296` | ✅ |
| Le Pape 2017, EasyqpcR（Bioconductor 3.12 归档包） | DOI `10.18129/B9.bioc.EasyqpcR` | ✅ |
"""
    body = replace_section(body, "## 4. 结果", "## 版本记录", results)

    pending = """## 5. 问题与备注（待用户确认 / 处理）

1. **1.3.0 公开 release 未完成**：必须上传匹配源码、Windows/macOS 包并回填 release 链接。
2. **R 包侧未闭环**：1.0.2 / 2.0.1 发布、issue 回复及 2022 年论文影响评估仍待处理。
3. **真实端到端示例不足**：当前标准曲线与 ΔΔCt 仍使用两套无关联示例数据。
4. **作者信息**：两位通讯作者的单位邮箱未提供；基金覆盖范围需确认。
5. **期刊格式**：OUP 官方 Application Note 模板和 4 页限制尚未取得；不能用当前 8 页
   `article` 骨架声称已满足。
6. **投稿系统**：Associate Editor 人选与利益冲突声明未定；两位作者尚未完成最终批准。
7. **图片**：语雀 API 无图片上传端点，Figure 1、S1、S2 仍需用户人工拖入对应位置。
"""
    body = replace_tail(body, "## 5. 问题与备注", pending)

    entry = """### 2026-10-29 08:52｜第二轮审稿返修：独立验证、Holm、1.3.0 与投稿包重建

**变更原因**：用户提供第二轮审稿意见，要求继续修改并完善。
**与上一版差异**：中心主张由“与 R 包等价”改为独立 base-R 对账；加入 Holm 校正；
重拍 1.3.0 Figure 1；补充 EasyqpcR 等文献；重写平台范围、RqPCR 前提和标曲质量说明；
主文、补充材料、Word、Cover letter 统一重出。

**结果**：
- `main_v15_20261029.pdf` 8 页；Word 10 页。
- `supplementary_v8_20261029.pdf` 3 页；Word 4 页。
- `cover_letter_v5_20261029.pdf` 1 页。
- `REFERENCE-COMPARE PASS`、C++ 53/53、JS `SMOKE PASS`。
- `16_check_submission_package.py` 返回
  `OK: LaTeX 8+3 pages; Word 10+4 pages; cover 1 page`。

**文件**：
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/main_v15_20261029.pdf`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/main_v15_20261029.docx`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/supplementary_v8_20261029.pdf`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/supplementary_v8_20261029.docx`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/cover_letter_v5_20261029.pdf`

**状态**：可交作者审阅；不能投稿，阻断项见本节问题与备注。
"""
    return insert_version(body, entry)


def update_overview(body: str) -> str:
    body = replace_once(
        body,
        "结果必须与原 R 实现数值等价，且这份等价性可由审稿人用仓库里的开放脚本复算",
        "结果须由不加载原 R 包的独立 base-R 参照逐项复核，并可由审稿人用仓库脚本复算",
    )
    flow = """## 分析主线（模块流程）
1. **01 项目与软件架构**：R 包 → C++/Qt6 移植；数值核心独立成静态库 `qpcr_core`。
2. **02 核心算法与独立数值验证**：当前 v1.2.0 用不加载 R 包的 base-R 参照覆盖三方法，
   58 个配对值最大相对差 1.9×10⁻¹⁴。
3. **03 统计检验实现与测试**：Welch/Wilcoxon/ANOVA/Tukey 与基因内 Holm；53 项 C++ 测试
   和 JS smoke test 通过。
4. **04 构建、打包与发布**：公开 release 为 1.2.0；源码工作树为 1.3.0，匹配包未发布。
5. **05 论文与投稿准备**：第二轮返修包 v6；主文 8 页（Word 10 页）、补充材料 3 页
   （Word 4 页）、Cover letter 1 页；投稿阻断项见本页待办。
6. **06 软件著作权申请材料**：2026-10-08 完成，版本 V1.1.0；后续版本变化是否重出待用户决定。
"""
    body = replace_section(body, "## 分析主线（模块流程）", "## 模块索引表", flow)

    replacements = {
        "| 代码仓库 | 107 次提交（2026-03-09 首次提交；最近一次提交 2026-08-14） | GitHub 公开仓库，MIT 许可 | `https://github.com/lixiang117423/qPCRtools_cpp` |":
        "| 代码仓库 | 工作树 1.3.0（未提交）；公开 release 仍为 1.2.0 | GitHub 公开仓库，MIT 许可 | `https://github.com/lixiang117423/qPCRtools_cpp` |",
        "| 02 核心算法与 R 等价性验证 | ✅ | v1.1.0 | `git log` 8 次提交 + `CHANGELOG.md` 1 条 | 2026-10-13 19:44 |":
        "| 02 核心算法与 R 等价性验证 | ✅ | v1.2.0（独立 base-R + Holm） | `reference_comparison.csv` + `CHANGELOG.md` | 2026-10-29 08:52 |",
        "| 03 统计检验实现与测试 | ✅ | v1.1.0 | `git log` 7 次提交 + `docs/08_status.md` | 2026-08-14 |":
        "| 03 统计检验实现与测试 | ✅ | v1.3.0（53 项 C++ 测试） | `ctest` + `SMOKE PASS` | 2026-10-29 08:52 |",
        "| 04 构建、打包与发布 | ✅ | v1.1.0（2026-06-28 发版） | `git log` 6 次提交 + `CHANGELOG.md` 1 条 | 2026-10-13 20:00 |":
        "| 04 构建、打包与发布 | 🔄 | 1.2.0 已发布；1.3.0 未发布 | `docs/08_status.md` + 投稿清单 | 2026-10-29 08:52 |",
        "| 05 论文与投稿准备 | 🔄 进行中 | v4（2026-10-13 定稿） | `CHANGELOG.md` 5 条 | 2026-10-13 20:16 |":
        "| 05 论文与投稿准备 | 🔄 作者审阅 | v6（2026-10-29 第二轮返修） | 稿件包 + `SUBMISSION_CHECKLIST.md` | 2026-10-29 08:52 |",
        "| 发版 tag | v0.0.1 / v0.1.1 / v0.1.2 / v1.0.0 / v1.1.0 | 其中 **2 个带 GitHub Release**：v1.0.0（2026-03-26）、v1.1.0（2026-06-28） | 同上 Release 页 |":
        "| 发版 tag | v0.0.1 / v0.1.1 / v0.1.2 / v1.0.0 / v1.1.0 / v1.2.0 | 最新公开 release 为 v1.2.0；1.3.0 仍未发布 | 同上 Release 页 |",
        "| 发布二进制 | v1.1.0：`qPCRtools-windows.zip`、`qPCRtools-macos.dmg` | 免安装、免注册 | `https://github.com/lixiang117423/qPCRtools_cpp/releases` |":
        "| 发布二进制 | v1.2.0 | 免安装、免注册；1.3.0 匹配包待发布 | `https://github.com/lixiang117423/qPCRtools_cpp/releases` |",
        "| 稿件（主线） | `main.pdf` 10 页 / 3 个展示项 / 1 张表 / 11 条参考文献 | 目标刊 Bioinformatics (OUP) Application Note | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/` |":
        "| 稿件（主线） | `main_v15_20261029.pdf` 8 页 / 1 图 1 表；`supplementary_v8` 3 页 | 目标刊 Bioinformatics (OUP) Application Note；正式模板待替换 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/` |",
        "| 短文（JOSS 支线） | `paper.md` 1,484 词 | JOSS 要求 750–1750 词；两处 `[[ ]]` 待补 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/paper.md` |":
        "| 短文（JOSS 支线） | 已归档 | 不再平行投稿 | `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/archive/paper_joss_20261013_archived.md` |",
    }
    for old, new in replacements.items():
        body = replace_once(body, old, new)

    conclusions = """## 关键结论
+ **当前验证不再依赖被移植对象本身**：`reference_base_r.R` 不加载 qPCRtools R 包，
  直接从原始 Cq 表复算三个定量方法；58 个配对值最大相对差 1.9×10⁻¹⁴（详见 02 页）。
+ **统计口径已明确**：多组对同一对照的 Welch/Wilcoxon 按基因内 Holm 校正，原始与校正 P 值
  均保留；ANOVA 用 Tukey HSD，跨基因未校正并作为局限写明。
+ **示例数据**：ΔΔCt 三组均值 1.0145 / 1.9169 / 2.7403；Welch 原始 P
  0.006027 / 0.022191，Holm 后 0.012055 / 0.022191；ANOVA P 0.0034471；
  Tukey 字母 b / ab / a。
+ **软件与稿件状态**：源码 1.3.0、53 项 C++ 测试与 JS smoke 通过；公开 release 仍为 1.2.0，
  1.3.0 发布是投稿阻断项。主文 `main_v15_20261029.pdf` 8 页，Word 10 页；
  补充材料 3 页 / Word 4 页。
+ **不能投稿**：OUP 官方模板与 4 页限制、真实端到端数据、单位邮箱、基金、AE、
  R 包修补和两位作者最终批准均未完成。
"""
    body = replace_section(body, "## 关键结论", "## 待办与待确认", conclusions)

    pending = """## 待办与待确认
+ **1.3.0 发布**：上传源码与匹配 Windows/macOS 包，打 tag 并回填稿件链接。
+ **R 包侧**：1.0.2 / 2.0.1 发布、issue 回复及 2022 年论文影响评估未完成。
+ **真实数据**：需用一套真实实验数据完成“标曲 → 方法选择 → 组间分析 → 导出记录”。
+ **作者与声明**：单位通讯邮箱、基金覆盖范围、Associate Editor 和两位作者最终批准待确认。
+ **期刊格式**：官方 Application Note 模板与 4 页限制尚未取得，不能把当前 8 页
  `article` 骨架写成已满足。
+ **语雀图片**：API 无上传端点，Figure 1、S1、S2 仍由用户人工拖入。
"""
    body = replace_section(body, "## 待办与待确认", "## 子页面", pending)
    return body


def merge(kind: str, body: str) -> str:
    if kind == "core":
        return update_core(body)
    if kind == "paper":
        return update_paper(body)
    if kind == "overview":
        return update_overview(body)
    raise ValueError(kind)


def main() -> None:
    token = TOKEN_FILE.read_text(encoding="utf-8").strip()
    headers = {"X-Auth-Token": token, "Content-Type": "application/json"}
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)

    docs: dict[str, dict] = {}
    for kind, doc_id in PAGES.items():
        response = requests.get(
            f"{API}/repos/{NAMESPACE}/docs/{doc_id}", headers=headers, timeout=30
        )
        response.raise_for_status()
        data = response.json()["data"]
        body = data.get("body") or ""
        backup = BACKUP_DIR / f"{kind}_{doc_id}_before_update.md"
        if backup.exists() and backup.read_text(encoding="utf-8") != body:
            backup = BACKUP_DIR / f"{kind}_{doc_id}_before_update_2.md"
        if not backup.exists():
            backup.write_text(body, encoding="utf-8")
        merged = merge(kind, body)
        if kind == "overview":
            if len(merged) < 3000:
                raise SystemExit("overview merge unexpectedly lost content")
        else:
            old_versions = body.count("\n### ")
            new_versions = merged.count("\n### ")
            if new_versions != old_versions + 1:
                raise SystemExit(
                    f"version history check failed for {kind}: "
                    f"{old_versions} -> {new_versions}"
                )
        docs[kind] = {
            "id": doc_id,
            "title": data["title"],
            "slug": data.get("slug"),
            "body": merged,
            "backup": backup,
        }

    for kind, data in docs.items():
        payload = {
            "title": data["title"],
            "slug": data["slug"],
            "body": data["body"],
            "format": "markdown",
            "public": 0,
        }
        response = requests.put(
            f"{API}/repos/{NAMESPACE}/docs/{data['id']}",
            headers=headers,
            json=payload,
            timeout=60,
        )
        response.raise_for_status()
        verify = requests.get(
            f"{API}/repos/{NAMESPACE}/docs/{data['id']}",
            headers=headers,
            timeout=30,
        )
        verify.raise_for_status()
        written = verify.json()["data"].get("body") or ""
        if "2026-10-29" not in written or data["body"][:200] != written[:200]:
            raise SystemExit(f"read-back verification failed for {kind}")
        print(
            f"updated: {kind} id={data['id']} title={data['title']} "
            f"chars={len(written)} backup={data['backup']}"
        )


if __name__ == "__main__":
    main()
