#!/usr/bin/env python3
"""Record the qPCRtools 1.3.1 patch release in Yuque."""

from __future__ import annotations

from pathlib import Path
import time

import requests

TOKEN_FILE = Path("/Users/lixiang/.codex/secrets/yuque_token.txt")
BACKUP_DIR = Path("/Users/lixiang/tmp/qpcr_yuque_v131")
API = "https://www.yuque.com/api/v2"
NAMESPACE = "lixiang117423/glg12g"
DOC_IDS = {"core": 288253631, "paper": 288253647}


def replace_once(body: str, old: str, new: str) -> str:
    if body.count(old) != 1:
        raise ValueError(f"expected one match, got {body.count(old)}: {old!r}")
    return body.replace(old, new, 1)


def update_core(body: str) -> str:
    marker = "- GUI 发布：qPCRtools **1.3.1** 已修复结果图散点与显著性标记的柱内定位。"
    if marker in body:
        return body
    old = """- R/CRAN：qPCRtools 1.0.2 源码包已于 **2026-10-29** 在 CRAN 发布；
  CRAN 检查页与 Windows/macOS 二进制可能滞后更新。

## 1. 目的"""
    new = """- R/CRAN：qPCRtools 1.0.2 源码包已于 **2026-10-29** 在 CRAN 发布；
  CRAN 检查页与 Windows/macOS 二进制可能滞后更新。
- GUI 发布：qPCRtools **1.3.1** 已修复结果图散点与显著性标记的柱内定位。
  v1.3.0 保留为包含该显示缺陷的历史版本。

## 1. 目的"""
    return replace_once(body, old, new)


def update_paper(body: str) -> str:
    if (
        "## ⭐ 当前有效版本：v9（2026-10-29 qPCRtools 1.3.1 补丁发布）"
        in body
        and "### 2026-10-29 14:42｜qPCRtools 1.3.1 补丁发布与主文 v18" in body
    ):
        return body

    old_header = """**状态**：🔄 作者审阅（第二轮意见第 1–9 项闭环；CRAN 1.0.2 源码已上线；第 10 项与最终批准未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照

## ⭐ 当前有效版本：v8（2026-10-29 CRAN 1.0.2 源码上线）

- 目标期刊：**Bioinformatics (OUP) Application Note**；题目为
  “qPCRtools: a cross-platform desktop application for reproducible qPCR
  analysis without programming”。
- 主文：`main_v17_20261029.pdf`（A4，9 页）与 `main_v17_20261029.docx`
  （A4，渲染 11 页）；1 个主图、1 个主表。
- 补充材料：`supplementary_v9_20261029.pdf`（4 页）与
  `supplementary_v9_20261029.docx`（渲染 5 页）；Figure S1/S2、Table S1/S2/S3。
- Cover letter：`cover_letter_v6_20261029.pdf/.docx/.md`，A4 1 页。
- 发布：qPCRtools 1.3.0 的 tag、Windows/macOS 资产和 GitHub Release 已完成；
  R 包 2.0.1 GitHub Release 已完成；R 包 1.0.2 源码包已于 2026-10-29 在 CRAN 发布
  （CRAN 检查页与 Windows/macOS 二进制可能滞后更新）。
- issue：R 包 #1–#5 均已回复并关闭；2022 年论文影响评估和真实数据端到端流程已完成。
- 关键证据：独立 base-R 对账 58 个值最大相对差 1.9×10⁻¹⁴；C++ 53/53；
  JS `SMOKE PASS`；`REAL-WORKFLOW PASS`；投稿包门禁
  `OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page`。
- 路径：本地 `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/`；超算无。
- 剩余：第二轮第 10 项；单位邮箱、基金覆盖范围、OUP 模板/4 页限制、AE 与两位作者
  最终批准仍未完成。"""
    new_header = """**状态**：🔄 作者审阅（第二轮意见第 1–9 项闭环；qPCRtools 1.3.1 已发布；第 10 项与最终批准未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照

## ⭐ 当前有效版本：v9（2026-10-29 qPCRtools 1.3.1 补丁发布）

- 目标期刊：**Bioinformatics (OUP) Application Note**；题目为
  “qPCRtools: a cross-platform desktop application for reproducible qPCR
  analysis without programming”。
- 主文：`main_v18_20261029.pdf`（A4，9 页）与 `main_v18_20261029.docx`
  （A4，渲染 11 页）；1 个主图、1 个主表。
- 补充材料：`supplementary_v9_20261029.pdf`（4 页）与
  `supplementary_v9_20261029.docx`（渲染 5 页）；Figure S1/S2、Table S1/S2/S3。
- Cover letter：`cover_letter_v7_20261029.pdf/.docx/.md`，A4 1 页。
- 发布：qPCRtools **1.3.1** 的 tag、Windows/macOS 资产和 GitHub Release 已完成；
  v1.3.0 保留为包含结果图散点错位缺陷的历史版本。R 包 2.0.1 GitHub Release 已完成；
  R 包 1.0.2 源码包已于 2026-10-29 在 CRAN 发布。
- issue：R 包 #1–#5 均已回复并关闭；2022 年论文影响评估和真实数据端到端流程已完成。
- 关键证据：GUI 1.3.1 散点与显著性标记柱内定位回归通过；独立 base-R 对账 58 个值
  最大相对差 1.9×10⁻¹⁴；C++ 53/53；JS `SMOKE PASS`；`REAL-WORKFLOW PASS`；
  投稿包门禁 `OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page`。
- 路径：本地 `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/`；超算无。
- 剩余：第二轮第 10 项；单位邮箱、基金覆盖范围、OUP 模板/4 页限制、AE 与两位作者
  最终批准仍未完成。"""
    body = replace_once(body, old_header, new_header)

    body = replace_once(
        body,
        "| 主图 Figure 1 | 1.3.0 重建版实机结果页 | "
        "`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/"
        "manuscript/figures/fig1_results_page_en_v4_20261029.png` |",
        "| 主图 Figure 1 | 1.3.1 修复版实机结果页 | "
        "`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/"
        "manuscript/figures/fig1_results_page_en_v5_20261029.png` |",
    )
    body = replace_once(
        body,
        "| 版本声明 | 1.3.0 已于 2026-10-29 正式发布；Windows/macOS 资产已核验 |",
        "| 版本声明 | 1.3.1 已于 2026-10-29 正式发布；Windows/macOS 资产已核验 |",
    )
    body = replace_once(
        body,
        "> **【待贴图】** Figure 1 1.3.0 重建版结果页",
        "> **【待贴图】** Figure 1 1.3.1 修复版结果页",
    )
    body = replace_once(
        body,
        "> 版本：1.3.0　日期：2026-10-29",
        "> 版本：1.3.1　日期：2026-10-29",
    )

    old_closing = """第二轮意见第 1–9 项已完成：qPCRtools 1.3.0 已发布；R 包 2.0.1 已发布；
CRAN 1.0.2 源码包已于 2026-10-29 发布（CRAN 检查页与二进制可能滞后更新）；
5 个公开 issue 均已关闭；2022 年论文影响评估完成；真实数据重分析已进入正文和
Supplementary Table S3。第 10 项按要求暂留。"""
    new_closing = """第二轮意见第 1–9 项已完成：qPCRtools 1.3.1 补丁版已发布，修复 1.3.0
结果图散点与显著性标记未跟随柱子的显示缺陷；R 包 2.0.1 已发布；
CRAN 1.0.2 源码包已于 2026-10-29 发布；5 个公开 issue 均已关闭；2022 年论文影响评估
完成；真实数据重分析已进入正文和 Supplementary Table S3。第 10 项按要求暂留。"""
    body = replace_once(body, old_closing, new_closing)

    entry = """### 2026-10-29 14:42｜qPCRtools 1.3.1 补丁发布与主文 v18

**变更原因**：用户发现结果图三组散点全部落在类目中央；该显示缺陷存在于 v1.3.0。

**与上一版差异**：
- 散点与显著性标记改为 custom series，和柱/误差线共用像素偏移。
- 主文、Cover letter 与 Figure 1 同步到 1.3.1；v1.3.0 的 PDF 与截图保留。

**结果**：
- v1.3.1 annotated tag、Windows ZIP、macOS DMG 与 GitHub Release 已完成。
- Windows ZIP 146,699,347 bytes；macOS DMG 219,876,385 bytes。
- 两包内 `app.js` 与源码去 CRLF 后 SHA-256 一致；macOS 签名校验与实际启动通过。
- 投稿包门禁 `OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page`。

**输出路径**：
- <https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.3.1>
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/main_v18_20261029.pdf`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/cover_letter_v7_20261029.pdf`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/figures/fig1_results_page_en_v5_20261029.png`

"""
    heading = "## 版本记录（倒序，最新在上）\n\n"
    body = replace_once(body, heading, heading + entry)
    return body


def main() -> None:
    token = TOKEN_FILE.read_text(encoding="utf-8").strip()
    headers = {"X-Auth-Token": token, "Content-Type": "application/json"}
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)

    for kind, doc_id in DOC_IDS.items():
        response = requests.get(
            f"{API}/repos/{NAMESPACE}/docs/{doc_id}",
            headers=headers,
            timeout=30,
        )
        response.raise_for_status()
        data = response.json()["data"]
        body = data.get("body") or ""
        backup = BACKUP_DIR / f"{kind}_{doc_id}_before_v131.md"
        suffix = 2
        while backup.exists():
            backup = BACKUP_DIR / f"{kind}_{doc_id}_before_v131_{suffix}.md"
            suffix += 1
        backup.write_text(body, encoding="utf-8")
        body = update_core(body) if kind == "core" else update_paper(body)
        response = requests.put(
            f"{API}/repos/{NAMESPACE}/docs/{doc_id}",
            headers=headers,
            json={
                "title": data["title"],
                "slug": data.get("slug"),
                "body": body,
                "format": "markdown",
                "public": 0,
            },
            timeout=60,
        )
        response.raise_for_status()

        expected = (
            "GUI 发布：qPCRtools **1.3.1**"
            if kind == "core"
            else "qPCRtools 1.3.1 补丁发布与主文 v18"
        )
        written = ""
        for attempt in range(1, 6):
            verify = requests.get(
                f"{API}/repos/{NAMESPACE}/docs/{doc_id}",
                headers=headers,
                timeout=30,
            )
            verify.raise_for_status()
            written = verify.json()["data"].get("body") or ""
            if expected in written:
                break
            if attempt < 5:
                time.sleep(1)
        else:
            raise ValueError(f"read-back verification failed for {kind}: {expected!r}")
        print(
            f"updated: {kind} id={doc_id} title={data['title']} "
            f"chars={len(written)} backup={backup}"
        )


if __name__ == "__main__":
    main()
