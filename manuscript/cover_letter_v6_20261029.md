# Cover letter - Bioinformatics (Application Note)

**To:** The Editors, *Bioinformatics*
**Article type:** Application Note
**Title:** qPCRtools: a cross-platform desktop application for reproducible qPCR analysis without programming

Dear Editors,

Please consider the enclosed Application Note for publication in Bioinformatics.

Quantitative real-time PCR is one of the most widely used techniques in molecular biology, yet the step between the instrument and a defensible result — standard-curve and efficiency estimation, choice of quantification method, group statistics and figures — is still handled either in R (including with our own package qPCRtools, Frontiers in Genetics 2022) or with instrument-vendor software that ties the analysis to one platform. The users who most need a reproducible analysis are frequently those least able to set up an R environment: newly enrolled students, visiting researchers and wet-lab staff without programming experience.

We present qPCRtools 1.3.0, a native C++17/Qt6 desktop application that re-implements the analytical core of the R package and wraps it in a graphical workflow — import, configuration, analysis, export — with in-interface figures, quality-control warnings and a plain-text record of every analysis. It runs as a single executable with no R, Python or other runtime dependency. Version 1.3.0, including the source tag and Windows/macOS binaries, is publicly released.

The central claim of the Application Note is reproducible, auditable analysis. We compared the application with an independent base-R implementation for all three quantification methods (Delta-Delta Ct, standard curve and the efficiency-based RqPCR method). Across 58 matched quantities the maximum relative difference was 1.9e-14. Welch and rank-sum P values for several groups against one control are adjusted by Holm's method within each gene, and the unadjusted values remain in the exported table. Preparing the port also revealed and allowed us to correct several defects in result aggregation. Each correction is covered by a regression test, and 53 C++ tests plus the front-end smoke tests run in continuous integration. We also completed a reproducible reanalysis of the real example plates distributed with the 2022 qPCRtools paper, from standard-curve quality control through method selection, group analysis and exported analysis record.

All source code, example data, validation scripts and tests are available under the MIT licence at https://github.com/lixiang117423/qPCRtools_cpp. Users do not need to register, and we will keep the software available for at least two years after publication.

The manuscript is original and is not under consideration elsewhere. Its relationship to our 2022 R-package paper — the same algorithms, a new delivery mode — is stated explicitly in the text. The authors declare no competing interests. In line with the journal's policy on AI-assisted tools, the manuscript includes a disclosure of how AI coding assistants were used.

Yours sincerely,

Xiang Li (on behalf of both authors)
State Key Laboratory for Conservation and Utilization of Bio-Resources in Yunnan, Yunnan Agricultural University, Kunming, China
lixiang117423@gmail.com
