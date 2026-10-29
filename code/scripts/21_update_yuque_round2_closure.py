#!/usr/bin/env python3
"""Record completion of second-round review items 1-9 in existing Yuque pages."""

from __future__ import annotations

from pathlib import Path

import requests

TOKEN_FILE = Path("/Users/lixiang/.codex/secrets/yuque_token.txt")
BACKUP_DIR = Path("/Users/lixiang/tmp/qpcr_yuque_round2_closure")
API = "https://www.yuque.com/api/v2"
NAMESPACE = "lixiang117423/glg12g"

PAGES = {
    "core": 288253631,
    "paper": 288253647,
    "overview": 288250009,
}

MARKER = "## 版本记录（倒序，最新在上）\n"


def insert_version(body: str, entry: str) -> str:
    if body.count(MARKER) != 1:
        raise ValueError("version marker is missing or duplicated")
    return body.replace(MARKER, MARKER + "\n" + entry.rstrip() + "\n", 1)


def replace_tail(body: str, heading: str, replacement: str) -> str:
    start = body.find(heading)
    if start < 0:
        raise ValueError(f"heading not found: {heading!r}")
    return body[:start] + replacement.rstrip() + "\n"


def update_core(body: str) -> str:
    entry = """### 2026-10-29 13:52｜issue #1 回归测试与第二轮意见 1–9 闭环

**变更原因**：完成第二轮审稿中剩余的发布记录、稿件状态和 issue 闭环。

**结果**：
- 新增“3 个 vs 4 个 BioRep + IQR 高端离群值”测试；R 两包树均通过。
- GitHub issue #1 已回复并关闭，R 包 5 个公开 issue 目前全部关闭。
- 真实数据流程和 `REAL-WORKFLOW PASS` 已写入正文与 Supplementary Table S3。

**路径**：
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools/tests/02_unequal_biorep_outlier.R`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools/CRAN/qPCRtools/tests/03_unequal_biorep_outlier.R`

**状态**：算法与 issue 验证闭环完成。
"""
    return insert_version(body, entry)


def update_paper(body: str) -> str:
    notes = """## 5. 问题与备注（仅保留第 10 项）

1. **单位邮箱**：两位通讯作者的 institution 邮箱尚未提供。
2. **基金声明**：2021YFD1000200、31801792、31960554 与霍英东基金是否覆盖本项工作仍需确认。
3. **官方模板与页数**：当前使用 `article` 骨架；需替换 Bioinformatics Application Note
   官方模板并复核 4 页限制。
4. **Associate Editor**：投稿系统内的人选和利益冲突确认未完成。

第二轮意见第 1–9 项已完成：qPCRtools 1.3.0 已发布；R 包 2.0.1 已发布；
CRAN 1.0.2 已通过 incoming 检查并进入处理队列；5 个 issue 已关闭；
2022 年论文影响评估完成；真实数据重分析已进入正文和 Supplementary Table S3。
"""
    body = replace_tail(body, "## 5. 问题与备注", notes)
    entry = """### 2026-10-29 13:52｜第二轮意见 1–9 闭环：v16/v9/v6 投稿包

**变更原因**：用户要求先解决第二轮意见第 1–9 项，第 10 项暂留。

**与上一版差异**：
- 1.3.0/R 包状态改为已发布；Contact 增加第二位通讯作者邮箱。
- 用已发表数据完整重分析替代不存在的用户试验，新增 Supplementary Table S3。
- Table 1 分列 qBase/qBase+；公式（2）明确对照组重复 ΔCt 均值。
- Results 明确 Gene3/Gene4 的效率质控；v1.2.0 Release 改为历史警告。
- 新增不等 BioRep + 离群值测试并关闭 issue #1。

**结果**：投稿包门禁
`OK: LaTeX 9+4 pages; Word 11+5 pages; cover 1 page`。

**文件**：
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/main_v16_20261029.pdf`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/supplementary_v9_20261029.pdf`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/manuscript/cover_letter_v6_20261029.pdf`

**状态**：第 1–9 项完成；第 10 项待用户提供/确认。
"""
    return insert_version(body, entry)


def update_overview(body: str) -> str:
    block = """## 待办与待确认
+ **第 10 项保留**：单位邮箱、基金覆盖范围、OUP 官方模板/4 页限制、Associate Editor。
"""
    end = body.find("## 子页面")
    if end < 0 or body.find("## 待办与待确认") < 0:
        raise ValueError("overview pending section not found")
    start = body.find("## 待办与待确认")
    return body[:start] + block + "\n" + body[end:]


def merge(kind: str, body: str) -> str:
    if kind == "core":
        return update_core(body)
    if kind == "paper":
        return update_paper(body)
    if kind == "overview":
        return update_overview(body)
    raise ValueError(kind)


def main() -> None:
    token = TOKEN_FILE.read_text(encoding="utf-8").strip()
    headers = {"X-Auth-Token": token, "Content-Type": "application/json"}
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    docs = {}

    for kind, doc_id in PAGES.items():
        response = requests.get(
            f"{API}/repos/{NAMESPACE}/docs/{doc_id}",
            headers=headers,
            timeout=30,
        )
        response.raise_for_status()
        data = response.json()["data"]
        body = data.get("body") or ""
        backup = BACKUP_DIR / f"{kind}_{doc_id}_before_closure.md"
        backup.write_text(body, encoding="utf-8")
        merged = merge(kind, body)
        if kind != "overview":
            old_versions = body.count("\n### ")
            new_versions = merged.count("\n### ")
            if new_versions != old_versions + 1:
                raise ValueError(f"version count mismatch for {kind}")
        docs[kind] = {
            "id": doc_id,
            "title": data["title"],
            "slug": data.get("slug"),
            "body": merged,
            "backup": backup,
        }

    for kind, data in docs.items():
        response = requests.put(
            f"{API}/repos/{NAMESPACE}/docs/{data['id']}",
            headers=headers,
            json={
                "title": data["title"],
                "slug": data["slug"],
                "body": data["body"],
                "format": "markdown",
                "public": 0,
            },
            timeout=60,
        )
        response.raise_for_status()
        verify = requests.get(
            f"{API}/repos/{NAMESPACE}/docs/{data['id']}",
            headers=headers,
            timeout=30,
        )
        verify.raise_for_status()
        written = verify.json()["data"].get("body") or ""
        if "第二轮意见 1–9" not in written and kind != "overview":
            raise ValueError(f"read-back verification failed for {kind}")
        print(
            f"updated: {kind} id={data['id']} title={data['title']} "
            f"chars={len(written)} backup={data['backup']}"
        )


if __name__ == "__main__":
    main()
