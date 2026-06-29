#!/usr/bin/env python3
# validation/compare.py
#
# 对比 R 版 (run_R.R) 与 C++ 版 (run_cpp) 的输出 CSV，生成：
#   - validation/results/comparison.csv   逐数值列的相对差汇总
#   - paper/figures/fig3_equivalence.pdf  C++ 结果 vs R 结果散点（y=x 参考线）
#
# 运行（仓库根，需先产出 R_*.csv 与 cpp_*.csv）： python3 validation/compare.py
# 依赖： pip install pandas matplotlib
#
# 对齐策略：把 R 与 C++ 的列名归一化到规范名，按"共同键列"(gene/group 等)
# merge 后再配对数值列——避免两边行顺序不同导致的错配。

import os
import sys
import pandas as pd
import matplotlib.pyplot as plt

RESULTS = "validation/results"
FIG_OUT = "paper/figures/fig3_equivalence.pdf"

PAIRS = {
    "deltadeltaCt":  ("R_deltadeltaCt.csv",  "cpp_deltadeltaCt.csv"),
    "standardcurve": ("R_standardcurve.csv", "cpp_standardcurve.csv"),
    "rqpcr":         ("R_rqpcr.csv",         "cpp_rqpcr.csv"),
}

# 列名归一化：R 名 / C++ 名 -> 规范名
COLUMN_ALIASES = {
    # 表达量
    "Mean": "expression", "mean.expre": "expression", "mean_expre": "expression",
    "Expression": "expression", "expression": "expression",
    # 标准差
    "StdDev": "sd", "sd.expre": "sd", "sd_expre": "sd",
    # p 值
    "PValue": "pvalue", "P.value": "pvalue", "p.value": "pvalue", "p": "pvalue",
    # 标准曲线
    "Slope": "slope", "Intercept": "intercept",
    "RSquared": "r2", "R2": "r2",
    "Efficiency": "efficiency", "E": "efficiency", "eff": "efficiency",
}
# 键列归一化（用于 merge）
KEY_ALIASES = {
    "Gene": "gene", "gene": "gene",
    "Group": "grp", "group": "grp", "Treatment": "grp",
}


KEY_NAMES = {"gene", "grp"}  # 即使是数值也强制作为键（组号 0/0.5/1 等）


def normalize(df):
    """归一化列名；返回 (归一化后的 df, 规范键列表, 规范数值列列表)。"""
    df = df.rename(columns={c: COLUMN_ALIASES.get(c, KEY_ALIASES.get(c, c))
                            for c in df.columns})
    key_cols = [c for c in df.columns
                if c in KEY_NAMES or not pd.api.types.is_numeric_dtype(df[c])]
    num_cols = [c for c in df.columns
                if c not in key_cols and pd.api.types.is_numeric_dtype(df[c])]
    return df, key_cols, num_cols


def align(r_path, c_path):
    r, r_keys, r_num = normalize(pd.read_csv(r_path))
    c, c_keys, c_num = normalize(pd.read_csv(c_path))

    merge_keys = [k for k in r_keys if k in c_keys]
    metrics = sorted(set(r_num) & set(c_num))
    if not metrics:
        return [], [], []

    rows, r_vals, c_vals = [], [], []
    if merge_keys:
        # R 可能有重复行（如 ΔΔCt 每技术重复一行），按键去重保留汇总
        r_dedup = r.drop_duplicates(subset=merge_keys)
        c_dedup = c.drop_duplicates(subset=merge_keys)
        merged = r_dedup.merge(c_dedup, on=merge_keys, suffixes=("_R", "_C"))
        for _, m in merged.iterrows():
            key_str = "/".join(str(m[k]) for k in merge_keys)
            for met in metrics:
                rv, cv = m.get(f"{met}_R"), m.get(f"{met}_C")
                if pd.isna(rv) or pd.isna(cv):
                    continue
                if abs(rv) < 1e-9 or abs(cv) < 1e-9:
                    continue  # 接近 0（如 R 回归 p 值显示为 0），相对差无意义
                rel = abs(rv - cv) / (abs(rv) + 1e-12)
                rows.append({"key": key_str, "metric": met,
                             "R": rv, "C++": cv, "rel_diff": rel})
                r_vals.append(rv); c_vals.append(cv)
    else:
        # 无共同键：按行顺序配对（仅当行数相同）
        n = min(len(r), len(c))
        for i in range(n):
            for met in metrics:
                rv, cv = r[met].iloc[i], c[met].iloc[i]
                if pd.isna(rv) or pd.isna(cv):
                    continue
                rel = abs(rv - cv) / (abs(rv) + 1e-12)
                rows.append({"key": str(i), "metric": met,
                             "R": rv, "C++": cv, "rel_diff": rel})
                r_vals.append(rv); c_vals.append(cv)
    return rows, r_vals, c_vals


def main():
    if not os.path.isdir(RESULTS):
        sys.exit(f"结果目录不存在: {RESULTS}（请先运行 run_R.R 与 run_cpp）")
    all_rows, all_r, all_c = [], [], []
    for name, (rp, cp) in PAIRS.items():
        rpath, cpath = os.path.join(RESULTS, rp), os.path.join(RESULTS, cp)
        if not (os.path.exists(rpath) and os.path.exists(cpath)):
            print(f"[skip] {name}: 缺 {rp} 或 {cp}")
            continue
        rows, rv, cv = align(rpath, cpath)
        for row in rows:
            row["method"] = name
        all_rows.extend(rows)
        all_r.extend(rv); all_c.extend(cv)
        if rows:
            maxrel = max(r["rel_diff"] for r in rows)
            print(f"[{name}] 配对点数={len(rows)}  最大相对差={maxrel:.3e}")

    if not all_rows:
        sys.exit("无可对比数据。")

    pd.DataFrame(all_rows).to_csv(os.path.join(RESULTS, "comparison.csv"), index=False)
    print(f"[compare] -> {RESULTS}/comparison.csv")

    os.makedirs(os.path.dirname(FIG_OUT), exist_ok=True)
    fig, ax = plt.subplots(figsize=(4.2, 4.2))
    ax.scatter(all_r, all_c, s=20, alpha=0.7)
    lo = min(min(all_r), min(all_c)); hi = max(max(all_r), max(all_c))
    ax.plot([lo, hi], [lo, hi], "r--", lw=1, label="y = x")
    ax.set_xlabel("R package result"); ax.set_ylabel("C++ implementation result")
    ax.set_aspect("equal", adjustable="datalim")
    ax.legend(loc="upper left", frameon=False)
    fig.tight_layout()
    fig.savefig(FIG_OUT)
    print(f"[compare] -> {FIG_OUT}")


if __name__ == "__main__":
    main()
