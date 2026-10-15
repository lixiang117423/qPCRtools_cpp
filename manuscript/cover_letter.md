# Cover letter — Bioinformatics (Application Note)

> 用途：投稿时粘贴到投稿系统的 cover letter 栏，或作为 PDF 上传。
> 需你填写/确认的地方用 `[[ ]]` 标出。

---

**To:** The Editors, *Bioinformatics*
**Article type:** Application Note
**Title:** qPCRtools: a cross-platform C++ desktop application for qPCR data
analysis with validated equivalence to its R implementation
**Authors:** Xiang Li (first and corresponding author) and Huichuan Huang
(co-corresponding author), State Key Laboratory for Conservation and
Utilization of Bio-Resources in Yunnan, and Key Laboratory for
Agro-Biodiversity and Pest Control of Ministry of Education, Yunnan
Agricultural University, Kunming, China.
**Corresponding e-mails:** lixiang117423@gmail.com (X.L.),
absklhhc@gmail.com (H.H.)

Dear Editors,

Please consider the enclosed Application Note for publication in
*Bioinformatics*.

Quantitative PCR is one of the most widely used techniques in molecular
biology, but the step between the instrument and a publication-ready result —
efficiency estimation, choice of quantification method, group statistics, and
figures — is still handled either in R (including by our own package
*qPCRtools*, Frontiers in Genetics 2022, cited more than 30 times) or by
costly, platform-restricted commercial software. The users who most need a
defensible analysis are frequently those least able to set up an R
environment: newly enrolled students, visiting researchers, and wet-lab staff
without programming experience.

This Application Note addresses exactly that gap. We reimplemented the
analytical core of the R package as a native C++17/Qt6 desktop application
that runs as a single executable with no R, Python, or other runtime
requirement, and we validated it against the R implementation on identical
inputs: for every shared quantification method the compared quantities agree
to floating-point precision, and the validation is reproducible from scripts
shipped with the software. Compared with the R version, the application
additionally offers non-parametric testing, one-way ANOVA with Tukey letter
grouping, effect sizes, IQR-based outlier removal, and automatic
reference-gene selection. We believe this fits the Application Note format
well: the contribution is a validated, reusable tool rather than a new
statistical method, and the software is freely available under the MIT
license with source, tests, and documentation.

The manuscript is original, is not under consideration elsewhere, and has no
overlap with the 2022 R-package paper beyond the shared algorithms, which are
stated explicitly. All authors (single author) approved the submission and
declare no competing interests. The software, the example datasets, and the
equivalence-validation package are publicly available at
https://github.com/lixiang117423/qPCRtools_cpp.

[[Associate Editor nomination: 按 Bioinformatics 投稿系统要求填写（如系统提示需要）]]

Yours sincerely,

Xiang Li (on behalf of both authors)
State Key Laboratory for Conservation and Utilization of Bio-Resources in
Yunnan, Yunnan Agricultural University, Kunming, China
(present address: Yazhouwan National Laboratory, Sanya, Hainan, China)
lixiang117423@gmail.com
