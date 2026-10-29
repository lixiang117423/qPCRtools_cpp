#!/usr/bin/env python3
"""Update Yuque release status now that CRAN serves qPCRtools 1.0.2."""

from __future__ import annotations

from pathlib import Path
import time

import requests

TOKEN_FILE = Path("/Users/lixiang/.codex/secrets/yuque_token.txt")
BACKUP_DIR = Path("/Users/lixiang/tmp/qpcr_yuque_cran_live")
API = "https://www.yuque.com/api/v2"
NAMESPACE = "lixiang117423/glg12g"
DOC_IDS = {"core": 288253631, "paper": 288253647}


def replace_once(body: str, old: str, new: str) -> str:
    if body.count(old) != 1:
        raise ValueError(f"expected one match, got {body.count(old)}: {old!r}")
    return body.replace(old, new, 1)


def update_core(body: str) -> str:
    marker = "- R/CRAN：qPCRtools 1.0.2 源码包已于 **2026-10-29** 在 CRAN 发布；"
    if marker in body:
        return body
    old = """- 边界：数值一致只证明实现符合指定算法，不等于生物学正确；RqPCR 的 `Cq,min`
  在每个 biological replicate 内跨处理取得，因此要求按重复配对的区组设计。

## 1. 目的"""
    new = """- 边界：数值一致只证明实现符合指定算法，不等于生物学正确；RqPCR 的 `Cq,min`
  在每个 biological replicate 内跨处理取得，因此要求按重复配对的区组设计。
- R/CRAN：qPCRtools 1.0.2 源码包已于 **2026-10-29** 在 CRAN 发布；
  CRAN 检查页与 Windows/macOS 二进制可能滞后更新。

## 1. 目的"""
    return replace_once(body, old, new)


def update_paper(body: str) -> str:
    if (
        "## ⭐ 当前有效版本：v8（2026-10-29 CRAN 1.0.2 源码上线）" in body
        and "### 2026-10-29 14:00｜CRAN 1.0.2 源码上线与主文 v17" in body
    ):
        return body
    old_header = """**状态**：🔄 作者审阅（第二轮意见第 1–9 项闭环；第 10 项与最终批准未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照

## ⭐ 当前有效版本：v7（2026-10-29 第二轮意见第 1–9 项闭环）

- 目标期刊：**Bioinformatics (OUP) Application Note**；题目为
  “qPCRtools: a cross-platform desktop application for reproducible qPCR
  analysis without programming”。
- 主文：`main_v16_20261029.pdf`（A4，9 页）与 `main_v16_20261029.docx`
  （A4，渲染 11 页）；1 个主图、1 个主表。
- 补充材料：`supplementary_v9_20261029.pdf`（4 页）与
  `supplementary_v9_20261029.docx`（渲染 5 页）；Figure S1/S2、Table S1/S2/S3。
- Cover letter：`cover_letter_v6_20261029.pdf/.docx/.md`，A4 1 页。
- 发布：qPCRtools 1.3.0 的 tag、Windows/macOS 资产和 GitHub Release 已完成；
  R 包 2.0.1 GitHub Release 已完成；R 包 1.0.2 源码包已于 2026-10-29 在 CRAN 发布。
- issue：R 包 #1–#5 均已回复并关闭；2022 年论文影响评估和真实数据端到端流程已完成。
- 关键证据：独立 base-R 对账 58 个值最大相对差 1.9×10⁻¹⁴；C++ 53/53；
  JS `SMOKE PASS`；`REAL-WORKFLOW PASS`；投稿包门禁
  `OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page`。
- 路径：本地 `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/`；超算无。
- 剩余：第二轮第 10 项；单位邮箱、基金覆盖范围、OUP 模板/4 页限制、AE 与两位作者
  最终批准仍未完成。"""
    new_header = """**状态**：🔄 作者审阅（第二轮意见第 1–9 项闭环；CRAN 1.0.2 源码已上线；第 10 项与最终批准未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照

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
    body = replace_once(body, old_header, new_header)

    old_closing = """第二轮意见第 1–9 项已完成：qPCRtools 1.3.0 已发布；R 包 2.0.1 已发布；
CRAN 1.0.2 已通过 incoming 检查并进入处理队列；5 个 issue 已关闭；
2022 年论文影响评估完成；真实数据重分析已进入正文和 Supplementary Table S3。"""
    new_closing = """第二轮意见第 1–9 项已完成：qPCRtools 1.3.0 已发布；R 包 2.0.1 已发布；
CRAN 1.0.2 源码包已于 2026-10-29 发布（CRAN 检查页与二进制可能滞后更新）；
5 个公开 issue 均已关闭；2022 年论文影响评估完成；真实数据重分析已进入正文和
Supplementary Table S3。第 10 项按要求暂留。"""
    if old_closing in body:
        body = replace_once(body, old_closing, new_closing)
    elif new_closing not in body:
        raise ValueError("paper closing status block not found")

    entry = """### 2026-10-29 14:00｜CRAN 1.0.2 源码上线与主文 v17

**变更原因**：CRAN 包页已显示 qPCRtools 1.0.2，发布日期为 2026-10-29；
正文中的“已提交 CRAN”需要同步为“可从 CRAN 获取”。

**与上一版差异**：
- 主文另存为 `main_v17_20261029.pdf/.docx`，v16 保留不覆盖。
- Results 的 R 包状态改为 1.0.2 可从 CRAN 获取。
- CRAN 检查页与 Windows/macOS 二进制仍可能显示 1.0.1，未误写成已同步。

**结果**：投稿包门禁
`OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page`。

**输出路径**：本地
`/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/`；超算无。

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
        backup = BACKUP_DIR / f"{kind}_{doc_id}_before_cran_live.md"
        suffix = 2
        while backup.exists():
            backup = BACKUP_DIR / f"{kind}_{doc_id}_before_cran_live_{suffix}.md"
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
            "qPCRtools 1.0.2 源码包已于 **2026-10-29** 在 CRAN 发布"
            if kind == "core"
            else "第 10 项按要求暂留"
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
