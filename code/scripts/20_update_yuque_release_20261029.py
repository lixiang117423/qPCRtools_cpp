#!/usr/bin/env python3
"""Merge the 2026-10-29 release results into the existing Yuque pages."""

from __future__ import annotations

from pathlib import Path

import requests

TOKEN_FILE = Path("/Users/lixiang/.codex/secrets/yuque_token.txt")
BACKUP_DIR = Path("/Users/lixiang/tmp/qpcr_yuque_release_20261029")
API = "https://www.yuque.com/api/v2"
NAMESPACE = "lixiang117423/glg12g"

PAGES = {
    "core": 288253631,
    "paper": 288253647,
    "overview": 288250009,
}

MARKER = "## 版本记录（倒序，最新在上）\n"


def replace_once(body: str, old: str, new: str) -> str:
    if body.count(old) != 1:
        raise ValueError(
            f"expected exactly one match, got {body.count(old)}: {old[:100]!r}"
        )
    return body.replace(old, new, 1)


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
    body = replace_once(
        body,
        "- 1.3.0 的公开 release 与匹配二进制仍未上传，是投稿阻断项。",
        "- 1.3.0 已于 2026-10-29 发布，Windows/macOS 资产和真实数据端到端流程均已核验。",
    )
    entry = """### 2026-10-29 10:40｜1.3.0 正式发布，补齐标准化真实数据闭环

**变更原因**：完成 qPCRtools_cpp 1.3.0 的源码 tag、Windows/macOS 包和 GitHub Release，
并修复 Standard Curve Expression 原始表未按汇总口径归一化的问题。

**与上一版差异**：算法主体未变；`rawData` 的逐重复 `Expression` 现在除以与汇总表相同的
基因内最小值，因此逐重复均值与汇总 `Mean` 严格一致。新增真实示例板端到端流程和确定性门禁。

**结果**：
- C++ `ctest` 53/53；JS `SMOKE PASS`；`REFERENCE-COMPARE PASS`；
  `REAL-WORKFLOW PASS`。
- 标准曲线四个基因：Gene1 R²=0.7509（不合格）；Gene2–4 R²≥0.9983。
- 组间分析：OSPR10 Holm P=0.0368（`*`）；OsWAK91 Holm P=0.002303（`**`）；
  其余基因 NS。
- Release：`qPCRtools-1.3.0-windows.zip` 146,699,165 bytes；
  `qPCRtools-1.3.0-macOS.dmg` 219,709,193 bytes。

**输出路径**：
- <https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.3.0>
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/examples/real_workflow/`
- `/Users/lixiang/NutstoreFiles/03.编程相关/qPCRtools_cpp/code/scripts/19_verify_real_workflow.py`

**状态**：发布完成；后续仍待用户决定是否补 Linux 包或重做论文图 2/3。
"""
    return insert_version(body, entry)


def update_paper(body: str) -> str:
    body = replace_once(
        body,
        "**状态**：🔄 作者审阅（第二轮返修包完成；公开 release、R 包修补与作者确认未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照",
        "**状态**：🔄 作者审阅（release 与 R 包闭环已完成；作者信息、期刊模板与最终批准未清）　**上游**：02 核心算法与独立数值验证、04 构建打包与发布　**下游**：97 论文图表对照",
    )
    body = replace_once(
        body,
        "- 当前不能投稿：1.3.0 公开 release、R 包修补、真实端到端数据、单位邮箱、基金、\n  OUP 模板/4 页限制、AE 与两位作者最终批准尚未完成。",
        "- 当前不能投稿：单位邮箱、基金覆盖范围、OUP 模板/4 页限制、AE 与两位作者最终批准\n  尚未完成；1.3.0、R 包修补和真实端到端数据已在 2026-10-29 闭环。",
    )
    body = replace_once(
        body,
        "| 版本声明 | 1.3.0 明确为投稿前待发布；1.2.0 为现有公开二进制 |",
        "| 版本声明 | 1.3.0 已于 2026-10-29 正式发布；Windows/macOS 资产已核验 |",
    )
    pending = """## 5. 问题与备注（待用户确认 / 处理）

1. **已完成发布闭环**：qPCRtools 1.3.0 GitHub Release；R 包 2.0.1 GitHub Release；
   CRAN 1.0.2 已上传并完成邮件确认；issues #2–#5 已关闭，#1 保留等待用户复测；
   2022 年论文影响评估与真实数据端到端示例已归档。
2. **作者信息**：两位通讯作者的单位邮箱未提供；基金覆盖范围需确认。
3. **期刊格式**：OUP 官方 Application Note 模板和 4 页限制尚未取得；不能用当前 8 页
   `article` 骨架声称已满足。
4. **投稿系统**：Associate Editor 人选与利益冲突声明未定；两位作者尚未完成最终批准。
5. **图片**：语雀 API 无图片上传端点，Figure 1、S1、S2 仍需用户人工拖入对应位置。
6. **论文数值影响**：R 包 1.0.2 会使 2022 年论文 Figures 2–4 的示例数值变化，
   必须使用影响评估目录中的新结果重跑，不能复用旧 Source Data。
"""
    body = replace_tail(body, "## 5. 问题与备注", pending)
    entry = """### 2026-10-29 10:40｜1.3.0 与 R 包发布闭环，真实数据示例补齐

**变更原因**：完成此前列出的三项硬阻断：软件发布、R 包/issue/旧论文影响评估、
真实数据端到端流程。

**与上一版差异**：
- qPCRtools 1.3.0 发布 Windows/macOS 资产。
- R 包 2.0.1 GitHub Release；1.0.2 已提交 CRAN并完成维护者邮件确认。
- issue #2/#3/#4/#5 已关闭；#1 保留待用户复测。
- 建立 2022 年论文示例数据的“标曲 → 方法选择 → 组间分析 → 记录导出”流程。

**结果**：53/53 C++ tests、`SMOKE PASS`、`REFERENCE-COMPARE PASS`、
`REAL-WORKFLOW PASS`；R 包两套源码包 `R CMD check --as-cran --no-manual`
均为 `Status: OK`（本地远程 incoming 检查关闭以避免网络超时）。

**发布链接**：
- <https://github.com/lixiang117423/qPCRtools_cpp/releases/tag/v1.3.0>
- <https://github.com/lixiang117423/qPCRtools/releases/tag/v2.0.1>

**状态**：发布与示例闭环完成；稿件仍需作者信息、期刊模板和最终批准。
"""
    return insert_version(body, entry)


def update_overview(body: str) -> str:
    body = replace_once(
        body,
        "4. **04 构建、打包与发布**：公开 release 为 1.2.0；源码工作树为 1.3.0，匹配包未发布。",
        "4. **04 构建、打包与发布**：1.3.0 已发布，Windows/macOS 资产与真实数据流程已核验。",
    )
    body = replace_once(
        body,
        "| 04 构建、打包与发布 | 🔄 | 1.2.0 已发布；1.3.0 未发布 | `docs/08_status.md` + 投稿清单 | 2026-10-29 08:52 |",
        "| 04 构建、打包与发布 | ✅ | v1.3.0（Windows/macOS） | GitHub Release + Actions run `40806820842` | 2026-10-29 10:40 |",
    )
    body = replace_once(
        body,
        "| 代码仓库 | 工作树 1.3.0（未提交）；公开 release 仍为 1.2.0 | GitHub 公开仓库，MIT 许可 | `https://github.com/lixiang117423/qPCRtools_cpp` |",
        "| 代码仓库 | v1.3.0 已提交并发布 | GitHub 公开仓库，MIT 许可 | `https://github.com/lixiang117423/qPCRtools_cpp` |",
    )
    body = replace_once(
        body,
        "| 发版 tag | v0.0.1 / v0.1.1 / v0.1.2 / v1.0.0 / v1.1.0 / v1.2.0 | 最新公开 release 为 v1.2.0；1.3.0 仍未发布 | 同上 Release 页 |",
        "| 发版 tag | v0.0.1 / v0.1.1 / v0.1.2 / v1.0.0 / v1.1.0 / v1.2.0 / v1.3.0 | 最新公开 release 为 v1.3.0 | 同上 Release 页 |",
    )
    body = replace_once(
        body,
        "| 发布二进制 | v1.2.0 | 免安装、免注册；1.3.0 匹配包待发布 | `https://github.com/lixiang117423/qPCRtools_cpp/releases` |",
        "| 发布二进制 | v1.3.0：Windows ZIP / macOS DMG | 免安装、免注册；macOS 未公证 | `https://github.com/lixiang117423/qPCRtools_cpp/releases` |",
    )
    conclusions = """## 关键结论
+ **当前验证不再依赖被移植对象本身**：`reference_base_r.R` 不加载 qPCRtools R 包，
  从原始 Cq 表复算三个定量方法；58 个配对值最大相对差 1.9×10⁻¹⁴。
+ **发布闭环已完成**：qPCRtools 1.3.0 的 Windows/macOS 资产、R 包 2.0.1 GitHub Release
  和 CRAN 1.0.2 提交均已落地；issue #2–#5 已关闭。
+ **真实数据示例已完成**：2022 年论文示例板已跑通标曲、方法选择、Welch/Holm 组间分析、
  QC 与全部导出；确定性检查为 `REAL-WORKFLOW PASS`。
+ **论文影响已量化**：R 包 1.0.2 使 2022 年论文 Figures 2–4 数值变化，
  Figure 1/CalCurve 不变；旧 Source Data 不得直接复用。
+ **仍需用户处理**：单位邮箱、基金、OUP 官方模板/4 页限制、AE、两位作者最终批准。

## 待办与待确认
+ **作者与声明**：单位通讯邮箱、基金覆盖范围、Associate Editor 和两位作者最终批准待确认。
+ **期刊格式**：官方 Application Note 模板与 4 页限制尚未取得。
+ **语雀图片**：API 无上传端点，Figure 1、S1、S2 仍由用户人工拖入。
+ **issue #1**：等待用户用不等 BioRep 数据复测。
"""
    end = body.find("## 子页面")
    if end < 0:
        raise ValueError("子页面 heading not found")
    body = body[: body.find("## 关键结论")] + conclusions + "\n" + body[end:]
    return body


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
        backup = BACKUP_DIR / f"{kind}_{doc_id}_before_release.md"
        backup.write_text(body, encoding="utf-8")
        merged = merge(kind, body)
        if kind != "overview":
            old_versions = body.count("\n### ")
            new_versions = merged.count("\n### ")
            if new_versions != old_versions + 1:
                raise ValueError(
                    f"version count mismatch for {kind}: "
                    f"{old_versions} -> {new_versions}"
                )
        docs[kind] = {
            "id": doc_id,
            "title": data["title"],
            "slug": data.get("slug"),
            "body": merged,
            "backup": backup,
        }

    for kind, data in docs.items():
        payload = {
            "title": data["title"],
            "slug": data["slug"],
            "body": data["body"],
            "format": "markdown",
            "public": 0,
        }
        response = requests.put(
            f"{API}/repos/{NAMESPACE}/docs/{data['id']}",
            headers=headers,
            json=payload,
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
        if "2026-10-29 10:40" not in written or "REAL-WORKFLOW PASS" not in written:
            if kind != "overview":
                raise ValueError(f"read-back verification failed for {kind}")
        print(
            f"updated: {kind} id={data['id']} title={data['title']} "
            f"chars={len(written)} backup={data['backup']}"
        )


if __name__ == "__main__":
    main()
