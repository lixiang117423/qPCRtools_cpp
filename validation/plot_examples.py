#!/usr/bin/env python3
# validation/plot_examples.py
#
# 生成 Figure 2（真实数据可视化）：两联图
#   A) 标准曲线：web/calsc.* 原始点 (log Conc vs Cq) + 回归线（斜率/截距取自
#      cpp_standardcurve.csv）
#   B) ΔΔCt 表达量 bar：cpp_deltadeltaCt.csv (Group vs Mean ± SD + 显著性)
#
# 运行（仓库根）： python3 validation/plot_examples.py
# 依赖： pip install pandas matplotlib numpy
# 输出： paper/figures/fig2_results.pdf

import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

FIG_OUT = "paper/figures/fig2_results.pdf"

# 标准 ΔΔCt 颜色与显著性
COLORS = ["#4C72B0", "#55A868", "#C44E52"]


def panel_standard_curve(ax):
    cq = pd.read_csv("web/calsc.cq.txt", sep="\t")     # Position, Cq
    info = pd.read_csv("web/calsc.info.txt", sep="\t")  # Position, Gene, Conc
    m = cq.merge(info, on="Position")
    sc = pd.read_csv("validation/results/cpp_standardcurve.csv")  # Gene,Slope,...
    for _, row in sc.iterrows():
        g = row["Gene"]
        sub = m[m["Gene"] == g]
        x = np.log10(sub["Conc"].astype(float))
        y = sub["Cq"].astype(float)
        ax.scatter(x, y, s=18, label=f"{g} ($R^2$={row['RSquared']:.3f})")
        slope, intercept = row["Slope"], row["Intercept"]
        xs = np.linspace(x.min(), x.max(), 50)
        ax.plot(xs, slope * xs + intercept, linewidth=1)
    ax.set_xlabel(r"$\log_{10}$(concentration)")
    ax.set_ylabel(r"$C_q$")
    ax.set_title("(A) Standard curves")
    ax.legend(fontsize=7, loc="best", frameon=False)


def panel_expression(ax):
    df = pd.read_csv("validation/results/cpp_deltadeltaCt.csv")
    # 列：Gene,Group,Mean,StdDev,PValue,Significance
    df = df.sort_values("Group")
    groups = df["Group"].astype(str).tolist()
    means = df["Mean"].astype(float).tolist()
    sds = df["StdDev"].astype(float).fillna(0).tolist()
    sigs = df["Significance"].fillna("").astype(str).tolist()
    x = np.arange(len(groups))
    bars = ax.bar(x, means, yerr=sds, capsize=4, color=COLORS[: len(groups)],
                  edgecolor="black", linewidth=0.5)
    for xi, m, s in zip(x, means, sigs):
        if s:
            ax.text(xi, m + (max(means) * 0.03), s, ha="center", va="bottom",
                    fontsize=9)
    ax.set_xticks(x)
    ax.set_xticklabels(groups)
    ax.set_xlabel("Treatment group")
    ax.set_ylabel(r"Relative expression ($2^{-\Delta\Delta C_t}$)")
    ax.set_title("(B) Expression (fos-glo-myc)")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def main():
    os.makedirs(os.path.dirname(FIG_OUT), exist_ok=True)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(8, 3.4))
    panel_standard_curve(ax1)
    panel_expression(ax2)
    fig.tight_layout()
    fig.savefig(FIG_OUT)
    print(f"[plot] -> {FIG_OUT}")


if __name__ == "__main__":
    main()
