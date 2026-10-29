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
OUT = ROOT / "manuscript" / "cover_letter_v7_20261029.docx"

MD_OUT = ROOT / "manuscript" / "cover_letter_v7_20261029.md"

SUBJECT = (
    "To: The Editors, Bioinformatics\n"
    "Article type: Application Note\n"
    "Title: qPCRtools: a cross-platform desktop application for reproducible "
    "qPCR analysis without programming"
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
    "We present qPCRtools 1.3.1, a native C++17/Qt6 desktop application that re-implements "
    "the analytical core of the R package and wraps it in a graphical workflow \u2014 import, "
    "configuration, analysis, export \u2014 with in-interface figures, quality-control warnings "
    "and a plain-text record of every analysis. It runs as a single executable with no R, "
    "Python or other runtime dependency. Version 1.3.1, including the source tag and "
    "Windows/macOS binaries, is publicly released.",
    "The central claim of the Application Note is reproducible, auditable analysis. We compared "
    "the application with an independent base-R implementation for all three quantification "
    "methods (Delta-Delta Ct, standard curve and the efficiency-based RqPCR method). Across "
    "58 matched quantities the maximum relative difference was 1.9e-14. Welch and rank-sum "
    "P values for several groups against one control are adjusted by Holm's method within "
    "each gene, and the unadjusted values remain in the exported table. Preparing the port "
    "also revealed and allowed us to correct several defects in result aggregation. Each "
    "correction is covered by a regression test, and 53 C++ tests plus the front-end smoke "
    "tests run in continuous integration. We also completed a reproducible reanalysis of "
    "the real example plates distributed with the 2022 qPCRtools paper, from standard-curve "
    "quality control through method selection, group analysis and exported analysis record.",
    "All source code, example data, validation scripts and tests are available under the MIT "
    "licence at https://github.com/lixiang117423/qPCRtools_cpp. Users do not need to "
    "register, and we will keep the software available for at least two years after publication.",
    "The manuscript is original and is not under consideration elsewhere. Its relationship to "
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

    doc.core_properties.title = "Cover letter - qPCRtools Application Note"
    doc.core_properties.author = "Xiang Li"
    doc.save(OUT)
    md_lines = [
        "# Cover letter - Bioinformatics (Application Note)",
        "",
        "**To:** The Editors, *Bioinformatics*",
        "**Article type:** Application Note",
        "**Title:** "
        + SUBJECT.split("Title: ", 1)[1].replace("\n", " "),
        "",
    ]
    for text in BODY:
        md_lines.append(text)
        md_lines.append("")
    md_lines.extend(SIGNATURE)
    md_lines.append("")
    MD_OUT.write_text("\n".join(md_lines), encoding="utf-8")
    print("written:", OUT)
    print("written:", MD_OUT)


if __name__ == "__main__":
    main()
