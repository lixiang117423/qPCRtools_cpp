# 投稿前清单（Bioinformatics Application Note 主线）

最后更新：2026-10-15 16:00。**官方要求已于 2026-10-15 由用户从 OUP 页面取得**（此前因 Cloudflare 未能核实）。

## 一、官方要求 ↔ 本文档设置（2026-10-15 核实）

官方原文（Bioinformatics, Instructions to Authors → Application Notes）：

> Up to 4 pages; this is approx. 2,600 words or 2,000 words plus one figure.
> … Software or data must be freely available to non-commercial users.
> Availability and Implementation must be clearly stated in the article.
> Authors must also ensure that the software is available for a full two years following publication.
> … Additional supplementary data can be published online-only by the journal.
> This supplementary material should be referred to in the abstract of the Application Note.
> … The name of the application should be included in the title.

| 官方要求 | 本稿设置 | 状态 |
| --- | --- | --- |
| ≤ 4 页；≈2,600 词，或 **2,000 词 + 1 图** | 按更严的"2,000 词 + 1 图"档执行：正文 **1,856 词**（含四段式摘要），主文只留 **1 图 + 1 表** | ✅ 已满足（表格不计入"1 图"，若编辑要求只留 1 个展示项，把 Table 1 移补充材料即可，5 分钟可改）|
| 摘要按 Summary / Availability and Implementation / Contact / Supplementary Information 分段 | 已改为四段式（2026-10-15） | ✅ |
| 使用 AI 编程工具需如实说明 | backmatter 新增 `Use of AI-assisted tools`（OpenAI Codex 用于重构、生成单测与语言审校；结论由作者核验） | ✅ 已加，**请确认措辞** |
| 图 1 建议换成分析结果页截图 | 现仍为英文界面**首页**截图；结果页截图（含图表 + 质控提示 + 导出按钮）需要实机重截 | ⏳ 待做（下一步用 Computer Use 跑一次）|
| 软件/数据对非商业用户免费可用 | MIT license，GitHub 公开仓库 + Windows/macOS 预编译二进制 | ✅ |
| Availability and Implementation 必须在正文写明 | 专设一节 `\section{Availability and implementation}`，写明实现方式（C++17/Qt6、CMake、可选依赖）与获取方式（仓库地址、二进制、无需注册） | ✅ |
| 软件须在发表后至少两年可用 | 该节明确写"We will keep the software and its source code available for at least two years after publication" | ✅（建议另做 Zenodo DOI 存档，见待办）|
| 补充材料需在摘要中提及 | 摘要末句："Supplementary figures and tables give the full analysis output and the equivalence comparison…" | ✅ |
| 软件名须出现在题目中 | 题名含 "qPCRtools" | ✅ |
| 网络服务不得强制注册 | 桌面软件，下载与使用均无需注册 | ✅ 不适用/满足 |
| 不得描述琐碎工具、安装不得耗时 | 单文件可执行，无需 R/Python/运行时；正文写明 | ✅ |

## 二、稿件状态（2026-10-15）

| 项 | 状态 |
| --- | --- |
| 主文 | `main_v6_20261015.pdf`（10 页，`article` 骨架；正文 1,777 词；1 图 + 1 表 + 17 条参考文献）|
| 补充材料 | `supplementary_v1_20261015.pdf`（2 页）：Figure S1（标准曲线 + ΔΔCt 表达量）、Figure S2（等价性散点）、Table S1（等价性汇总）|
| 作者与单位 | Xiang Li（第一作者 + 通讯，单位 1、2，现地址 Yazhouwan National Laboratory）与 Huichuan Huang（共同通讯，单位 1、2）；邮箱 `lixiang117423@gmail.com` / `absklhhc@gmail.com` |
| Figure 1 | 英文界面首页截图（`manuscript/figures/fig1_home_en.png`，实机截取）|
| 软件版本 | 稿件描述 **1.2.0**（较 1.1.0 新增：界面内作图、质控预警、分析记录导出；并修正 ΔΔCt 校准基准、Tukey p 值、geNorm 输入、RqPCR 重复处理等）|
| `main.docx` | 2026-10-13 归档到 `archive/manuscript_2026-06_word_version.docx`，投稿只用 PDF |
| 历史版本 | `main.pdf`(10-13)、`main_v2/v3/v4/v5_20261015.pdf` 保留不删（v6 为当前定稿）|

## 三、重新生成 PDF 的命令（本机已实测）

```bash
cd /Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic main.tex
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic supplementary.tex
```

## 四、参考文献核验（Crossref，最新 2026-10-15）

原 11 条（Bustin 2009/2010、Livak & Schmittgen 2001、Pfaffl 2001、Li et al. 2022、Ahmed & Kim 2018、
Pabinger et al. 2014、Vandesompele et al. 2002、Rancurel et al. 2019、Wickham 2016、Qt/ECharts）全部 ✅。

2026-10-15 新增并逐条核验 ✅：

| 条目 | DOI |
| --- | --- |
| Pfaffl, Horgan & Dempfle 2002, REST, Nucleic Acids Res 30(9):e36 | 10.1093/nar/30.9.e36 |
| Ramakers et al. 2003, Neurosci Lett 339(1):62-66 | 10.1016/S0304-3940(02)01423-4 |
| Ruijter et al. 2009, Nucleic Acids Res 37(6):e45 | 10.1093/nar/gkp045 |
| Hellemans et al. 2007, qBase, Genome Biol 8(2):R19 | 10.1186/gb-2007-8-2-r19 |
| Ballester et al. 2013, DAG Expression, PLoS ONE 8(11):e80385 | 10.1371/journal.pone.0080385 |
| Maussion et al. 2021, Auto-qPCR, Sci Rep 11:21293 | 10.1038/s41598-021-99727-6 |

> 旧 bioRxiv 稿（`archive/manuscript_biorxiv_2026-03.md`）中有一条错误引用（"西瓜花叶病毒…"），该旧稿已弃用，不得复用。

## 五、待你确认 / 处理

1. **基金声明确认**：backmatter 保留原基金（2021YFD1000200、31801792、31960554、霍英东基金），与 Frontiers 2022 一致——请确认是否仍适用。
2. **Associate Editor 提名**：投稿系统通常要求，cover letter 里已留位置。
3. **Zenodo DOI（建议）**：给 1.2.0 的 release 打 DOI 存档，配合"至少两年可用"的承诺更稳；需要你授权我操作 GitHub release（或你手动建）。
4. **OUP 官方模板**：现用 `article` 骨架；若 OUP 提供 Application Note 专用 `.cls`/`.bst`，投稿前替换（`main.tex` 顶部已标注该步骤）。
5. **R 包侧同步**：`CRAN/qPCRtools/` 下已修 ΔΔCt 基准、SE、字母分组 level、geNorm 相对量、RqPCR 去重、`CalExpCurve` 反变换与技术重复层级（**工作区未提交**）。需要你决定是否提交/发版；`CalExpCurve` 一修，**2022 年论文图 2 需重跑**看结论是否变化。
6. **GUI 端到端取证**：作图、分析记录导出、质控面板这三项只做了结构级单测与编译验证，尚未在真实界面里跑一遍截图。
7. **JOSS 支线**（`paper.md`）：作者已含 Huichuan Huang，1,484 词符合 JOSS 要求；仍待补 Research impact 段的自用/教学证据与 AI 使用披露确认。

## 六、本轮已解决的历史问题

- **作图功能**：原 `displayCharts()` 是写死假数据的桩、结果页连图表容器都没有；2026-10-15 已按真实结果表重写（表达量图 bar + SD 误差线 + 显著性标注；标准曲线图 log₄(浓度) 散点 + 回归线 + 范围外空心点），均可导出 PNG，界面版本升至 1.2.0。
- **界面标签与检验不符**：下拉原本写 "Student's t-test" 而实际调用 Welch 检验；2026-10-15 改为 "t-test (Welch)"，与结果表显示一致。
- **p 值口径**：Results 原写"p < 10⁻³"与实测不符，已改为实测的 0.006 / 0.022，并给出零依赖复算脚本。
- **ΔΔCt 校准**：原按全样本均值归一化（对照组 0.59，与稿件公式矛盾）；已按 Livak & Schmittgen 改为以对照组为校准样品，对照组 ≈ 1（1.01 ± 0.21）。
- **标准曲线横轴**：图 2A 原将点画在 log₁₀ 而回归线按 log₄ 斜率，已统一为 log₄（稀释步数），并把标定范围外的点标为空心。
