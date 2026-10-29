# 投稿前清单（Bioinformatics Application Note 主线）

> **2026-10-29 更新：以下第一至六节是 2026-10-19 的历史快照，已不再代表当前状态。**
> 旧版原文另存于 `archive/SUBMISSION_CHECKLIST_20261019_archived.md`。当前状态、阻断项和
> 重新生成命令见文末“七、2026-10-29 当前状态”。

最后更新：2026-10-29 14:42。**官方要求以用户 2026-10-15 从 OUP 页面取得的原文为准**（本机再次抓取 `academic.oup.com/bioinformatics/pages/instructions_to_authors` 仍被 Cloudflare 拦成 "Just a moment..." 页面，无法复核当年指南，故不臆造任何新条款；投稿前若页面可访问，请再对一遍）。

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
| 图 1 建议换成分析结果页截图 | 已换成结果页截图（`figures/fig1_results_page_en_v3.png`，1262×857；上＝汇总表含 p 值与显著性、下＝界面内相对表达量图含**居中的 SD 误差线**）| ✅ 已满足 |
| 软件/数据对非商业用户免费可用 | MIT license，GitHub 公开仓库 + Windows/macOS 预编译二进制 | ✅ |
| Availability and Implementation 必须在正文写明 | 专设一节 `\section{Availability and implementation}`，写明实现方式（C++17/Qt6、CMake、可选依赖）与获取方式（仓库地址、二进制、无需注册） | ✅ |
| 软件须在发表后至少两年可用 | 该节明确写"We will keep the software and its source code available for at least two years after publication"；v1.2.0 已发布 GitHub Release（含 Windows/macOS 安装包）| ✅（用户 2026-10-15 决定：不做 Zenodo DOI，以 GitHub release 为准）|
| 等价性报告表应含统计层（p 值 / Tukey 字母）与独立参照 | Table S1 分两块：(a) 与 R 包的数值等价；(b) 统计层（均值±SD、Welch p、ANOVA p、Tukey 字母）对零依赖 base-R 参照；`code/validation/verify_stats.R` → `STATS-VERIFY PASS` | ✅ 已满足 |
| 补充材料需在摘要中提及 | 摘要末句："Supplementary figures and tables give the full analysis output and the equivalence comparison…" | ✅ |
| 软件名须出现在题目中 | 题名含 "qPCRtools" | ✅ |
| 网络服务不得强制注册 | 桌面软件，下载与使用均无需注册 | ✅ 不适用/满足 |
| 不得描述琐碎工具、安装不得耗时 | 单文件可执行，无需 R/Python/运行时；正文写明 | ✅ |

## 二、稿件状态（2026-10-15）

| 项 | 状态 |
| --- | --- |
| 主文 | `main_v10_20261015.pdf`（**7 页**，`article` 骨架＋A4/2.5 cm 页边距；正文 **1,805 词**＋摘要约 230 词；**1 图 + 1 表** + 17 条参考文献）。Word 版 `main_v10_20261015.docx`（A4、Arial 11 pt、1.5 倍行距、两端对齐、11 页，图注表注带 Figure 1./Table 1. 编号）已生成并渲染核对 |
| 补充材料 | `supplementary_v3_20261019.pdf`（4 页 A4，图注已改为 S1/S2 自动编号，消除原来 "Figure 1: Figure S1." 的双编号）；同源 Word 版 `supplementary_v3_20261019.docx`（2 图 1 表，图已转 400 dpi PNG 嵌入）。⚠️ 本机 Tectonic 的 TeX 包缓存不完整，v3 的 PDF 是由同源 docx 经 LibreOffice 渲染（排版为 Arial/1.5 倍行距，与主文 LaTeX 版观感略有差异）；`supplementary.tex` 已改好，等 TeX 包缓存补齐后重编即可得到 LaTeX 版，两者内容一致 |
| 作者与单位 | Xiang Li（第一作者 + 通讯，单位 1、2，现地址 Yazhouwan National Laboratory）与 Huichuan Huang（共同通讯，单位 1、2）；邮箱 `lixiang117423@gmail.com` / `absklhhc@gmail.com` |
| Figure 1 | 分析结果页截图（`manuscript/figures/fig1_results_page_en_v3.png`）：上＝汇总表、下＝界面内作图，同窗两个滚动位置拼接；原始首页截图 `fig1_home_en.png` 保留未删 |
| 软件版本 | 稿件描述 **1.2.0**；已发布 release <https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.2.0>（Windows ZIP 由 CI 构建；macOS DMG 本地构建并实测可启动、界面正常加载）|
| Cover letter | `cover_letter_20261015.docx`（Word 版，1 页 A4）+ 同名 `.pdf` / `.md`；内容含发表理由、验证方式、公开获取方式与作者声明；已删除占位说明、并删去"两位作者已批准"的表述（黄老师审阅前不下此结论）。Associate Editor 提名仅在投稿系统要求时于系统内填写 |
| `main.docx` | 2026-10-13 归档到 `archive/manuscript_2026-06_word_version.docx`，投稿只用 PDF |
| 历史版本 | `main.pdf`(10-13)、`main_v2..v9_20261015.pdf` 保留不删（**v10 为当前定稿**；v8 为已废弃的中间版）|

## 三、重新生成 PDF 的命令（本机已实测）

```bash
cd /Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic main.tex
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic supplementary.tex
```

Word 版（正文 + 补充材料，改完 tex 后重跑一次）：

```bash
cd /Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp
/Users/lixiang/miniforge3/envs/python/bin/python3 code/scripts/15_make_manuscript_docx.py
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

1. **基金声明确认**：backmatter 保留原基金（2021YFD1000200、31801792、31960554、霍英东基金），与 Frontiers 2022 一致——**仍需黄老师确认是否仍适用**；投稿系统里的 funding 栏要与正文一致。
2. **Associate Editor 提名**：投稿系统通常要求；确认人选后在**系统里**填写（cover letter 不再留占位）。建议从"生物信息学软件/工具"或"qPCR / MIQE 方法学"方向选，并确认无利益冲突。
3. **Zenodo DOI**：用户 2026-10-15 决定**不做**——以 GitHub release（v1.2.0，含安装包）作为存档与引用入口。
4. **OUP 官方模板**：现用 `article` 骨架；若 OUP 提供 Application Note 专用 `.cls`/`.bst`，投稿前替换（`main.tex` 顶部已标注该步骤）。
5. **R 包侧同步**：用户 2026-10-15 决定**先不做**（`CRAN/qPCRtools/` 工作区里的修补保持未提交状态；`CalExpCurve` 反变换一修会牵动 2022 年论文图 2 的重跑，将来要做时再议）。
6. **GUI 端到端取证**：✅ 已完成——1.2.0 实机跑通"导入示例 → 配置 → 运行 → 结果页（含图表、p 值、导出按钮）"，并据此换了图 1；DMG 打包版另行实测"能启动 + WebEngine 正常加载界面"（未在打包版里重复跑整套分析，核心库与 Web 资源与开发版完全同一份）。
7. **JOSS 支线**（`paper.md`）：作者已含 Huichuan Huang，1,484 词符合 JOSS 要求；仍待补 Research impact 段的自用/教学证据与 AI 使用披露确认。

## 六、本轮已解决的历史问题

- **补充材料图注双编号**（2026-10-19）：LaTeX 自动编号 + 图注里手写 "Figure S1." 会排成 "Figure 1: Figure S1."；已改为 `\renewcommand{\thefigure}{S\arabic{figure}}` / `\thetable`，图注里去掉手写前缀。
- **Cover letter 占位与越权声明**（2026-10-19）：删除 `[[Associate Editor nomination …]]` 占位段；删去 "Both authors have approved the submission"（黄老师尚未审阅，不能先写）。
- **Word 版缺失**（2026-10-19）：新增 `code/scripts/15_make_manuscript_docx.py`，一次生成正文与补充材料 docx；pandoc 的 LaTeX reader 会把图注转成图片 alt 文本、且无法显示嵌入的 PDF 图，脚本里已分别处理（补 Figure/Table 编号、PDF 图先转 400 dpi PNG）。

- **作图功能**：原 `displayCharts()` 是写死假数据的桩、结果页连图表容器都没有；2026-10-15 已按真实结果表重写（表达量图 bar + SD 误差线 + 显著性标注；标准曲线图 log₄(浓度) 散点 + 回归线 + 范围外空心点），均可导出 PNG，界面版本升至 1.2.0。
- **界面标签与检验不符**：下拉原本写 "Student's t-test" 而实际调用 Welch 检验；2026-10-15 改为 "t-test (Welch)"，与结果表显示一致。
- **p 值口径**：Results 原写"p < 10⁻³"与实测不符，已改为实测的 0.006 / 0.022，并给出零依赖复算脚本。
- **ΔΔCt 校准**：原按全样本均值归一化（对照组 0.59，与稿件公式矛盾）；已按 Livak & Schmittgen 改为以对照组为校准样品，对照组 ≈ 1（1.01 ± 0.21）。
- **标准曲线横轴**：图 2A 原将点画在 log₁₀ 而回归线按 log₄ 斜率，已统一为 log₄（稀释步数），并把标定范围外的点标为空心。

## 七、2026-10-29 当前状态

| 项目 | 当前文件/事实 | 状态 |
| --- | --- | --- |
| 主文 PDF | `manuscript/main_v18_20261029.pdf`，A4，9 页（含参考文献） | 已编译并通过程序化门禁；4 页限制仍须按 OUP 官方模板复核 |
| 主文 Word | `manuscript/main_v18_20261029.docx`，A4，渲染 11 页 | 已核对标题页、Table 1 不跨页、Figure 1 居中 |
| 补充材料 PDF | `manuscript/supplementary_v9_20261029.pdf`，A4，4 页 | 已编译；图 S1/S2、表 S1/S2/S3 完整 |
| 补充材料 Word | `manuscript/supplementary_v9_20261029.docx`，A4，渲染 5 页 | 已核对两图居中、三张表分页 |
| 软件版本 | 1.3.1；[GitHub Release](https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.3.1) 已含 Windows ZIP 与 macOS DMG | ✅ 资产、签名、版本号与包内 `app.js` 已核验 |
| Figure 1 | `manuscript/figures/fig1_results_page_en_v5_20261029.png`，修复散点与显著性标记跟随柱子 | 已更新 |
| Cover letter | `manuscript/cover_letter_v7_20261029.pdf/.docx/.md`，A4，1 页 | 已生成并渲染核对 |
| 目标期刊要求 | 仅记录用户 2026-10-15 提供的 Application Note 摘要 | **官网当前版未复核（Cloudflare），不能声称已核对最新指南** |
| 投稿包门禁 | `code/scripts/16_check_submission_package.py` | `OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page` |

已解决：

- 题名改为功能导向，不再以“与 R 版等价”为卖点。
- 验证改为对独立 base-R 参照逐项对账；58 个配对值最大相对差 1.9 × 10^-14。
- Welch/Wilcoxon 多组对同一对照按基因内 Holm 校正；原始和校正 P 值均保留。
- ΔΔCt 对照组校准、SE = SD/√n、RqPCR 样本身份去重和逐步 geNorm 均有回归测试。
- Figure 1 改为新版结果页截图：汇总表显示调整后与原始 P 值，图中有单个重复点、
  柱间间隔；散点与显著性标记现在跟随各自柱子，SD 不再作为图例项。
- 标准曲线示例明确标出 Gene1 的 R² = 0.75 不合格；不再把 0.954 当作有效效率证据。
- MIQE 2.0、qpcR、NormqPCR、EasyqpcR 引用已核验并加入；EasyqpcR 明确标为已归档的 Bioconductor 包。
- AI 声明已分别列出 Claude Code 与 OpenAI Codex 的使用阶段。
- `paper.md`、`paper.bib` 与 `docs/superpowers` 已移入 `archive/`，消除平行投稿风险。
- macOS Apple Silicon、macOS 12+、未公证以及无 Linux 发布包的可用性限制已写清。
- Word 标题页不再与作者区分离；Table 1 列宽、补充材料分页和图片居中均已重新渲染核对。
- 可用性声明明确 1.3.1 已发布；R 包 1.0.2 源码包可从 CRAN 获取，
  CRAN 检查页与 Windows/macOS 二进制可能滞后更新。

投稿阻断项 / 待作者处理：

1. **单位邮箱**：两位通讯作者的 institution 邮箱未提供；当前仍用 Gmail。
2. **基金声明**：2021YFD1000200、31801792、31960554 与霍英东基金是否覆盖本项工作仍需确认。
3. **官方模板与页数**：当前使用 `article` 骨架；若 Bioinformatics 提供 Application Note 专用模板，需替换并复核 4 页限制。
4. **Associate Editor**：投稿系统内的人选和利益冲突确认未完成。

已完成：

- CRAN 1.0.2 源码包已于 2026-10-29 发布，CRAN 包页现显示 1.0.2；
  CRAN 检查页与 Windows/macOS 二进制可能滞后更新。
- R 包 GitHub 2.0.1 已发布；公开 issues #1–#5 均已回复并关闭。

2026-10-29 新增或复核的参考文献：

| 文献 | DOI |
| --- | --- |
| Bustin et al. 2025, MIQE 2.0, Clinical Chemistry 71(6):634-651 | 10.1093/clinchem/hvaf043 |
| Ritz & Spiess 2008, qpcR, Bioinformatics 24(13):1549-1551 | 10.1093/bioinformatics/btn227 |
| Perkins et al. 2012, ReadqPCR/NormqPCR, BMC Genomics 13:296 | 10.1186/1471-2164-13-296 |
| Le Pape 2017, EasyqpcR, archived Bioconductor package 3.12 | 10.18129/B9.bioc.EasyqpcR |

投稿包最终门禁：

```bash
/Users/lixiang/miniforge3/envs/python/bin/python3 \
  /Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/scripts/16_check_submission_package.py
```
