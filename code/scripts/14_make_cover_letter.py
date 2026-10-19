#!/usr/bin/env python3
"""生成投稿用 cover letter 的 Word 版（manuscript/cover_letter_<date>.docx）。

为什么用脚本而不是手工排版：
  - 全局规则要求投稿材料（含 cover letter）同时给 Word 版；
  - 内容改动后可以一条命令重出，避免"上次那份和这版内容对不上"。

排版口径：正文 Arial、11 pt、两端对齐；页边距 2.5 cm；信头为作者与单位。
内容以 manuscript/cover_letter_<date>.md 为准（本脚本内嵌同一份文本，
改内容时两处一起改，或只改本脚本后重新生成）。

用法（在项目根目录）：
    python3 code/scripts/14_make_cover_letter.py
"""

from pathlib import Path

from docx import Document
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.shared import Cm, Pt

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "manuscript" / "cover_letter_20261015.docx"

SUBJECT = (
    "To: The Editors, Bioinformatics\n"
    "Article type: Application Note\n"
    "Title: qPCRtools: a cross-platform C++ desktop application for qPCR data "
    "analysis with validated equivalence to its R implementation"
)

BODY = [
    "Dear Editors,",
    "Please consider the enclosed Application Note for publication in Bioinformatics.",
    "Quantitative real-time PCR is one of the most widely used techniques in molecular "
    "biology, yet the step between the instrument and a defensible result \u2014 standard-curve "
    "and efficiency estimation, choice of quantification method, group statistics and "
    "figures \u2014 is still handled either in R (including with our own package qPCRtools, "
    "Frontiers in Genetics 2022) or with instrument-vendor software that ties the analysis "
    "to one platform. The users who most need a reproducible analysis are frequently those "
    "least able to set up an R environment: newly enrolled students, visiting researchers "
    "and wet-lab staff without programming experience.",
    "We present qPCRtools 1.2.0, a native C++17/Qt6 desktop application that re-implements "
    "the analytical core of the R package and wraps it in a graphical workflow \u2014 import, "
    "configuration, analysis, export \u2014 with in-interface figures, quality-control warnings "
    "and a plain-text record of every analysis. It runs as a single executable with no R, "
    "Python or other runtime dependency, and pre-built Windows and macOS binaries are provided.",
    "The central claim of the Application Note is validated equivalence. On identical inputs "
    "the application and the R package agree to floating-point precision for every shared "
    "quantification method (Delta-Delta Ct, standard curve and the efficiency-based RqPCR "
    "method), and the statistical layer \u2014 group means and standard deviations, Welch "
    "p-values, one-way ANOVA and Tukey letter groupings \u2014 is additionally reproduced by a "
    "zero-dependency base-R script shipped with the software, so that the reported numbers "
    "can be checked without installing anything. Preparing the port also revealed and allowed "
    "us to correct several defects shared with the R implementation (the Delta-Delta Ct "
    "calibrator, replicate handling and the input of the reference-gene stability measure); "
    "each correction is covered by a regression test, and 52 C++ tests plus the front-end "
    "smoke tests run in continuous integration.",
    "All source code, example data, validation scripts and tests are available under the MIT "
    "licence at https://github.com/lixiang117423/qPCRtools_cpp; version 1.2.0 with pre-built "
    "binaries is archived at "
    "https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.2.0. Users do not need to "
    "register, and we will keep the software available for at least two years after publication.",
    "The manuscript is original, is not under consideration elsewhere, and its relationship to "
    "our 2022 R-package paper \u2014 the same algorithms, a new delivery mode \u2014 is stated "
    "explicitly in the text. The authors declare no competing interests. In line with the "
    "journal's policy on AI-assisted tools, the manuscript includes a disclosure of how AI "
    "coding assistants were used.",
    "Yours sincerely,",
]

# 信尾逐行单列（不要塞进一个段落里用软换行——Word/LibreOffice 会把它拆得很难看）
SIGNATURE = [
    "Xiang Li (on behalf of both authors)",
    "State Key Laboratory for Conservation and Utilization of Bio-Resources in Yunnan, "
    "Yunnan Agricultural University, Kunming, China",
    "lixiang117423@gmail.com",
]


def main() -> None:
    doc = Document()
    style = doc.styles["Normal"]
    style.font.name = "Arial"
    style.font.size = Pt(10.5)
    for section in doc.sections:
        section.page_width = Cm(21.0)    # A4
        section.page_height = Cm(29.7)
        section.top_margin = section.bottom_margin = Cm(2.5)
        section.left_margin = section.right_margin = Cm(2.5)

    for i, line in enumerate(SUBJECT.split("\n")):
        p = doc.add_paragraph()
        p.paragraph_format.space_after = Pt(0 if i < 2 else 12)
        run = p.add_run(line)
        run.bold = line.startswith(("To:", "Article type:", "Title:"))
        p.alignment = WD_ALIGN_PARAGRAPH.LEFT

    for text in BODY:
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
        p.paragraph_format.space_after = Pt(6)
        p.add_run(text)

    for i, line in enumerate(SIGNATURE):
        p = doc.add_paragraph()
        p.alignment = WD_ALIGN_PARAGRAPH.LEFT
        p.paragraph_format.space_after = Pt(0)
        if i == 0:
            p.paragraph_format.space_before = Pt(8)
        p.add_run(line)

    doc.save(OUT)
    print("written:", OUT)


if __name__ == "__main__":
    main()
