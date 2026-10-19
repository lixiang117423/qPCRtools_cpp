#!/usr/bin/env python3
"""生成正文与补充材料的 Word 版（供导师/共同作者批注）。

为什么单独写脚本：
  - 全局规则要求投稿材料在 PDF 之外同时给 .docx；
  - 内容改了以后一条命令重出，避免 docx 与 PDF 对不上；
  - LaTeX 里的自定义标题块（\\parbox 作者与单位）pandoc 读不出来，
    必须由脚本补回去，否则 Word 版会缺题目、作者和单位。

做法：
  1) pandoc 读 main.tex / supplementary.tex（含 \\input 展开、公式转 OMML、
     图片嵌入、参考文献用 nlm-citation-sequence.csl 生成顺序编号）；
  2) python-docx 补标题块、统一 Arial 11 pt / 两端对齐、图片缩到版心宽、
     表格改成三线表。

用法（项目根目录）：
    /Users/lixiang/miniforge3/envs/python/bin/python3 code/scripts/15_make_manuscript_docx.py
"""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from docx import Document
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt

ROOT = Path(__file__).resolve().parents[2]
MS = ROOT / "manuscript"
CSL_CANDIDATES = [
    Path("/Users/lixiang/Zotero/styles/nlm-citation-sequence.csl"),
    Path("/Users/lixiang/Zotero/styles/nature.csl"),
]

TITLE = (
    "qPCRtools: a cross-platform C++ desktop application for qPCR data "
    "analysis with validated equivalence to its R implementation"
)
AFFILIATIONS = [
    "1 State Key Laboratory for Conservation and Utilization of Bio-Resources "
    "in Yunnan, Yunnan Agricultural University, Kunming, China",
    "2 Key Laboratory for Agro-Biodiversity and Pest Control of Ministry of "
    "Education, Yunnan Agricultural University, Kunming, China",
]
CORRESPONDING = (
    "* Corresponding authors: lixiang117423@gmail.com (X.L.), "
    "absklhhc@gmail.com (H.H.)"
)
PRESENT_ADDRESS = (
    "Present address (X.L.): Yazhouwan National Laboratory, Sanya, Hainan, China"
)


def sh(cmd: list[str], cwd: Path) -> None:
    subprocess.run(cmd, cwd=cwd, check=True)


def make_reference_docx(path: Path) -> None:
    """以 pandoc 默认模板为底，改成项目要求的排版口径。"""
    with path.open("wb") as fh:
        subprocess.run(
            ["pandoc", "--print-default-data-file", "reference.docx"],
            stdout=fh,
            check=True,
        )

    doc = Document(str(path))
    normal = doc.styles["Normal"]
    normal.font.name = "Arial"
    normal.font.size = Pt(11)
    rpr = normal.element.get_or_add_rPr()
    rfonts = rpr.find(qn("w:rFonts"))
    if rfonts is None:
        rfonts = OxmlElement("w:rFonts")
        rpr.append(rfonts)
    rfonts.set(qn("w:eastAsia"), "Arial")
    normal.paragraph_format.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
    normal.paragraph_format.line_spacing = 1.5
    normal.paragraph_format.space_after = Pt(6)

    for name, size in (("Heading 1", 14), ("Heading 2", 12), ("Heading 3", 11)):
        style = doc.styles[name]
        style.font.name = "Arial"
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = None

    for name in ("Title", "Subtitle", "Author", "Date"):
        try:
            style = doc.styles[name]
            style.font.name = "Arial"
            style.font.color.rgb = None
        except KeyError:
            continue

    doc.save(str(path))


def _clear_paragraph(p) -> None:
    for run in list(p.runs):
        run._element.getparent().remove(run._element)


def format_title_block(doc: Document, *, supplementary: bool) -> None:
    """pandoc 已把 \\title/\\author 写成了 Title/Author 段落，这里只重排格式。

    注意：不要另建标题段——否则会出现两份题目与作者（2026-10-19 踩过）。
    """
    title_p = next(
        (p for p in doc.paragraphs[:6] if p.style.name == "Title"), None
    )
    author_p = next(
        (p for p in doc.paragraphs[:6] if p.style.name == "Author"), None
    )
    if title_p is None or author_p is None:
        raise SystemExit("pandoc 输出里找不到 Title/Author 段落")

    title_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    title_p.paragraph_format.space_before = Pt(0)
    title_p.paragraph_format.space_after = Pt(8)
    _clear_paragraph(title_p)
    if supplementary:
        run = title_p.add_run("Supplementary material")
        run.bold = True
        run.font.size = Pt(15)
        run.font.name = "Arial"
        run.add_break()
        run = title_p.add_run(TITLE)
        run.italic = True
        run.font.size = Pt(11.5)
        run.font.name = "Arial"
    else:
        run = title_p.add_run(TITLE)
        run.bold = True
        run.font.size = Pt(14)
        run.font.name = "Arial"

    author_p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    author_p.paragraph_format.line_spacing = 1.0
    author_p.paragraph_format.space_after = Pt(12)
    _clear_paragraph(author_p)

    for text, kind in (
        ("Xiang Li", None),
        ("1,2,*", "sup"),
        ("    ", None),
        ("Huichuan Huang", None),
        ("1,2,*", "sup"),
    ):
        run = author_p.add_run(text)
        run.font.name = "Arial"
        run.font.size = Pt(11)
        if kind == "sup":
            run.font.superscript = True

    for text in (*AFFILIATIONS, CORRESPONDING, PRESENT_ADDRESS):
        author_p.add_run().add_break()
        run = author_p.add_run(text)
        run.font.name = "Arial"
        run.font.size = Pt(9.5)


def set_table_borders(table) -> None:
    """三线表：顶线、表头下线、表尾底线，其余无框线。"""
    tbl = table._tbl
    tblPr = tbl.tblPr
    borders = OxmlElement("w:tblBorders")
    for edge in ("top", "left", "bottom", "right", "insideH", "insideV"):
        el = OxmlElement(f"w:{edge}")
        el.set(qn("w:val"), "nil")
        borders.append(el)
    tblPr.append(borders)

    table.alignment = WD_TABLE_ALIGNMENT.CENTER

    def cell_border(cell, edge: str, sz: int = 8):
        tcPr = cell._tc.get_or_add_tcPr()
        tcBorders = tcPr.find(qn("w:tcBorders"))
        if tcBorders is None:
            tcBorders = OxmlElement("w:tcBorders")
            tcPr.append(tcBorders)
        el = OxmlElement(f"w:{edge}")
        el.set(qn("w:val"), "single")
        el.set(qn("w:sz"), str(sz))
        el.set(qn("w:color"), "000000")
        tcBorders.append(el)

    for cell in table.rows[0].cells:
        cell_border(cell, "top", 12)
        cell_border(cell, "bottom", 8)
    for cell in table.rows[-1].cells:
        cell_border(cell, "bottom", 12)


def fit_images(doc: Document, width_cm: float = 15.0) -> int:
    n = 0
    for shape in doc.inline_shapes:
        ratio = shape.height / shape.width  # 先记录原始长宽比，避免只改宽度把图拉变形
        shape.width = Cm(width_cm)
        shape.height = int(Cm(width_cm) * ratio)
        n += 1
    return n


def set_page_a4(doc: Document) -> None:
    """pandoc 模板默认 Letter；按项目口径统一 A4 + 2.5 cm 页边距。"""
    for section in doc.sections:
        section.page_width = Cm(21.0)
        section.page_height = Cm(29.7)
        section.top_margin = section.bottom_margin = Cm(2.5)
        section.left_margin = section.right_margin = Cm(2.5)


def center_image_paragraphs(doc: Document) -> int:
    n = 0
    for p in doc.paragraphs:
        if p._p.findall(".//" + qn("w:drawing")):
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            n += 1
    return n


def label_captions(
    doc: Document, *, figure_prefix: str, table_prefix: str
) -> tuple[int, int]:
    """给 pandoc 生成的图注/表注补上 "Figure 1." / "Table 1." 编号。"""
    n_fig = n_tab = 0
    for p in doc.paragraphs:
        style = p.style.name
        if style == "Image Caption":
            n_fig += 1
            label = f"{figure_prefix}{n_fig}."
        elif style == "Table Caption":
            n_tab += 1
            label = f"{table_prefix}{n_tab}."
        else:
            continue
        run = p.add_run(label + " ")
        run.bold = True
        run.font.name = "Arial"
        run.font.size = Pt(9.5)
        p.runs[0]._element.addprevious(run._element)
        p.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
        for r in p.runs:
            r.font.size = Pt(9.5)
    return n_fig, n_tab


def png_copy_of_pdf_figures(tmpdir: Path) -> dict[str, Path]:
    """Word 不能可靠显示嵌入的 PDF 图像，先把补充材料里的 PDF 图转成 PNG。"""
    out: dict[str, Path] = {}
    for rel in ("figures/fig2_results_v2.pdf", "figures/fig3_equivalence.pdf"):
        src = MS / rel
        if not src.exists():
            continue
        prefix = tmpdir / src.stem
        subprocess.run(
            ["pdftoppm", "-r", "400", "-png", "-singlefile", str(src), str(prefix)],
            check=True,
        )
        out[rel] = prefix.with_suffix(".png")
    return out


def build(kind: str) -> Path:
    src = MS / ("main.tex" if kind == "main" else "supplementary.tex")
    out = MS / (
        "main_v10_20261015.docx"
        if kind == "main"
        else "supplementary_v3_20261019.docx"
    )

    csl = next((p for p in CSL_CANDIDATES if p.exists()), None)
    if csl is None:
        raise SystemExit("找不到 CSL 样式文件（Zotero styles 目录）")

    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = Path(tmp)
        ref = tmpdir / "reference.docx"
        raw = tmpdir / "raw.docx"
        make_reference_docx(ref)

        tex_path = src
        if kind == "supplementary":
            pngs = png_copy_of_pdf_figures(tmpdir)
            text = src.read_text(encoding="utf-8")
            for rel, png in pngs.items():
                text = text.replace(rel, str(png))
            tex_path = tmpdir / "supplementary_png.tex"
            tex_path.write_text(text, encoding="utf-8")

        cmd = [
            "pandoc",
            str(tex_path),
            "-o",
            str(raw),
            "--citeproc",
            f"--bibliography={MS / 'refs.bib'}",
            f"--csl={csl}",
            f"--reference-doc={ref}",
            f"--resource-path={MS}",
        ]
        sh(cmd, cwd=MS)

        doc = Document(str(raw))
        format_title_block(doc, supplementary=(kind != "main"))
        set_page_a4(doc)
        for table in doc.tables:
            set_table_borders(table)
        n_img = fit_images(doc)
        center_image_paragraphs(doc)
        if kind == "main":
            label_captions(doc, figure_prefix="Figure ", table_prefix="Table ")
        else:
            label_captions(doc, figure_prefix="Figure S", table_prefix="Table S")
        doc.core_properties.title = TITLE
        doc.core_properties.author = "Xiang Li; Huichuan Huang"
        doc.save(str(out))

    print(f"written: {out}  (images: {n_img})")
    return out


def main() -> None:
    build("main")
    build("supplementary")


if __name__ == "__main__":
    main()
