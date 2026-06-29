#!/usr/bin/env python3
# validation/compare.py
#
# 对比 R 版 (run_R.R) 与 C++ 版 (run_cpp) 的输出 CSV，生成：
#   - validation/results/comparison.csv   逐数值列的最大相对差汇总
#   - paper/figures/fig3_equivalence.pdf  C++ 结果 vs R 结果散点（y=x 参考线）
#
# 运行（仓库根目录，需先产出 R_*.csv 与 cpp_*.csv）： python3 validation/compare.py
# 依赖： pip install pandas matplotlib
#
# 列名对齐说明：R 与 C++ 输出的列名可能不同（如 Mean vs Expression）。
# 本脚本按"共同数值列 + 共同行键(Gene/Group 等)"自动配对；运行 R 后若发现
# 列名差异，在 COLUMN_ALIASES 里补充映射即可。

import os
import sys
import glob
import pandas as pd
import matplotlib.pyplot as plt

RESULTS = "validation/results"
FIG_OUT = "paper/figures/fig3_equivalence.pdf"

# 方法 -> (R 输出, C++ 输出)
PAIRS = {
    "deltadeltaCt": ("R_deltadeltaCt.csv", "cpp_deltadeltaCt.csv"),
    "rqpcr":        ("R_rqpcr.csv",        "cpp_rqpcr.csv"),
    "standardcurve":("R_standardcurve.csv","cpp_standardcurve.csv"),
}

# 用于按行对齐的候选键列（取第一个两表都有的）
KEY_CANDIDATES = ["Gene", "gene", "Group", "group", "Treatment", "Position"]

# 数值列名别名（R 名 -> 规范名），按需补充
COLUMN_ALIASES = {
    "Mean": "value", "Expression": "value", "expression": "value",
    "PValue": "pvalue", "p.value": "pvalue", "p": "pvalue",
    "Efficiency": "efficiency", "eff": "efficiency",
    "Slope": "slope", "RSquared": "r2", "R2": "r2",
}


def numeric_columns(df):
    return [c for c in df.columns if pd.api.types.is_numeric_dtype(df[c])]


def align(r_path, c_path):
    r = pd.read_csv(r_path)
    c = pd.read_csv(c_path)
    # 选共同行键
    key = next((k for k in KEY_CANDIDATES if k in r.columns and k in c.columns), None)
    rows = []
    r_vals, c_vals = [], []
    r_num = numeric_columns(r)
    c_num = numeric_columns(c)
    # 数值列按别名归一后取交集
    def norm(col):
        return COLUMN_ALIASES.get(col, col)
    common = set(map(norm, r_num)) & set(map(norm, c_num))
    rn = {norm(col): col for col in r_num}
    cn = {norm(col): col for col in c_num}
    for canon in sorted(common):
        rs = r[rn[canon]].astype(float).reset_index(drop=True)
        cs = c[cn[canon]].astype(float).reset_index(drop=True)
        n = min(len(rs), len(cs))
        for i in range(n):
            rv, cv = rs[i], cs[i]
            if pd.isna(rv) or pd.isna(cv):
                continue
            rel = abs(rv - cv) / (abs(rv) + 1e-12)
            rows.append({"metric": canon, "R": rv, "C++": cv, "rel_diff": rel})
            r_vals.append(rv)
            c_vals.append(cv)
    return rows, r_vals, c_vals


def main():
    if not os.path.isdir(RESULTS):
        sys.exit(f"结果目录不存在: {RESULTS}（请先运行 run_R.R 与 run_cpp）")
    all_rows, all_r, all_c = [], [], []
    for name, (rp, cp) in PAIRS.items():
        rpath = os.path.join(RESULTS, rp)
        cpath = os.path.join(RESULTS, cp)
        if not (os.path.exists(rpath) and os.path.exists(cpath)):
            print(f"[skip] {name}: 缺 {rp} 或 {cp}")
            continue
        rows, rv, cv = align(rpath, cpath)
        for row in rows:
            row["method"] = name
        all_rows.extend(rows)
        all_r.extend(rv)
        all_c.extend(cv)
        if rows:
            maxrel = max(r["rel_diff"] for r in rows)
            print(f"[{name}] 配对点数={len(rows)}  最大相对差={maxrel:.3e}")

    if not all_rows:
        sys.exit("无可对比数据。")

    comp = pd.DataFrame(all_rows)
    comp.to_csv(os.path.join(RESULTS, "comparison.csv"), index=False)
    print(f"[compare] -> {RESULTS}/comparison.csv")

    # Fig 3: C++ vs R 散点
    os.makedirs(os.path.dirname(FIG_OUT), exist_ok=True)
    fig, ax = plt.subplots(figsize=(4.2, 4.2))
    ax.scatter(all_r, all_c, s=18, alpha=0.7)
    lo = min(min(all_r), min(all_c))
    hi = max(max(all_r), max(all_c))
    ax.plot([lo, hi], [lo, hi], "r--", lw=1, label="y = x")
    ax.set_xlabel("R package result")
    ax.set_ylabel("C++ implementation result")
    ax.set_aspect("equal", adjustable="datalim")
    ax.legend(loc="upper left", frameon=False)
    fig.tight_layout()
    fig.savefig(FIG_OUT)
    print(f"[compare] -> {FIG_OUT}")


if __name__ == "__main__":
    main()
