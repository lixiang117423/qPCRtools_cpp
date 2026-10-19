# Cover letter — Bioinformatics (Application Note)

**To:** The Editors, *Bioinformatics*
**Article type:** Application Note
**Title:** qPCRtools: a cross-platform C++ desktop application for qPCR data analysis with validated equivalence to its R implementation

Dear Editors,

Please consider the enclosed Application Note for publication in *Bioinformatics*.

Quantitative real-time PCR is one of the most widely used techniques in molecular biology, yet the step between the instrument and a defensible result — standard-curve and efficiency estimation, choice of quantification method, group statistics and figures — is still handled either in R (including with our own package *qPCRtools*, *Frontiers in Genetics* 2022) or with instrument-vendor software that ties the analysis to one platform. The users who most need a reproducible analysis are frequently those least able to set up an R environment: newly enrolled students, visiting researchers and wet-lab staff without programming experience.

We present qPCRtools 1.2.0, a native C++17/Qt6 desktop application that re-implements the analytical core of the R package and wraps it in a graphical workflow — import, configuration, analysis, export — with in-interface figures, quality-control warnings and a plain-text record of every analysis. It runs as a single executable with no R, Python or other runtime dependency, and pre-built Windows and macOS binaries are provided.

The central claim of the Application Note is validated equivalence. On identical inputs the application and the R package agree to floating-point precision for every shared quantification method (ΔΔCt, standard curve and the efficiency-based RqPCR method), and the statistical layer — group means and standard deviations, Welch *p*-values, one-way ANOVA and Tukey letter groupings — is additionally reproduced by a zero-dependency base-R script shipped with the software, so that the reported numbers can be checked without installing anything. Preparing the port also revealed and allowed us to correct several defects shared with the R implementation (the ΔΔCt calibrator, replicate handling and the input of the reference-gene stability measure); each correction is covered by a regression test, and 52 C++ tests plus the front-end smoke tests run in continuous integration.

All source code, example data, validation scripts and tests are available under the MIT licence at <https://github.com/lixiang117423/qPCRtools_cpp>; version 1.2.0 with pre-built binaries is archived at <https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.2.0>. Users do not need to register, and we will keep the software available for at least two years after publication.

The manuscript is original, is not under consideration elsewhere, and its relationship to our 2022 R-package paper — the same algorithms, a new delivery mode — is stated explicitly in the text. The authors declare no competing interests. In line with the journal's policy on AI-assisted tools, the manuscript includes a disclosure of how AI coding assistants were used.

Yours sincerely,

Xiang Li (on behalf of both authors)
State Key Laboratory for Conservation and Utilization of Bio-Resources in Yunnan, Yunnan Agricultural University, Kunming, China
(present address: Yazhouwan National Laboratory, Sanya, Hainan, China)
lixiang117423@gmail.com
