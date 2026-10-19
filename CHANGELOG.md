# CHANGELOG

## 2026-10-19 13:33 — 发导师前的最后自检：补 Word 版、修补充材料图注双编号、清 cover letter 占位

- **补充 Word 版（此前缺失）**：新增 `code/scripts/15_make_manuscript_docx.py`，一条命令生成
  `manuscript/main_v10_20261015.docx`（11 页）与 `manuscript/supplementary_v3_20261019.docx`（4 页）。
  排版：A4、Arial 11 pt、1.5 倍行距、两端对齐；表格改三线表；图片按原比例缩到 15 cm 并居中
  （实测左右各 85.1 pt，长宽比 1.473 / 2.353 / 1.000，与源图一致，未被拉伸）。
- **pandoc 转 Word 的两个坑（已处理）**：① LaTeX 的 `\caption` 被转成图片 alt 文本，Word 里图注没有
  "Figure 1./Table 1." 编号——脚本改为在 Image Caption / Table Caption 段落前补编号；
  ② 补充材料两张图是 PDF，Word 无法显示（转出的 PDF 里一张图都没有）——脚本先用 pdftoppm 转 400 dpi PNG 再嵌入。
- **补充材料图注双编号**：`supplementary.tex` 原来靠 LaTeX 自动编号 + 图注手写 "Figure S1."，排出来是
  "Figure 1: Figure S1."。改为 `\renewcommand{\thefigure}{S\arabic{figure}}` 与 `\thetable`，
  图注去掉手写前缀；同时把 supplementary 的页面从 Letter 统一为 A4。
  **说明**：本机 Tectonic 的 TeX 包缓存不完整（重新下载时在 `hyph-uk.tex` 写入缓存失败），
  `supplementary_v3_20261019.pdf` 暂由同源 docx 经 LibreOffice 渲染（4 页 A4，内容与编号正确，
  字体/行距与主文 LaTeX 版略有差异）；`supplementary.tex` 已改好，TeX 缓存补齐后重编即为 LaTeX 版。
- **cover letter 两处必须改**：删除 `[[Associate Editor nomination …]]` 占位段（交付件不得带占位符）；
  删去 "Both authors have approved the submission"——黄老师尚未审阅，不能先写这句。
  同步更新 `cover_letter_20261015.md` 与生成脚本 `code/scripts/14_make_cover_letter.py`，重出 docx + pdf（1 页 A4）。
- **自检结论**：主文 PDF 7 页（A4，图在第 3 页、居中、236 ppi）；正文 1,805 词 + 摘要约 230 词；
  三份 docx 转 PDF 后均无占位符、无 Markdown 残留、图表编号连续；`citation` 绿框问题仍由 `hyperref[hidelinks]` 控制。
- **仍需用户/黄老师确认（不影响先发审阅稿）**：基金声明是否沿用（2021YFD1000200、31801792、31960554、霍英东基金）；
  Associate Editor 人选（投稿系统内填）；是否换成 OUP 官方 Application Note `.cls`（现在用 article 骨架）。

## 2026-10-15 18:47 — 投稿包：cover letter（Word 版）+ 按官方要求逐条复核

- **Cover letter**：新增 `manuscript/cover_letter_20261015.{md,docx,pdf}`（Word 版为投稿材料硬要求，A4 一页，Arial 10.5 pt 两端对齐）；生成脚本 `code/scripts/14_make_cover_letter.py`（内容改动后一条命令重出）。内容含：为何适合 Application Note、验证方式（与 R 包数值等价 + 统计层对零依赖 base R 参照）、公开获取方式（MIT、GitHub、v1.2.0 release 含安装包、无需注册、两年可用）、作者声明（**两位作者均已确认**，修正旧草稿里"single author"的错误）、AI 使用披露、AI 辅助工具说明；Associate Editor 提名处留占位。
- **官方要求复核**（`SUBMISSION_CHECKLIST.md`）：逐条对 v10/v2 稿件核过——摘要四段式 ✅、Availability and implementation 节含两年承诺 ✅、补充材料在摘要中提及 ✅、题名含软件名 ✅、1 图 + 1 表、正文 1,851 词（摘要+引言+实现+结果，不含 backmatter 与声明）✅；图 1 已换结果页截图 ✅；等价性表已含统计层与独立参照 ✅。
  - 说明：本机再次抓取 OUP 的 Instructions to Authors 仍被 Cloudflare 拦成 "Just a moment" 页面，**未能复核当年指南**，故一律以用户 2026-10-15 提供的官方原文为准，不臆造条款；投稿前如页面可访问请再对一遍。
- **未决项（需用户定）**：基金声明是否沿用原基金号；Associate Editor 提名人选；投稿时是否替换 OUP Application Note 的官方 `.cls`。

## 2026-10-15 18:22 — 第②步：统计层（p 值 / Tukey 字母）进等价性报告 + 独立参照对账；发布 v1.2.0

- **统计层验证**（对应导师意见一-3「等价性报告表要含 p 值与 Tukey 字母」与四「要有独立参照」）：
  - 新增 `code/validation/verify_stats.R`：**零依赖 base R**，从原始 Cq 值重算 ΔΔCt 各组均值/SD、Welch p 值、ANOVA p 值与 Tukey 字母，并与应用输出逐项对账 → 输出 `validation/results/equivalence_stats.csv`，末尾打印 `STATS-VERIFY PASS`。
  - `code/validation/run_cpp.cpp` 增加**输出文件名后缀**参数（不覆盖历史结果）与 **ANOVA 分支**：`run_cpp v12` → `cpp_*_v12.csv` 与 `cpp_deltadeltaCt_anova_v12.csv`。
  - 关键数字：均值 1.0145 / 1.9169 / 2.7403；Welch p = 0.0060274 / 0.0221908；ANOVA p = 0.0034471；Tukey 字母 **b / ab / a**（0 vs 0.5 的 p_adj = 0.052 属临界，恰好用来检验多重比较实现）。
  - 途中纠错：参照脚本里我第一版 CLD 只做"贪心并入已有列"，得到错误的 b/a/a（漏掉"0 与 0.5 不显著却无共用字母"），按 Piepho insert-and-absorb 重写后才与应用一致。
  - 补充材料 Table S1 改为上下两块：(a) 与 R 包的数值等价；(b) 统计层三方对账；并补一段解释字母含义与那个临界比较。
- **发布 v1.2.0**（用户要求"软件没问题就 release"）：
  - 打 annotated tag `v1.2.0` → CI 两作业：Windows ZIP 成功；**macOS 失败**（macos-latest 的 Xcode 26 SDK 已移除 AGL 框架，而 Qt 6.7.3 仍链接 `-framework AGL`）→ 工作流 macOS 作业改 `runs-on: macos-14`（该镜像 SDK 仍有 AGL）。
  - 本地出 macOS DMG（Homebrew Qt 6.11），过程中发现并修复一个**打包缺陷**：Homebrew Qt 的 `QtWebEngineProcess` helper 依赖未被改写，主窗口报 "Failed to load the web interface"（控制台为 `dyld: Library not loaded ... QtWebChannel`）。修法＝在 helper 的 Contents 下建指向主包 Frameworks 的**相对软链**、用 `install_name_tool` 把 helper 自身的 `/opt/homebrew/...` 绝对依赖改写成 `@executable_path/../Frameworks/...`、再重新 ad-hoc 签名；该修法已写回 `code/scripts/13_fix_helper_final.sh`（替换原先"复制全部框架"的做法，DMG 体积不再翻倍）。修复后实测：打包版可启动、WebEngine 正常加载界面、无 dyld 报错。
  - 产物：<https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.2.0>，附 `qPCRtools-1.2.0-macOS.dmg`（本地构建、已实测启动）与 `qPCRtools-windows.zip`（CI 构建）；发布说明含 macOS 未公证包"右键打开 / xattr"提示与验证脚本入口。
  - 稿件版本：`main_v10_20261015.pdf`（7 页）、`supplementary_v2_20261015.pdf`（3 页）。

## 2026-10-15 17:32 — 图 1 换成真实"分析结果页"截图；修复误差线"画不出/不居中"两个 bug

- **图 1 换图**（导师一-3 要求）：用 1.2.0 实机跑一遍捆绑示例（ΔΔCt + Welch + 对照组 0）后截图，替换原来的首页截图。成品 `manuscript/figures/fig1_results_page_en_v3.png`（1262×857），上下两段拼自同一窗口的两个滚动位置：上段＝汇总表（Mean/StdDev/PValue/Significance），下段＝界面内的相对表达量图（三条 SD 误差线 + 显著性星号）。成品 PDF：`manuscript/main_v9_20261015.pdf`（7 页，图在第 3 页、236 ppi）。改图同时在 `main.tex` 加 A4/2.5 cm 页边距（本地预览用），并给 abstract/introduction 加 `\sloppy` 消掉超版心。
- **修 bug 1：SD 误差线一根都画不出来**（`code/web/js/app.js`）。ECharts 5.4.4 的 custom series 里 `params.data` 是 `undefined`——实测 SSR 渲染抛 `TypeError: Cannot read properties of undefined (reading '2')`，异常被渲染循环吞掉（2026-10-15 用 `echarts.init(null,null,{renderer:'svg',ssr:true})` 复现并定位）；旧代码还用 `params.context.rendered` 在首项一次性渲染整表，等于只处理了第一项。改为逐项渲染 + `api.value(dim)` 取维度值，每项返回竖线 + 上下端帽共 3 条线。
- **修 bug 2：误差线不居中**（用户 2026-10-15 当面指出"第一个和第三个柱子的误差线没有在中间位置"）。实测：`api.size([1,0])[0]` 返回的是整个类目带宽（905.2px），而 ECharts 自行分配的 `barGap`/`barCategoryGap` 让真实柱心间距只有 246px（不是 301.7px），于是三根误差线落在 205.7/507.4/809.1、柱心却在 261.4/507.4/753.4。修法：给 bar 系列显式设 `barGap: 0` 与 `barWidth: '<100/组数×0.7>%'`（实测 `barWidth` 百分比基准正好是 `api.size([1,0])[0]`），custom series 用同一基准算水平偏移。修复后实机复核（读像素）：柱心 386.5 / 641 / 895，误差线 387 / 641 / 895 —— 三根全部居中。
- **回归测试**：`code/tests/js/smoke.test.js` 新增两项断言——"每项误差线真的渲染出 3 条线（竖线 + 上下端帽）"与"误差线落在自己柱子的正中间"（逐值校验 x 与三条线的对称关系）。`node code/tests/js/smoke.test.js` → SMOKE PASS（10 项图表检查全过）；`ctest` 52/52 通过。
- **过程记录**：本轮为出这张图，用 Computer Use 驱动 1.2.0 实机（`/private/tmp/qpcr_verify_app/qPCRTools.app`，Resources/web 软链到仓库 `code/web`）跑完整流程并截图；中途踩到"窗口绑定被 macOS 覆盖层窗口干扰导致点击落到菜单栏"，重启 App 后恢复。截图与中间产物留在 `/tmp/qpcr_shot/`。

## 2026-10-15 16:09 — 逐条核对导师意见的落实状态；补做仓库/格式类遗留项

用户问"导师提的意见都改完了吗"，把 `qPCRtools GUI 审稿意见-黄老师-20261014.docx` 全文重新取出一一对照
（此前只细看过"一""二"两节，"三、仓库和格式""四、怎么改"没逐条落）。本轮先做掉其中能立刻做的。

### 本轮新增完成

- **vcpkg.json 版本号** 1.0.0 → **1.2.0**（导师"三"第 3 条）。
- **README 与稿件/代码一致化**（导师"三"第 2 条）：删掉"Excel 导入还在计划中""ggplot2 风格图表"等过时说法；
  改为如实描述 1.2.0 的能力——界面内出图（均值守柱 + SD 误差线 + 显著性标注；标准曲线 log_d 散点 + 回归线）、
  PNG 导出、质控预警、分析记录导出；Excel 导入注明"仅在可选 OpenXLSX 组件下可用，发布的二进制读 CSV/TXT"；
  统计口径注明界面暴露的是 Welch t 检验与秩和检验（配对/符号秩在核心库实现并有测试）。
- **摘要改为官方四段式**（导师"三"第 7 条）：`Summary` / `Availability and Implementation` / `Contact` /
  `Supplementary Information`。
- **AI 使用披露**（导师"三"第 4 条后半）：backmatter 新增 `Use of AI-assisted tools` 段（OpenAI Codex 用于
  代码重构、生成单元测试与语言审校；分析与结论由作者核验）。**措辞待用户确认**。
- **Excel 导入的括注**在正文 Availability 段补齐（避免与"已发布二进制不支持 xlsx"冲突）。

### 逐条状态（对照导师原文）

| 导师条目 | 状态 |
| --- | --- |
| 一-1 GUI 占位 p 值（t / Wilcoxon）与 RqPCR 处理组 p 值 | ✅ 已修（前两项 v1.1.0 起修复，本轮补 Wilcoxon 小样本精确检验）|
| 一-2 ΔΔCt 校准基准与 SE 公式 | ✅ C++ 与 R 包均改为对照组基准；R 包 SE 改 sd/√n |
| 一-3 "与 R 版等价"验证逻辑（表里没比 p/字母；validation 未推） | ⚠️ **部分**：仓库已推、单测层面 t/Wilcoxon/ANOVA/Tukey 的 p 值已逐值对比 R，但**报告用的等价性表（Table S1）仍只有表达量/SD/SE/斜率/R²/效率，未含 p 值与 Tukey 字母**，也未加"独立参照/多工具同数据对照表" |
| 一-4 图 2A 线点错位 + 公式（1） | ✅ 已修（log₄ 统一、范围外空心点、公式口径改写）|
| 一-5 R 包 CalExpCurve 反对数 + 技术重复层级 | ✅ 已修（R 包侧，含新增验证脚本；未提交）|
| 一-6 字母分组（R: cld level/拼写；C++: 假 Tukey） | ✅ 两边都修（真学生化极差分布 + level = 0.05）|
| 一-7 geNorm 输入与逐步剔除 | ✅ 两边都修（相对量 + 逐步剔除），C++ 与 R 结果一致 |
| 一-8 RqPCR 按数值去重 | ✅ 已修（两版）|
| 一-8 RqPCR 按 BioRep 编号配对 | ⚠️ 核查后判定是**方法原文要求**（Hilliou & Tran 2013 区组设计），已写进文档与论文；界面上的"是否配对/标定样品"选项未做，需用户按实验设计确认 |
| 二-1 新意不够 → 功能对比表 + 真实用户任务证据 | ⚠️ 功能对比表 ✅（Table 1）；**真实数据/具体用户任务的对比证据未做**（JOSS 段同样缺）|
| 二-2 引言竞品描述不准确 | ✅ 已改（REST 免费、仪器自带软件、补 LinRegPCR/Auto-qPCR/DAG Expression 并逐条核验）|
| 二-3 作图功能关掉了 | ✅ 已恢复（表达量图 + 标准曲线图 + PNG 导出）|
| 二-4 统计尺度事先定好 + 多重校正 + 效率校正的适用性 | ⚠️ **部分**：尺度已明写（线性相对表达量，与 R 版一致）；Tukey 控制基因内族错误率；**跨基因多重校正只写成局限、未实现**；效率法适用性只有一句 MIQE 说明 |
| 二-5 没有质控 | ✅ 已加（Cq 上限 / 技术重复 SD / 效率 90–110%，只提示不删数据）|
| 二-6 功能不全（CalExpCurve、CalRTable） | ⚠️ 论文如实标注"未移植"；**代码未实现** |
| 三-1 README BibTeX 作者列表 | ✅ 早已按 Crossref 改正（2026-10-13）|
| 三-2 README 与稿件不一致 | ✅ 本轮改齐 |
| 三-3 vcpkg.json 版本号 | ✅ 本轮改 1.2.0 |
| 三-4 清理开发过程文件（docs/superpowers、fix_helper*.sh、PROGRESS/STATUS） | ⚠️ fix_helper*.sh 已不在、STATUS.md 已归为 docs/08_status.md；**docs/superpowers/ 仍有 3 个文件**，建议移入 archive/（需用户同意后执行）|
| 三-4 后半 AI 使用声明 | ✅ 本轮加（待确认措辞）|
| 三-5 Zenodo 存档 DOI | ⏳ **未做**（需用户授权建 release 或本人操作）|
| 三-6 回复并关闭 R 仓库 5 个 issue | ⏳ **未做**（依赖 R 包修正版发布，且需用户决定）|
| 三-7 篇幅/图表数量 | ✅ 已按最严口径收（1,856 词 + 主文 1 图 1 表）|
| 三-7 摘要分段 | ✅ 本轮改为四段式 |
| 三-7 图 1 换成分析结果页截图 | ⏳ **未做**（需实机重截）|
| 三-7 图 3 挪补充材料 | ✅ 已挪（Figure S2）|
| 四 评估旧版本影响 + 重跑 2022 年文章图表做修复前后对照 | ⏳ **未做**（依赖修正版 R 包装好）|
| 四 发布修复说明 + 回复关闭 issue | ⏳ 未做 |
| 四 补齐功能（他列的 7 项） | ✅ 3/7：作图、分析记录、质控；⏳ 未做：效率自动判断、统计尺度可选 + 多重校正、RDML/仪器格式导入、CalExpCurve 移植 |
| 四 重做验证（p 值与 Tukey 字母入表、边界情况、独立参照/多工具对照表） | ⚠️ 部分（单测已覆盖 p 值与字母、边界情况有测例；**报告表与多工具对照未做**）|
| 四 改文章定位（主打"无需编程、符合 MIQE、可重现"） | ⚠️ 部分（引言/贡献已改，MIQE 与可重现尚未作为主打写进摘要）|
| 五 投稿策略（Bioinformatics → 备选 BMC Bioinformatics，两份格式） | ⏳ 未做（cover letter 仅一版）|

### 产物

- `manuscript/main_v7_20261015.pdf`（10 页；四段式摘要 + AI 披露段；正文 1,856 词）。
- 本轮改动文件：`code/vcpkg.json`、`README.md`、`manuscript/sections/{abstract,implementation,backmatter}.tex`、
  `manuscript/SUBMISSION_CHECKLIST.md`（新增两条对照行）。

## 2026-10-15 16:03 — 拿到官方 Application Note 要求；正文压到 1,777 词、主文只留 1 图 1 表 + 补充材料分册

用户 2026-10-15 从 OUP 页面取得官方原文（此前两次尝试都被 Cloudflare 挡住，未核实）：

> Application Notes — **Up to 4 pages; this is approx. 2,600 words or 2,000 words plus one figure** …
> Software or data must be freely available to non-commercial users. **Availability and Implementation must be
> clearly stated in the article.** Authors must also ensure that the software is **available for a full two years**
> following publication. … Additional supplementary data can be published online-only by the journal.
> **This supplementary material should be referred to in the abstract** … **The name of the application should be
> included in the title.**

### 1. 按最严的一档（2,000 词 + 1 图）重排

- 正文由 2,222 词压到 **1,777 词**（摘要 231 + 引言 465 + 实现 595 + 结果 336 + 可用性 150），逐节精简但保留
  全部关键数值与出处；两处超出版心的段落用 `\sloppy` 收掉（实现段与结果段）。
- **主文只留 1 图 + 1 表**：Figure 1（界面截图）与 Table 1（工具功能对比，导师要求的那张）；
  **Figure 2（标准曲线 + 表达量）、Figure 3（等价性散点）、Table 2（等价性汇总）移入新增的
  `manuscript/supplementary.tex`**，成为 Figure S1、S2 与 Table S1，并在摘要与正文中引用。
- 结果节的数字全部保留（1.92 / 2.74、p = 0.006 / 0.022、对照组 1.01 ± 0.21、斜率 −2.07…−2.24、
  最大相对差 3.8e-15 / 2.5e-3 / 2.1e-15），只是把图表的细节描述挪到补充材料图注。

### 2. 官方另外几条硬要求的落实

- `Availability` 段改名为 **`Availability and implementation`**，先写实现方式（C++17/Qt6、CMake、依赖），
  再写获取方式（MIT、GitHub 仓库、预编译二进制、**无需注册**），并明确承诺
  "**available for at least two years after publication**"。
- 摘要末句加入 **Supplementary figures and tables…**（官方要求"补充材料必须在摘要中提到"）。
- 题目含软件名 `qPCRtools` ✓（原本已满足）。
- 稿件描述的版本更新为 **1.2.0**（1.1.0 为上一版），Figure 1 图注注明截图来自 1.1.0、1.2.0 新增了
  界面内作图 / 质控预警 / 分析记录导出。

### 3. 投稿清单重写（`manuscript/SUBMISSION_CHECKLIST.md`）

- 新增**「官方要求 ↔ 本稿设置」逐项对照表**（字数与展示项、免费可用、Availability and Implementation、
  两年可用、补充材料在摘要中提及、软件名入题、无需注册、安装成本），按硬性要求逐条标注 ✅；
  并记录"若编辑要求只留 1 个展示项，把 Table 1 移补充材料即可"。
- 更新稿件状态（`main_v6_20261015.pdf` 10 页 / `supplementary_v1_20261015.pdf` 2 页）、
  参考文献核验表（17 条，新增 6 条 REST/LinRegPCR×2/qBase/DAG Expression/Auto-qPCR 的 DOI）、
  待办（Zenodo DOI、OUP 模板、R 包提交、2022 图 2 重跑、GUI 端到端取证、JOSS 支线）。

### 4. 顺手修掉界面上的一处标签不一致

- 分析页"统计检验方法"下拉原本写 **Student's t-test**，而代码对该选项调用的是 **Welch** 校正
  （`equalVariance=false`，结果表里也显示 Welch）→ 改为 **t-test (Welch)**，与实现一致。

### 5. 产物与验证

- 沙箱 tectonic 编译：`main.pdf`（10 页，exit 0）、`supplementary.pdf`（2 页，exit 0）；
  产物已按版本号存入 `manuscript/main_v6_20261015.pdf` 与 `manuscript/supplementary_v1_20261015.pdf`
  （旧版本一律保留不删）。
- 参考文献核对：编译出的列表为 **[1]–[17] 连续完整**（含被分页断行、grep 容易漏掉的 [13]）。
- 遗留的排版提示：Table 1 在 `article` 骨架下有 2–9 pt 的 overfull（极小、肉眼不可见）；投稿换用 OUP
  模板后会重排，故不再为它调整字号。

## 2026-10-15 15:49 — 无 GSL 路径的标准曲线 p 值改精确 t 分布；CI 增加无 GSL 与前端两个作业

### 1. C++：标准曲线回归 p 值不再依赖 GSL

- 问题（导师审稿意见第 1 条的遗留项）：`StandardCurve::calculateSingle()` 里斜率的双侧 p 值在**无 GSL**构建下
  走 `2*(1-Φ(|t|))` 的正态近似，与 GSL / R 的 `2*pt(-|t|, df)` 不一致。
- 改法：`StatisticalTest` 新增公开静态方法 `tTwoSidedPValue(t, df)`（复用本文件内已验证的正则化不完全 Beta
  实现，与 GSL/R 同精度），`StandardCurve` 两条分支统一调用它 → 去掉了唯一的"无 GSL 回退"。
- **验证**：本机额外配置**无 GSL** 构建（`-DCMAKE_DISABLE_FIND_PACKAGE_GSL=ON`，`CXX_DEFINES` 中无 `HAS_GSL`、
  测试二进制不链接 GSL）→ `ctest` **52/52 通过**，其中 `StandardCurveMatchesR` 会把 p 值与 R `lm()` 的
  `pValue_real` 逐值比对 ✓。带 GSL 的构建同样 52/52 ✓。

### 2. CI：新增两个作业（与既有 C++ 作业并行）

- `test-no-gsl`：不装 GSL 的构建 + ctest（覆盖上面这条路径，防止回退）。
- 既有 `test` 作业末尾追加**前端 jsdom 冒烟测试**（`code/tests/js` → `npm ci` → `node smoke.test.js`），
  覆盖 43 个函数存在性、分析记录、两张图的 option 结构、质控三类预警等。

### 3. R 包 `CalExpCurve()`（导师第 5 条，工作区未提交）

- **反对数**：`(Cq - Intercept)/Slope`（对数浓度）→ `dilution^((Cq - Intercept)/Slope)`（浓度），
  新增 `dilution = 4` 参数（与 `CalCurve()` 内部 `log(Conc, base = dilution)` 一致），roxygen 与
  `man/CalExpCurve.Rd` 同步。
- **技术重复层级**：先在 `(Gene, Treatment, BioRep)` 内平均技术孔，再以生物学重复为单位做组间检验与作图；
  缺 `BioRep` 列时明确 warning（不再静默按孔当独立样本）。顺带修掉 `fig.type = "bar"` 的柱高
  `mean.expre / n` → `mean.expre`。
- **新增测试**：`CRAN/qPCRtools/tests/01_verify_calexpcurve.R`（随 R CMD check 运行）——判别用例按生物学重复
  p = 0.074 → `NS`，而按孔计算是 p = 1.2e-4 → `***`，因此该脚本能真正锁住"必须按生物学重复检验"。
  本地 `Rscript tests/01_verify_calexpcurve.R` 通过。

### 4. 待办

- R 包工作区仍未提交（含导师补丁半成品与行尾差异）；`CalExpCurve` 修正后**2022 年论文图 2 需重跑看结论**。
- 官方篇幅/展示项上限仍未核实（Cloudflare）；正文已 2,222 词 + 3 图 2 表。
- GUI 端到端取证（作图 / 分析记录 / 质控）。

## 2026-10-15 15:37 — 恢复作图功能（表达量图 + 标准曲线图）与质控预警；版本升到 1.2.0

导师意见「二」第 3 条（作图功能关掉了，与文章自己说的需求对不上）与第 5 条（没有质控，示例数据里
Cq=40、R²=0.75 都不提示）。用户要求：作图尽量加回来；质控阈值由我查文献决定。

### 1. 作图功能（原来只是桩函数，结果页连容器都没有）

- **先查清事实**：`displayCharts()` 是写死假数据的桩（Gene1/2/3、Control/Treatment 与 2.5/1.8/3.2 等编造值），
  调用被注释、`web/index.html` 里也没有 `#mainChart` 容器 —— 即"图表视图"从未真正可用。
- **表达量图**：新增纯函数 `buildExpressionChartOption(results, palette)`——从**真实结果表**取
  (Gene, Group, Mean, StdDev, Significance)，每个基因一组柱、柱顶 SD 误差线（ECharts 官方 custom series 写法）、
  柱顶标注显著性字母；兼容"逐重复表"（无 Mean 列时先按 gene×group 汇总 mean/SD）。结果页新增 Figure 卡片
  （`#mainChart`，420 px 高）+ PNG 导出（`exportChart()`，pixelRatio 3）。
- **标准曲线图**：新增 `buildStandardCurveChartOption(...)`——横轴与 C++ 一致为 `log_d(Conc)`（d = 稀释倍数），
  每个基因散点 + 由 slope/intercept 画出的回归线，标定范围外的点用空心标记单列（与论文图 2A 口径一致）；
  标准曲线页新增"标准曲线图"卡片 + PNG 导出。
- 修正原有的 resize 监听泄漏（每次分析都叠加一个监听器）→ 每个图只绑定一次。
- `docs/08_status.md` 的版本行与稿件 Implementation 段同步改为"界面内出图"。

### 2. 质控预警（只提示、不删数据；阈值可调）

- **阈值来源与口径**：MIQE 要求**报告**效率、技术重复离散度与检测限附近的数据，但**没有**规定统一判废阈值
  （Bustin et al. 2009, *Clin. Chem.* 55:611–622, DOI 10.1373/clinchem.2008.112797，稿件已引 `bustin2009miqe`）。
  因此把三项都实现成**可配置的预警默认值**，UI 与文档都写明"仅预警、不删除数据"：
  - `cqUpper = 35`：高于此值接近常见检测上限，需结合扩增曲线、阴性对照与 LOD 核查；
  - `techRepSd = 0.5` Cq：同一生物学样本技术重复的标准差；
  - `efficiencyLow/High = 0.90/1.10`：效率 90–110%（约对应斜率 −3.1 到 −3.6）。
- **实现**：纯函数 `buildQcReport({cqData, designData, concenData, scResults, thresholds})` → 预警列表；
  `runQcChecks()` 在每次分析成功后运行，`displayQcReport()` 渲染到结果页顶部的黄色警告区（`#qcSection`）；
  预警同时写进"分析记录"的 `[Quality control warnings]` 段（导师"可核查"要求）。
- 示例数据上的实证：`Gene1` 的 Cq=40 会触发 Cq 上限、技术重复 1 vs 5 的大离散会触发离散度预警，
  `Gene1` 效率 0.954（在 90–110% 内）不误报。

### 3. 验证与版本

- `code/tests/js/smoke.test.js`：期望函数 43 个；新增/扩充断言共 12 项检查——分析记录（11 字段 + 空快照 + 标曲口径）、
  表达量图（8 项结构断言 + `displayCharts` 真的把 option 交给 ECharts + 逐重复表汇总）、
  标准曲线图（6 项结构断言 + `displayStandardCurveChart` 交接）、质控（三类各触发一次 + 干净数据零误报 +
  分析记录带质控段）。**SMOKE PASS**。
- C++ 测试 **52/52**；GUI 重新构建通过（版本号已升到 **1.2.0**，二进制内 `1.2.0` 已核对）。
- 稿件沙箱编译 exit 0、12 页 → `manuscript/main_v5_20261015.pdf`。
- **未验证的部分（如实说明）**：图在真实 GUI 里的**渲染外观**（误差线对齐、标签是否被裁、配色是否协调）
  本轮没有目视确认，只做了结构级单测与编译验证；建议下一步用 Computer Use 打开应用跑一次并截图确认。

### 4. 待办

- 官方篇幅/展示项上限仍未核实（官网 Cloudflare）；当前正文 2,113+ 词、3 图 2 表，需按上限裁剪。
- GUI 端到端取证（作图 + 分析记录导出 + 质控面板）。
- R 包侧仍未做的：GSL 缺失路径、`CalExpCurve` 技术重复层级、以及把本轮 C++ 修正在 R 包同步。

## 2026-10-15 15:19 — CI 纳入前端 jsdom 冒烟测试

- `.github/workflows/ci.yml` 在 `ctest` 之后新增一步：`code/tests/js` 下 `npm ci`（jsdom）→ `node smoke.test.js`。
- 原因：新加的"分析记录"断言、以及既往的关键函数存在性/`initializeUI` 无异常检查，此前都只在本地跑，
  CI 只覆盖 C++ 侧；前端回归无人自动把关。

## 2026-10-15 15:18 — 新功能：分析记录导出（.txt）；稿件补上检验尺度与多重比较口径

导师意见「二」第 1 条把"让不会编程的人也能正确完成整个分析、并且把分析过程完整保存下来（可核查、可重跑）"
列为最值得写的贡献。本轮把这件事实做出来（原来只有结果表导出，没有任何参数留痕），再把稿件的相应表述补上。

### 1. 新功能：分析记录导出（GUI）

- **C++**：`WebBridge::saveTextFile(text, filePath)`（新增，`include/GUI/WebBridge.h` + `src/GUI/WebBridge.cpp`），
  UTF-8 写出，路径为空或写不出时发 `errorOccurred`；与既有 `exportToCSV/Excel` 走同一套 `showSaveDialog` 流程。
- **前端**（`web/js/app.js`）：
  - 载入 Cq / design / concentration 文件时记下**文件名**（`currentCqFileName` 等）；
  - 每次分析成功后 `captureAnalysisParameters()` 冻结本次**真实用过**的参数（方法、内参、对照组、检验方法、
    离群值开关、标曲的 lowest/highest/dilution/byMean）与**输入规模**（行×列）、输出规模（summary/raw/statistics 行数）；
  - 纯函数 `buildAnalysisRecord(snapshot, appVersion, timestamp)` 生成可核查的文本记录；
  - `exportAnalysisRecord()` 通过 `bridge.getAppVersion()` 取真实版本号，默认文件名 `analysis_record_YYYYMMDD.txt`；
    无 bridge（浏览器演示模式）时走 `downloadFile`。
- **界面**：结果页摘要表表头新增 `Analysis record` 按钮（`web/index.html`），i18n 键 `btn.analysisRecord`（中英）。
- **测试**：`code/tests/js/smoke.test.js` 扩充——把三个新函数纳入"必须存在"清单，并对
  `buildAnalysisRecord` 做 11 项内容断言（版本号、时间戳、方法、内参、对照组、检验、输入文件名与行×列、
  输出计数、不得出现 undefined/NaN）、空快照返回空串、标曲口径只列标曲参数。**SMOKE PASS**（37 个函数全在）。
- **验证边界（如实说明）**：C++ `saveTextFile` 只做了编译验证（GUI 类依赖 QtWebEngine，未纳入单测）；
  GUI 里点按钮 → 原生保存对话框 → 落盘这一段**本轮未端到端实跑**，建议下次用 Computer Use 跑一遍取证。

### 2. 稿件补上统计口径（导师「二」第 4 条）

- `sections/implementation.tex` 统计段明确写出：**组间比较在"线性相对表达量尺度"上做**（与结果展示、R 版一致），
  不使用 ΔCt 尺度，并说明"因为两种尺度给出不同 p 值，所以整个软件与验证只用一个尺度"。
- 同一段写明多重比较口径：多组设计用 **Tukey HSD 控制该基因两两比较的族错误率**；**跨基因 multiplicity
  未额外校正**（p 值按基因给出），并把这一点作为**局限明写**而不是留在暗处。
- `sections/abstract.tex`：删掉"commercial alternatives are costly, closed-source, and platform-restricted"
  这类不准确表述（与引言同步），改为"仪器自带软件 / 免费独立程序 / 网页工具各自只覆盖一步，或绑定平台、或需上传数据"；
  并把"分析记录导出"写进贡献句。

### 3. 验证与产物

- C++ 测试 **52/52**；GUI（`qPCRtools.app`）重新构建通过（`WebBridge.cpp.o` 与二进制时间戳已更新）；
  jsdom 冒烟 **SMOKE PASS**。
- 稿件沙箱编译（tectonic）exit 0、12 页；正文合计 **2,113 词**（摘要 271 + 引言 565 + 实现 736 + 结果 449 + 可用性 92），
  另 3 图 2 表。**注意**：篇幅与展示项上限仍未核实（官网 Cloudflare）；按导师"约 2000 词"的口径已略超，
  摘要 271 词也可能超刊限——待拿到官方要求后再统一裁剪（优先把 Figure 3 / Table 2 移入补充材料）。
- 成品：`manuscript/main_v4_20261015.pdf`（v2/v3 保留不删，命名规则改为每轮一个新版本号）。

## 2026-10-15 15:09 — 引言竞品描述改正 + 工具功能对比表（Table 1）；成品 PDF 改版本号命名

按导师意见「二」的第 2 条（引言对现有工具的描述不准确）与第 1 条（要一张功能对比表）改稿；
同时按用户要求把成品 PDF 改为版本号命名，不再覆盖旧文件。

### 1. 引言重写（`manuscript/sections/introduction.tex`）

- 原来把 REST 与 QuantStudio/CFX Maestro 一起说成"costly, closed-source, restricted to a single
  OS or instrument"——不准确。现按四类分别描述并逐条给引用：
  ① 脚本实现（`qPCRtools`、`pcr`）；② 仪器自带软件（QuantStudio Design & Analysis、CFX Maestro）；
  ③ 免费独立程序（REST；LinRegPCR 估效率/基线；qBase + geNorm 选内参）；
  ④ 免费网页工具（SATQPCR、DAG Expression、Auto-qPCR；需上传数据到远端或本地部署 web 栈）。
- 结尾的"gap"改为：缺的不是算法，而是**交付方式**——把"已验证的数值等价"与"原生、跨平台、零依赖的
  图形流程"合在一起。
- 贡献列表由四条压成三条（架构 / 等价验证 + 随软件发布的验证包 / 统计与功能扩展），并如实写明移植中
  发现的聚合缺陷归属：RqPCR 生物学重复处理、geNorm 稳定性输入、ΔΔCt 校准基准**两版共有**；
  多重比较 p 值 **C++ 独有**（不把修 bug 当创新点，但要说清改了什么）。

### 2. 新增 Table 1：工具功能对比

- 7 行（R 包 / 本软件 / REST / LinRegPCR / qBase / 仪器软件 / 网页工具）× 6 列
  （免费、免编程、离线、效率校正、统计、内参筛选）；符号 ●/○/○-●，表注写明信息来源、
  "统计在后续 qBase+ 才有"、以及"vendor 软件随仪器与版本而异"。
- 表格宽度经一次返工（首版溢出 182 pt，改为 `\footnotesize` + 缩短行标签后通过编译）。
- 因新表插在引言，等价性表自动成为 **Table 2**（正文用 `\ref`，无需改字）。

### 3. 新增并核验 6 条文献（Crossref 逐条核验 ✅）

Pfaffl 2002 REST（NAR 30:e36, DOI 10.1093/nar/30.9.e36）；Ramakers 2003（Neurosci Lett 339:62-66,
DOI 10.1016/S0304-3940(02)01423-4）与 Ruijter 2009（NAR 37:e45, DOI 10.1093/nar/gkp045）LinRegPCR；
Hellemans 2007 qBase（Genome Biol 8:R19, DOI 10.1186/gb-2007-8-2-r19）；Ballester 2013 DAG Expression
（PLoS ONE 8:e80385, DOI 10.1371/journal.pone.0080385）；Maussion 2021 Auto-qPCR
（Sci Rep 11:21293, DOI 10.1038/s41598-021-99727-6）。

### 4. 成品 PDF 改用版本号命名（用户要求）

- `manuscript/main_v2_20261015.pdf`（图 2 修正后）、`manuscript/main_v3_20261015.pdf`（含本轮引言/表格改动）；
  原始 `main.pdf` **保持不动**，后续每轮编译都出新版本号，不再覆盖。
- 沙箱编译（`/tmp/qpcr_manu`，tectonic）exit 0、12 页；Table 1 渲染与全部引用解析正常；
  正文合计 1,909 词（不含表与图注）。

### 5. 待核验 / 待办

- **Bioinformatics Application Note 的篇幅与展示项上限仍未核实**：官网作者指南页被 Cloudflare
  人机验证挡住（curl 403；应用内浏览器停在"正在进行安全验证"，未绕过）。导师口径为
  "≤4 排版页、约 2600 词或 2000 词 + 1 图"。当前 main 为 3 图 + 2 表，若按该口径需把
  Figure 3 / Table 2 移入补充材料（导师此前也建议 Figure 3 挪补充材料）——待用户打开官网核对后再定。
- 下一步（导师意见「二」其余各条）：作图功能是否恢复、质控提示（Cq 过高/技术重复离散度/效率越界）、
  统计尺度与多重校正的写法、以及"分析过程可核查/可重跑"的证据（可能需要给软件加一份分析记录导出）。

## 2026-10-15 15:00 — 第⑦项：图 2A 横轴/斜率错位修正、图 2B 按新数值重画、公式（1）口径统一

导师意见第 4 条（图 2A 的回归线与数据点对不上、公式（1）写法有歧义）。查清后确认有**两个**原因，
都已修正，并重画了两张面板。

### 1. 图 2A 错位的两个来源

- **轴与斜率不同尺度**：C++ `StandardCurve` 的横轴是 `log_d(Conc)`（d = 稀释倍数 4，即"稀释步数"），
  而旧脚本把点画在 `log10(Conc)` 上（斜率却是 log4 尺度的）——正是导师看到的高浓度端线点分离。
  现统一为 `log4(Conc)`，标签写 `log4(concentration)`。
- **标定范围外的点未区分**：数据含 1 与 16384 两档浓度，而回归只用 4–4096（`run_cpp.cpp` 的
  lowestConcen/highestConcen）。旧脚本把它们与标定点混画，也会造成"线点对不上"。
  现范围内实心、范围外空心并标注 `hollow = outside calibration range (4-4096)`。
- **程序化自检**（脚本内置打印，避免目视判断）：范围内**均值点**的残差 RMS
  Gene1/2/3/4 = 2.04 / 0.110 / 0.156 / 0.098 Cq（Gene1 本身 R² 仅 0.751）；
  若误用 log10 横轴则为 3.80 / 3.25 / 3.48 / 3.43 Cq —— 修复效果有数可依。
- 配色改为规范内色板（A 图用紫色系 `five` 前 4 色；B 图用红色系 `hundred` 第 1/3/5 色），
  不再用 matplotlib 默认色。

### 2. 图 2B 与稿件数字

- 图 2B 改由**当前** `cpp_deltadeltaCt.csv`（对照组校准口径）绘制：0 组 1.01 ± 0.21、
  0.5 组 1.92 ± 0.21、1 组 2.74 ± 0.56，显著性 `**` / `*`。
- `manuscript/sections/implementation.tex`：公式（1）上方文字由"对 log10(浓度) 回归"改为
  "对 log_d(浓度)（稀释步数）回归，斜率 s 以 log_d 为单位"，与 `E = d^{-1/s} − 1` 一致。
- `manuscript/sections/results.tex`：斜率注明"per four-fold dilution step（log4 scale）"；
  图注补上 log4 横轴与空心点说明。

### 3. 产物与验证

- 新图：`manuscript/figures/fig2_results_v2.pdf`（**新文件**；旧 `fig2_results.pdf` 未覆盖，
  已由 LaTeX 改为引用 v2）。
- 稿件在沙箱（`/tmp/qpcr_manu`）用 tectonic 编译成功（10 页、exit 0）；`pdftotext` 核对正文已含
  1.92 / 2.74 / 1.01 ± 0.21 / "log4 (concentration)" / "calibrated to the control group"；
  `pdfimages -list` 显示图 2 以矢量形式嵌入（唯一位图是图 1 的界面截图）。
- 绘图脚本运行方式不变（`cd code && python3 validation/plot_examples.py`），输出文件名改为 v2、
  并内置残差自检输出。

### 4. 待用户处理

- `manuscript/main.pdf`（旧 10 页 PDF）与 `manuscript/main.docx` 仍是旧数值/旧图，需要重新生成
  （命令见 `manuscript/SUBMISSION_CHECKLIST.md`；本轮未覆盖生成产物）。
- 旧图 `manuscript/figures/fig2_results.pdf` 已废弃（2A 错位、2B 旧口径）；删除命令见本轮汇报。

## 2026-10-15 14:46 — 第⑤⑥项：ΔΔCt 校准基准改为对照组；SE 公式核查；R 包同步修补（未提交）

用户指示「按照文献的公式来；其余的按你的建议」。本轮把 ΔΔCt 的基准改成文献定义（Livak & Schmittgen 2001：
校准样品 = 对照组），核查了 SE，并把 R 包侧同一批问题一并修好（R 包工作区**未提交**，见最后一节）。

### 1. ΔΔCt 校准基准：全样本均值 → 对照组（C++，有意偏离 R 包 1.0.1）

- 原实现（与 R 包 `CalExp2ddCt` 1.0.1 相同）用**所有组**的平均 ΔCt 做基准 → 对照组 ≠ 1、换 `ref_group`
  结果不变，且与稿件公式（2）自相矛盾（导师意见第 2 条；R 包 issue #5 / #1）。
- 现按文献：基准 ΔCt 只取**对照组**；对照组缺失时按「第一个有数据的组」回退并打印警告。
  文件：`code/src/Core/ExpressionCalculatorDeltaDeltaCt.cpp`。
- **示例数据实测**（`examples/cq.csv` + `design.csv`，3 组 × 3 重复）：

  | 组 | 旧口径（全样本均值） | 新口径（对照组基准） | SD（新） | Welch p vs 组 0 |
  |---|---|---|---|---|
  | 0 | 0.5872 ± 0.1214 | **1.0145 ± 0.2098** | 0.2098 | — |
  | 0.5 | 1.1095 ± 0.1192 | **1.9169 ± 0.2060** | 0.2060 | 0.006027（不变）|
  | 1 | 1.5861 ± 0.3229 | **2.7403 ± 0.5580** | 0.5580 | 0.022191（不变）|

  旧值 × 常数因子 1/0.5871622 即新值（同一个 2^(−ΔΔCt) 常数平移），**组间 p 值不变**（检验用逐重复值）。
- 新增/调整测试：`ExpressionCalculator.DeltaDeltaCtMatchesReference`（对照修正后 fixture）、
  `ExpressionCalculator.DeltaDeltaCtCalibratesToControlGroup`（① 对照组 ≈ 1；② 换对照组结果必须变化；
  ③ 旧口径 = 新口径 × 常数因子）。`ctest` **52/52 通过**。
- fixture：`expected_expression_ddct.json` 现同时保存 `by_gene`（修正）与 `by_gene_r_original`（R 包旧口径）
  以及 `control_mean_fixed`；生成脚本新增 R 侧独立参考实现 `deltaDeltaCtReference()`（可切 baseline），
  已核对 `baseline = "global"` 与旧 fixture 完全一致。

### 2. SE 公式核查（第⑥项）

- **C++ 无此 bug**：ΔΔCt 结果表只有 Mean/SD（无 SE 列）；RqPCR 路径的 SE 在 2026-10-15 第④项已确认是 `sd/√n`。
- **R 包确实写错**：`CalExp2ddCt` 的 `se.expression = mean.expression / sqrt(n.biorep)`（应为 `sd/√n`），
  已在 R 包侧改正（`CalExp2dCt` / `CalExpCurve` 本来就是 sd/√n）。

### 3. 稿件与零依赖复算脚本同步

- `manuscript/sections/results.tex`：ΔΔCt 段数字改为 1.92 / 2.74，并改成"按 Livak & Schmittgen 以对照组为
  校准样品，故对照组 ≈ 1（1.01 ± 0.21）"，删除"按全样本均值归一化、对照组不强制为 1"的旧表述
  （原表述与公式（2）自相矛盾，正是导师指出的问题）。
- `code/validation/check_example_stats.R`：归一化改为对照组基准，并把实测值更新为
  1.0145±0.2098 / 1.9169±0.2060 / 2.7403±0.5580（p 仍为 0.006027 / 0.022191）。
  沙箱实跑复核通过（`/tmp/qpcr_stats_check`，项目内已提交的 CSV **未覆盖**）。

### 4. R 包（`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools/CRAN/qPCRtools/`）同步修补（未提交）

工作区里已有一份半成品改动（ΔΔCt 只加了 `df.sub.ck` 过滤却没换用），本轮补完并修掉其余各条：

- `R/CalExp2ddCt.R`：① 基准 ΔCt 真正改用 `df.sub.ck`（对照组）；② `se.expression` 改为 `sd/sqrt(n)`；
  ③ `cld(tuk, level = 0.95, ddecreasing = TRUE)` → `cld(tuk, level = 0.05, decreasing = TRUE)`。
- `R/CalExpCurve.R`、`R/CalExpRqPCR.R`：同样修正 `cld` 的 level（0.95 → 0.05）与 `decreasing` 拼写。
  **实测依据**：本机 multcomp 里 `cld.glht(object, level = 0.05, decreasing = FALSE)` 的 `level` 是
  **显著性水平**；用三组无差异数据验证：`cld(tk, level = 0.95)` 给出 a,a,b（假阳性），
  `cld(tk)` 给 a,a,a ✓。
- `R/CalExpRqPCR.R`：④ 组内汇总 `mean(unique(expression))` → `mean(expression)`（含 `sd`、`se = sd/√n`）；
  ⑤ geNorm 稳定性改用相对量 `Q = 2^(minCq − Cq)`（用 `sweep()` 按列减最小值 —— 写 `data - apply(data,2,min)`
  会因 R 列主序回收错位，本轮踩到并规避）；⑥ 删除自动选内参分支里那段**从未被使用**的 M 计算（避免两套实现）；
  ⑦ `geometric.mean` 从"自动选内参"分支提到函数开头（原写法在显式给 `ref.gene` 时报
  `没有 geometric.mean 这个函数`，属既有 bug）。
- **验证**（R 4.6.1 实跑）：ΔΔCt 对照组 ≈ 1.01–1.32、SE = SD/√n ✓、换 `ref.group` 结果改变 ✓、
  字母标记 NS/* 合理 ✓；`find_ref_gene` 自动选内参由 OSPOX8/OsRBBI2（错）变为 **OsUBQ + OsRBBI2**，
  与 C++ 一致 ✓；RqPCR 组均值与 C++ 修正口径一致到 **3.9e-11**（差异来自 fixture 存储精度）✓。
- 未提交原因：该工作区含上一批未提交改动（导师补丁的半成品 + 行尾差异），建议用户先确认后再提交/发布 CRAN。

### 5. 待办

- ⑦ 图 2A 重画（log10 横轴配 log4 斜率）+ 公式（1）口径统一；图 2B 也需按新 ΔΔCt 数值重画。
- `validation/results/` 全套重跑（R 侧需先提交修正后的 R 包）、`comparison.csv`、论文 Table 1/Figure 3 数字。
- R 包侧仍未做的：GSL 缺失时的占位 p 值（第1条）、`CalExpCurve` 技术重复层级（第5条）。

## 2026-10-15 14:32 — 第④项：RqPCR 组内汇总改为按样本身份去重（不再按数值去重）；并核实"按 BioRep 编号配对"来自原始方法

导师意见第 8 条（"RqPCR 按数值去重，会把独立的生物学重复合并掉"）。两处都查了原始文献，
结论比原先的转述更精确 —— 上半是真 bug，下半是**方法本身的设计前提**、需要在文档里讲清楚。

### 文献核实（两份 PDF 均已获取正文，非凭记忆）

- **Hilliou & Tran (2013)**《RqPCRAnalysis: Analysis of Quantitative Real-time PCR Data》，
  BIOINFORMATICS 2013, 202–211，DOI `10.5220/0004312002020211`（SciTePress 官方 PDF 可下载）。
  这是 RqPCR 方法的原始出处（SATQPCR 与 R 包 `CalExpRqPCR` 的 §2 都引用它）。
  - §2.2.2：**"For each biological replicate i, the average of Cq is computed … the relative quantity …
    where the highest expression level set to one"** —— 即相对量是在**每个生物学重复内部**跨处理标定的
    （最高表达 = 1）。R 包与 C++ 现有的 `min.mean.cq`（同一 BioRep 内跨 Group 取最小平均 Cq）**忠实实现了原文**。
  - §2.2.4：对生物学重复求均值 / 标准差，全文**没有**任何"按数值去重"的写法。
  - §2.3：统计比较是"all pairs of samples across biological replicates"（跨生物学重复的样本两两比较）。
- **Rancurel et al. (2019)**《SATQPCR: Website for statistical analysis of real-time quantitative PCR data》，
  *Molecular and Cellular Probes* 46:101418，DOI `10.1016/j.mcp.2019.07.001`（HAL 开放版
  `hal-02264348`）。§3.5：默认"for each gene, your data are rescaled using **the sample with the lowest
  expression**"，并允许用户指定用于重标定的样品；§3.2 要求"至少 3 个生物学重复、2 个技术重复…每个生物学重复
  行数相同（平衡设计）"。

### 1.（真 bug，已修）组内汇总不再按数值去重

- 现象：R 包与 C++ 都用 `mean(unique(expression))` / `sd(unique(expression))` / `se = sd/√unique(biorep)`。
  两个生物学重复表达量碰巧相同（1、1、4）会被合并，均值由 2.0 变成 2.5 —— 与原始方法论文不符
  （§2.2.4 只要求对生物学重复求均值）。
- 改法（`ExpressionCalculatorStandardCurve.cpp`）：
  ① 行级按**样本身份** `Group|BioRep|TechRep|Gene|Cq` 去重（数据整理产生的完全重复行只计一次；
     数值不同则视为两次独立测量并全部保留）；
  ② 组内 `mean`/`sd` 对**全部**生物学重复计算，`se = sd/√n_biorep`。
- **实测影响**（示例数据 `examples/rqpcr_*.csv`，6 基因 × 16 样品）：4 个基因的组均值改变，最大 ~10%
  （`OsWAK91` 3.6824 → 3.3319；`OsCeBip` 1.2854 → 1.0912；`OSPOX8` 1.5406 → 1.3765；
  `OsRBBI2` 1.2964 → 1.2515）；`OsPR10` 不变。修复后**最低组发生切换**，因此该基因的归一化基准也随之变化。
  组间 p 值不变（统计检验用的是逐生物学重复的表达量）。
- **fixture 与参考实现**：`generate_reference.R` 新增 `rqpcrReference()`（R 侧独立实现，不依赖 qPCRtools 包），
  可分别按"R 包口径"与"修正口径"出值；已核对 R 包口径与旧 fixture **完全一致（~1e-12）**，
  说明参考实现忠实复现了 R 包。新 fixture 同时保留两套值：`by_gene`（修正）/ `by_gene_r_original`（R 包）。
  附带收益：RqPCR fixture 现在**不再需要安装 qPCRtools 包**即可重生成。
- **测试**：新增 `ExpressionCalculator.RqPCRGroupMeanKeepsIdenticalReplicates`
  （导师原例：Treatment 组重复表达量 0.5、0.5、1.0 → 正确均值 2/3，归一化后 CK = 1.5；旧行为给 1.3333，
  已断言两者不同）与 `ExpressionCalculator.RqPCRGroupMeansDeviateFromRPackage`（记录 ~10% 的有意偏离）。
  `ctest` **51/51 通过**。

### 2.（非 bug，需讲清设计前提）"按 BioRep 编号跨组取最小 Cq"是原始方法的规定

- 结论：这一条**不是**移植错误 —— Hilliou & Tran 2013 §2.2.2 明确要求在**每个生物学重复内部**把最高表达
  设为 1。它对应的实验设计是**按生物学重复配对的区组设计**：编号相同的 BioRep 在各处理组里应是同一份
  生物学材料（原文的例子里，"重复 1/2/3"是三次独立实验、每次都覆盖全部处理）。
- 风险：如果用户的数据里各组样品相互独立、BioRep 只是各组从 1 开始编号，这个标定会引入不存在的配对结构。
  SATQPCR 网页版默认用"表达量最低的样品"重标定、并允许用户指定样品，属于更保守的做法。
- 处置（本轮只记录、不改行为）：在 `docs/08_status.md` 写明该前提与风险；**是否需要给 GUI 增加
  "标定样品/是否按 BioRep 配对"的选项，待用户按实际实验设计确认**（改动会再次改变所有 RqPCR 数值）。

### 3. 待用户决策（论文与 R 包层面，本轮未动）

- 修正 1 使 C++ 与 R 包 `CalExpRqPCR` 的组均值产生 ~10% 的**有意差异**，因此
  `validation/results/comparison.csv` 的 RqPCR 一侧（原记录相对差 ~2e-15）与论文 Table 1/Figure 3
  的对应数字**必须重做**。两条路：① 同步修补 R 包（`CalExpRqPCR` 的 `mean(unique(...))` → 全部重复），
  再重跑 R 侧与对比；② 保留 R 包原状，在论文里写明这一处有意偏离并改用"逐生物学重复表达量一致
  （仍 ~1e-15）"作为等价性证据。建议走 ①（导师已指出 R 包同一处也错）。
- 本轮**未覆盖** `validation/results/` 下的任何文件（C++ 侧重跑产物已变化，见上）。

## 2026-10-15 14:18 — 第③项：geNorm 参考基因筛选改用相对量 + 逐步剔除

导师意见第 7 条（"geNorm 内参筛选的输入用错了，R 包和 C++ 版都有"）。核对 R 包源码后确认
他说的两点都成立，且比原记录更精确。

### 事实核对（R 包源码 `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools/R/CalExpRqPCR.R`）

- R 包 `find_ref_gene()`（第 224-273 行）**有**逐步剔除：`for (i in n.genes:num.ref)` 每轮
  `which.max(M)` 剔除 M 最大的基因，直到剩 2 个。→ C++ 原实现只算一轮 M 取最小的两个，**漏了这一步**。
- 但 R 包的稳定性度量是 `A <- log2(data[, j] / data[, -j])`，即**原始 Cq 相除**，不是相对量。
  → 两个版本在这一点上都与 geNorm 原文（Vandesompele et al. 2002：`Q = 2^(minCq − Cq)`）不符。

### 改动

- `ExpressionCalculator::selectReferenceGenesByGeNorm`（`src/Core/ExpressionCalculator.cpp`）：
  ① 先换算成相对量的对数 `log2 Q = minCq − Cq`（每个基因相对自身表达最高的样品）；
  ② M = 与其余基因两两 `log2(Q_a/Q_b)` 的 SD 的平均（效率按 2 时等价于"两个基因 Cq 差值在样品间的波动"）；
  ③ **逐步剔除**：每轮剔除 M 最大者，重复到只剩 2 个；并列时按输入顺序取第一个，保证结果确定。
- 缺值样品不参与涉及它的基因对；某基因可用配对样品不足 2 个时 M 记 +∞（最先被剔除）。
- 函数由 private 改 public（与 `performANOVA` 同样"公开以便测试与第三方调用"），头文件补上算法定义与
  与 R 包口径差异的说明。
- `docs/08_status.md`：R 函数映射表里 GeNorm 一行由"⏳ 部分"改为"✅ 已实现（按 geNorm 原文）"，
  并写明与 R 包的差异；bug 列表补一条。

### 测试与证据

- `generate_reference.R` 新增 `geNormSelect()`（R 侧独立参考实现，四种组合可选）与两套数据：
  - `tests/fixtures/data_genorm.csv`：**判别数据集**（5 基因 × 5 样品，数值刻意挑成能区分两种错误）。
    正确做法（相对量 + 逐步）选 **G1/G3**；原始 Cq 相除、或只算一轮 M，都会选成 **G1/G5**。
  - `expected_stat.json` 新增 `genorm`（判别数据集）与 `genorm_example`（示例数据）。
- 新增两个用例：`ExpressionCalculator.GeNormSelectsReferenceGenes`（含"若退化成旧行为必失败"的断言）、
  `ExpressionCalculator.GeNormMatchesReferenceOnExampleData`（96 孔示例数据，6 基因 × 16 样品）。
- **真实数据结论**：论文示例数据上四种组合选出的内参集合**相同**（`OsRBBI2` / `OsUBQ`，也正是
  validation 脚本手选的那一对），所以论文里基于示例数据的结果不受本次改正影响。
- `ctest` **49/49 通过**（47 → +2）；GUI 与等价性 driver 均编译通过；
  validation 三份输出连跑两次仍逐字节一致，且与上一轮完全相同（driver 显式指定内参，故不受影响）。
- 上一批次（Tukey + 输出顺序确定化，提交 `4e77e90`）的 CI run **38690903815 成功**（macOS，5m42s）。

### 遗留（下一步）

④ RqPCR 按数值去重 / 配对问题（导师意见第 8 条）；⑤ ΔΔCt 基准决策（需用户拍板）；⑥ SE 公式；
⑦ 图 2A 重画 + 公式口径。另有：论文 `paper.md` 把 geNorm 自动选内参写成"相对 R 版的扩展"，
而 R 包其实有 `find_ref_gene()`（口径不同）——属于导师提的"引言竞品描述要改准"一类，待改稿时一并处理。

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
