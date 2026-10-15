# 投稿前清单（Bioinformatics Application Note 主线）

生成时间：2026-10-13。**已核实**项给出核实方式与来源；**待你确认**项不要跳过。

## 一、稿件状态（2026-10-13 已完成的修改）

| 项 | 状态 |
| --- | --- |
| 作者与单位 | 2026-10-13 更新：Xiang Li（第一作者 + 通讯，单位 1、2，现地址 Yazhouwan National Laboratory）与 Huichuan Huang（共同通讯，单位 1、2）；通讯邮箱 `lixiang117423@gmail.com` / `absklhhc@gmail.com` |
| Figure 1 | 用英文界面首页截图（`manuscript/figures/fig1_home_en.png`，2560×1656，取自软件实机）——即原先嵌在 `main.docx` 里的那张，现已直接接入 LaTeX 并编译进 `main.pdf` |
| Shapiro–Wilk | 已从摘要与 Implementation 删除（代码里该检验未实现，只返回 NA） |
| Excel 导入 | 措辞已收紧为"构建时链接可选 OpenXLSX 时支持 .xlsx"（已发布的 CI 二进制未启用该组件） |
| 配对 t 检验 / 符号秩检验 | 措辞已改为"核心库实现、测试覆盖，但界面暂未提供"，不再暗示界面可用 |
| 参考文献 | 11 条已逐条 Crossref 核验（见下） |
| `main.docx` | 2026-10-13 归档到 `archive/manuscript_2026-06_word_version.docx`（避免与 `main.pdf` 混淆）；投稿只用 `main.pdf` |

## 一之二、重新生成 main.pdf 的命令（本机能跑，已验证）

本机没有 TeX Live，`tectonic` 默认要联网抓 bundle；但它可以直接复用 Codex 内置编译器的缓存：

```bash
cd /Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript
TECTONIC_CACHE_DIR="$HOME/Library/Caches/TectonicProject.Tectonic" tectonic main.tex
```

当前产物：`main.pdf`（10 页，2.28 MB）。**页数是这份 `article` 骨架原有的排版尺度**（6 月版为 9 页），
不是排版错误；正式投稿若换用 OUP 的 Application Note 模板，篇幅与版式都会随之改变。

## 二、参考文献核验结果（Crossref，2026-10-13）

| 条目 | 状态 |
| --- | --- |
| Bustin 2009 MIQE, Clin Chem 55(4):611-622, DOI 10.1373/clinchem.2008.112797 | ✅ |
| Bustin 2010 MIQE précis, BMC Mol Biol 11:74, DOI 10.1186/1471-2199-11-74 | ✅ |
| Livak & Schmittgen 2001, Methods 25(4):402-408, DOI 10.1006/meth.2001.1262 | ✅ |
| Pfaffl 2001, Nucleic Acids Res 29(9):45e, DOI 10.1093/nar/29.9.e45 | ✅ |
| Li et al. 2022 qPCRtools (R), Front Genet 13:1002704, DOI 10.3389/fgene.2022.1002704 | ✅ 被引 32 次（OpenAlex，2026-10-13） |
| Ahmed & Kim 2018 pcr, PeerJ 6:e4473, DOI 10.7717/peerj.4473 | ✅ |
| Pabinger et al. 2014 survey, Biomol Detect Quantif 1(1):23-33, DOI 10.1016/j.bdq.2014.08.002 | ✅ |
| Vandesompele et al. 2002 geNorm, Genome Biol 3(7):research0034, DOI 10.1186/gb-2002-3-7-research0034 | ✅ |
| Rancurel et al. 2019 SATQPCR, Mol Cell Probes 46:101418, DOI 10.1016/j.mcp.2019.07.001 | ✅ |
| Wickham 2016 ggplot2 (book), Springer, DOI 10.1007/978-3-319-24277-4 | ✅ |
| Qt / Apache ECharts（misc 条目） | ✅ 网站可达，无需 DOI |

> 旧稿（现归档于 `archive/manuscript_biorxiv_2026-03.md`）里另有一条错误引用（"西瓜花叶病毒 … 2009;161(2):210-213"）实为 Pepino mosaic virus, J Virol Methods **162**(1-2):46-55, DOI 10.1016/j.jviromet.2009.07.008；**该旧稿已弃用，不要复用其中任何引用**。

## 三、待你确认 / 处理

1. **字数与图表上限**：OUP 官网（Instructions to Authors）在 Cloudflare 保护下，我未能读取，因此 Application Note 的字数上限与"图表数量上限"仍未核实。
   **用户决定（2026-10-13）：先按现在的 `article` 骨架投，不压缩**——`main.pdf` 共 10 页（3 个展示项 + 1 张表 + 11 条参考文献）。
   若投稿系统卡字数/图表数退回，我可在 15 分钟内出压缩版（约 1,500 词、2 个展示项）。
2. ~~单位英文名~~（已解决）：正文用云南农业大学两个单位；Xiang Li 的现地址单列 "Present address (X.L.): Yazhouwan National Laboratory, Sanya, Hainan, China"（**投稿系统里你的 current affiliation 可填崖州湾国家实验室**）。
3. ~~是否保留原单位~~（已解决）：两位作者均署单位 1、2（State Key Laboratory … in Yunnan；Key Laboratory for Agro-Biodiversity … Ministry of Education）。
4. **基金声明**：backmatter 保留原基金（2021YFD1000200、31801792、31960554、霍英东基金），与 Frontiers 2022 那篇一致，请确认是否仍适用。
   （**作者贡献段已按用户要求照 Frontiers in Genetics 2022 的写法改写**：
   "XL and HH designed this study and performed the analysis. XL wrote the manuscript. Both authors read and approved the final manuscript."）
5. **Associate Editor 提名**：若投稿系统要求，需你提名一位 AE（cover letter 中已留位置）。
6. **LaTeX 模板**：现用 `article` 骨架。若 OUP 提供 Application Note 专用 `.cls`/`.bst`，需在投稿前替换（我在 main.tex 顶部已写明该步骤）。
7. **Fig.1 语言**：截图界面为中文（软件为双语）。若你希望换成英文界面截图，需要在英文模式下重跑一次软件重新截屏——我可以按你给的步骤做。
   （2026-10-13 已改用英文界面截图，此条作废。原先做的三张中文界面截图
   `figures/fig1a_import_and_preview.jpg`、`fig1b_analysis_configuration.jpg`、`fig1c_results.jpg`
   现在未被正文引用，**保留在原地备选**；要归档或删掉请告诉我。）

## 三之二、JOSS 支线（`paper.md`）

- 2026-10-13：作者已加入 **Huichuan Huang**（与稿件一致，两位作者均署单位 1、2；X.L. 现地址写在 Acknowledgements）。
- `paper.md` 现 **1,484 词**（JOSS 要求 750–1750 词），章节齐全；仍待你补 **Research impact 段的自用/教学证据** 与 **确认 AI 使用披露段**（文件内已用 `[[ ]]` 标出）。

## 四、顺手发现的一个软件端问题（与本次投稿无关，但建议修）

- 界面"统计检验方法"下拉的第一项文字是 **Student's t-test**，但代码实际调用的是
  **Welch** 校正：`code/src/Core/ExpressionCalculator.cpp:96` 写死 `equalVariance=false`，
  而 `code/src/Core/StatisticalTest.cpp:170` 正是按该标志决定输出 "Student's t-test" 还是
  "Welch's t-test"。也就是说：**界面标签与计算结果不一致**（结果表里会显示 Welch's t-test，
  下拉里选的是 Student's t-test）。两种检验核心库都实现了，修法是把下拉拆成两项
  （Student's / Welch's）或把标签改为 "t-test (Welch)"。稿件里我已按事实写成
  "the Welch two-sample $t$-test"，但软件端建议一并改掉，避免审稿人试用时发现。

- **图表功能在当前版本是关闭的**：`code/web/js/app.js:1195` 的调用被注释
  （`// displayCharts(results); // Charts feature temporarily disabled`），
  图表导出按钮的事件绑定同样被注释（`code/web/js/app.js:1452-1453`），`code/web/index.html` 里也没有
  图表容器（只有引入 `echarts.min.js` 的一行）。因此 1.1.0 版**不出图**，
  论文 Figure 2 是由仓库脚本 `code/validation/plot_examples.py`（matplotlib）依据
  C++ 计算结果画的。我已经把摘要、Implementation 与 Figure 2 图注改成与代码一致的表述。
  两条路请选一条：**（推荐）** 把图表功能修好重新发布（这样 Figure 2 可以直接用软件出的图，
  JOSS 的"feature-complete"门禁也更稳）；或维持现状按现在的措辞投，投稿时不要宣称有绘图功能。

## 五、本轮修正的一处数值错误（重要）

- 原稿 Results 写"both significant at $p < 10^{-3}$"。实测（软件结果表截图与
  `code/validation/check_example_stats.R` 独立复算一致）真实 p 值为 **0.006 与 0.022**
  （Welch 两样本 t 检验，剂量组 vs 对照组）。已改为 `p = 0.006` 与 `p = 0.022`，
  并补了一段说明"按 R 版口径，表达量按**全样本** ΔCt 均值归一化，对照组不等于 1
  （本例 0.59 ± 0.12）"，同时给出零依赖复算脚本路径 `code/validation/check_example_stats.R`。
