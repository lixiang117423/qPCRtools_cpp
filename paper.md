---
title: 'qPCRtools: a cross-platform desktop application for qPCR data analysis without a programming environment'
tags:
  - C++
  - Qt
  - qPCR
  - gene expression
  - reproducible analysis
authors:
  - name: Xiang Li
    affiliation: "1, 2"
  - name: Huichuan Huang
    affiliation: "1, 2"
affiliations:
  - name: State Key Laboratory for Conservation and Utilization of Bio-Resources in Yunnan, Yunnan Agricultural University, Kunming, China
    index: 1
  - name: Key Laboratory for Agro-Biodiversity and Pest Control of Ministry of Education, Yunnan Agricultural University, Kunming, China
    index: 2
date: 13 October 2026
bibliography: paper.bib
---

# Summary

Quantitative real-time PCR (qPCR) is a routine technique in molecular
biology laboratories: it is used to measure how strongly a gene is expressed,
to detect pathogens, and to genotype samples. Between the instrument and a
usable result, however, sits a chain of analysis steps that is easy to get
wrong: fitting a standard curve and estimating primer amplification
efficiency, choosing between the relative-quantification methods, normalising
against a reference gene, comparing treatment groups statistically, and
producing a figure for a manuscript.

\textit{qPCRtools} is a desktop application that performs this whole chain
through a graphical interface. The user loads the $C_q$ values exported by
the instrument (CSV, or Excel workbooks when the optional dependency is
compiled in) together with a table describing the experimental design,
previews both tables in the window, chooses a quantification method
($\Delta C_t$, $\Delta\Delta C_t$, or an efficiency-based method), sets the
reference gene, the control group and the statistical test, and receives
result tables with Tukey letters that can be exported as CSV or Excel. The
application is a native C++17/Qt6 program distributed as a single executable
for Windows and macOS and requires no R, Python, or any other runtime
environment; its interface is bilingual (English and Chinese). The analytical
core is a separate library that is exercised by a unit-test suite, and the
same core is driven by a command-line harness that re-runs the original R
implementation on identical inputs so that the two can be compared
automatically.

# Statement of need

Several mature tools already implement qPCR analysis correctly, but all of
them assume something about the user. R packages such as \textit{qPCRtools}
@li2022qpcrtools, \textit{pcr} @ahmed2018pcr, \textit{RQdeltaCT}
@zalewski2025rqdeltct, \textit{rtpcr} @mirzaghaderi2025rtpcr and the \textit{qDATA}
application @ionascu2024qdata require an R installation and, in most cases, at
least a minimal amount of programming. Commercial instrument software
(QuantStudio, CFX Maestro, REST, qbase+) provides a graphical interface but is
expensive, closed-source, and often tied to one operating system or one
instrument. Web-based analysis services avoid installation but are often
discontinued, and they require uploading experimental data to a third-party
server, which many laboratories will not do for unpublished results
@pabinger2014survey.

The users who fall through this gap are the ones with the least support:
newly enrolled students analysing their first qPCR plate, visiting
researchers, and wet-lab staff whose projects last a single season. In our own
teaching and collaborative work, such users either resort to manual
spreadsheet calculations — which are difficult to audit and easy to get wrong
— or to whatever instrument software happens to be installed. The purpose of
this software is to remove the installation and programming requirement
without changing the analysis: the intended user should be able to go from an
exported $C_q$ table to a defensible result table and figure by
double-clicking an executable.

# State of the field

The starting point of this project is our own R package \textit{qPCRtools}
@li2022qpcrtools, which implements the standard-curve, $\Delta C_t$,
$\Delta\Delta C_t$ and efficiency-based methods together with t-test and
ANOVA/Tukey testing. That package is established in the field — at the time
of writing it has more than thirty citing works, including independent qPCR
tooling papers @zalewski2025rqdeltct @mirzaghaderi2025rtpcr @ionascu2024qdata — but its
reach is bounded by the R environment. The alternative we considered was to
keep the analysis in R and to lower the entry barrier elsewhere, for example
by packaging a bundled R runtime or by wrapping the package in a local web
application. We rejected the first because it does not remove the dependency,
only hides it, and the second because it still requires the user to start and
keep a server process alive.

Rather than reimplementing the statistics from scratch and hoping that the
results coincide, we ported the algorithms to C++ and made equivalence to the
R implementation a testable property of the build: the same input files are
fed to both implementations, every shared quantity is compared pairwise, and
the comparison is regenerated from open scripts (see *Software design*).
Relative to the R version the application also extends the statistics
(Wilcoxon tests, one-way ANOVA with Tukey letter grouping, Cohen's $d$, IQR
outlier removal, and automatic reference-gene selection by the geNorm
algorithm @vandesompele2002accurate), and it adds an interactive, offline
interface. Both implementations are usable side by side: users who work in R
can continue to use the package, and users who cannot are no longer excluded
from the same analysis.

# Software design

The application is written in C++17 on top of Qt6. Three deliberate
trade-offs shaped the design.

*A tested core library, decoupled from the interface.* All numerical work
lives in a static library (`qpcr_core`) that depends only on Qt Core, Eigen
and, optionally, GSL. The graphical interface is a Qt WebEngine view that
renders HTML/CSS/JavaScript and talks to the library through Qt WebChannel. We
chose a web-based front end inside a native shell because it allows the
interface to be iterated quickly (tables, forms, and a figure view that is
bundled but disabled in the current release) while keeping every computation
in compiled code, and because it keeps the
core independent of the GUI: the core is linked directly by the unit tests
and by the validation driver, and it can be reused by other front ends.

*Equivalence as a build-time property, not a claim.* The validation harness
consists of an R script, a small C++ driver linked against `qpcr_core`, and a
comparison script. Both sides read the same on-disk files, so any difference
reflects the implementations alone rather than differences in the input data.
Running it regenerates the comparison table and the figures shipped with the
manuscript, which means the claim "this application reproduces the R results"
can be re-checked by a reviewer in a few minutes instead of being taken on
trust.

*Offline and self-contained by default.* A native executable was preferred
over a server-based service because qPCR data are frequently unpublished, and
because a laboratory that cannot reach the internet should still be able to
analyse a plate. The price is that the software must be built and packaged per
platform; we accept that cost and provide pre-built Windows and macOS
binaries, with the source compiling on Linux.

The scope of the port is stated explicitly in the documentation and in the
manuscript: two functions of the R version — the standard-curve
slope–intercept quantification and the reverse-transcription reagent volume
calculation — are not yet ported, and two tests that exist in the core
library are not yet exposed in the interface.

# Research impact statement

The analytical approach implemented here is already used in published
research: the R implementation on which this application is based has more
than thirty citing works, including independent qPCR software papers
@zalewski2025rqdeltct @mirzaghaderi2025rtpcr @ionascu2024qdata, and the numerical
behaviour of the two implementations has been verified to agree (see
*Software design*).

For the desktop application itself, the realized impact is currently confined
to its developers and their immediate environment: it is used by the authors
for routine qPCR analysis and in teaching, and the interface screenshots in
the accompanying manuscript were captured from the running application rather
than reconstructed. [[待你补充：请填具体、可核实的自用/教学证据，例如"2026 年 X 月起用于本人课题组 N 项分析"、"在某课程/培训中由 N 名学生使用，用于 X"、"某合作实验室已装机使用"等。JOSS 明确要求"软件已在被用于研究"的证据，只有"将来会被使用"的说法会被直接拒稿。]]
We do not yet claim external adopters beyond this, and the desktop
application is a young project (first public release March 2026); the
significance we claim now is therefore the combination of a validated
analytical core, a feature-complete and tested release, and the fact that the
usage it targets — students and laboratories without an R environment — is
one that the R implementation cannot reach at all.

# AI usage disclosure

Generative AI tools were used in the development of this software and in the
preparation of this paper, and their use is disclosed here in full.

*Tools used.* OpenAI Codex (GPT-5 family models), in the Codex desktop
application and command-line interface, between June and October 2026.

*Where and how they were used.* (i) Code: refactoring of the statistical
core, implementation of several bug fixes in the t-test, Wilcoxon and ANOVA
paths, scaffolding of unit tests, and the continuous-integration
configuration. (ii) Documentation: drafting and editing of README and
in-project documentation. (iii) Paper text: drafting and language editing of
the manuscript and of this paper.

*Human verification.* The authors reviewed and edited every AI-assisted change
before it was committed; ran the application manually to produce the
interface figures; executed the R-versus-C++ equivalence validation and
checked the resulting numbers against the R output; and made the core design
decisions — the core/interface split, the choice of a native Qt build, the
scope of the port, and the decision to make equivalence an automatically
reproducible property. No AI tool was used in correspondence with editors or
reviewers, and no AI system is credited as an author.

[[待你确认：上面这段 AI 披露必须由你核对后才可提交。JOSS 要求写明"工具与版本、用在哪里、如何人工核验"，不完整或不准确会被视为学术不端。请确认（a）实际使用的模型与时间范围、（b）是否还有本文件未列出的 AI 用途（例如界面截图、图件生成）、（c）是否同意"人工复核"的表述与你实际情况一致。]]

# Acknowledgements

The authors thank the developers of the R packages and libraries on which
this work builds, and the users who provided feedback during development.
Financial support is acknowledged in the corresponding manuscript. X.L.'s
present address is Yazhouwan National Laboratory, Sanya, Hainan, China.

# References
