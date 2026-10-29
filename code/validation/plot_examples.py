#!/usr/bin/env python3
# validation/plot_examples.py
#
# 生成 Figure 2（真实数据可视化）：两联图
#   A) 标准曲线：web/calsc.* 原始点 + 回归线（斜率/截距取自 cpp_standardcurve.csv）
#      横轴必须用与 C++ 相同的变换 log_d(Conc)（d = 稀释倍数 = 4，即"稀释步数"）。
#      2026-10-15 修正：原脚本点画在 log10(Conc)、线却按 log4 尺度的斜率画，两者错位
#      （导师审稿意见第 4 条：高浓度端点明显偏离回归线）。
#   B) ΔΔCt 表达量 bar：cpp_deltadeltaCt.csv (Group vs Mean ± SD + 显著性)
#
# 运行（code/ 目录）： python3 validation/plot_examples.py
# 依赖： pip install pandas matplotlib numpy
# 输出： ../manuscript/figures/fig2_results_v2.pdf（本脚本在 code/ 下运行）
#   注：不覆盖 2026-10-13 生成的旧 fig2_results.pdf（其 2A 有上述轴/斜率错位、
#       2B 用的是旧 ΔΔCt 口径），待确认后由用户自行删除。

import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# 2026-10-13 目录整理：本脚本在 code/ 下运行，manuscript/ 是它的同级目录
FIG_OUT = "../manuscript/figures/fig2_results_v2.pdf"

# 与 C++ 端一致：稀释倍数（run_cpp.cpp 调用 StandardCurve::calculate(..., dilution = 4.0)）
DILUTION = 4.0
# 与 C++ 端一致的标定浓度范围（run_cpp.cpp 里 lowestConcen = 4.0、highestConcen = 4096.0）；
# 范围外的点不参与回归，图上用空心点标出，避免"线点对不上"的误判。
LOWEST_CONCEN, HIGHEST_CONCEN = 4.0, 4096.0

# 配色按全局规范从预定义色板取（不用 matplotlib 默认色）：
#   A 图 4 个基因 → 紫色系 five 前 4 色；B 图 3 组 → 红色系 hundred 的第 1/3/5 色
FIVE    = ["#3B1F4E", "#5C2E7A", "#7A3F9D", "#9A6BBE", "#D0B8E0"]
HUNDRED = ["#780018", "#AA0033", "#DD0022", "#CC0044", "#FA8095"]
COLORS  = [HUNDRED[0], HUNDRED[2], HUNDRED[4]]


def panel_standard_curve(ax):
    cq = pd.read_csv("web/calsc.cq.txt", sep="\t")     # Position, Cq
    info = pd.read_csv("web/calsc.info.txt", sep="\t")  # Position, Gene, Conc
    m = cq.merge(info, on="Position")
    sc = pd.read_csv("validation/results/cpp_standardcurve_round2.csv")
    residuals = []
    for idx, (_, row) in enumerate(sc.iterrows()):
        g = row["Gene"]
        sub = m[m["Gene"] == g]
        # 与 C++ StandardCurve 相同的横轴变换：log_d(Conc)
        x = np.log(sub["Conc"].astype(float)) / np.log(DILUTION)
        y = sub["Cq"].astype(float)
        conc = sub["Conc"].astype(float).to_numpy()
        in_range = (conc >= LOWEST_CONCEN) & (conc <= HIGHEST_CONCEN)
        color = FIVE[idx % len(FIVE)]
        ax.scatter(x[in_range], y[in_range], s=18, color=color,
                   label=f"{g} ($R^2$={row['RSquared']:.3f})")
        ax.scatter(x[~in_range], y[~in_range], s=18, facecolors="none",
                   edgecolors="#6b7280", linewidths=0.7)
        slope, intercept = row["Slope"], row["Intercept"]
        xs = np.linspace(x[in_range].min(), x[in_range].max(), 50)
        ax.plot(xs, slope * xs + intercept, linewidth=1, color=color)
        # 自检：正确横轴下回归线应贴合数据点；误用 log10 横轴时残差会明显变大
        gm = sub.groupby("Conc")["Cq"].mean()
        gm = gm[(gm.index >= LOWEST_CONCEN) & (gm.index <= HIGHEST_CONCEN)]
        xm = np.log(gm.index.astype(float).to_numpy()) / np.log(DILUTION)
        ym = gm.to_numpy(dtype=float)
        rms_correct = float(np.sqrt(np.mean((ym - (slope * xm + intercept)) ** 2)))
        x_wrong = np.log10(gm.index.astype(float).to_numpy())
        rms_wrong = float(np.sqrt(np.mean((ym - (slope * x_wrong + intercept)) ** 2)))
        residuals.append((g, rms_correct, rms_wrong))
    for g, rc, rw in residuals:
        print(f"  [check] {g}: 均值点残差 RMS（log{DILUTION:g} 横轴）= {rc:.3f} Cq | "
              f"若误用 log10 横轴 = {rw:.3f} Cq")
    ax.set_xlabel(r"$\log_{4}$(concentration)")
    ax.set_ylabel(r"$C_q$")
    ax.set_title("(A) Standard curves")
    ax.text(0.02, 0.03, "hollow = outside calibration range (4-4096)",
            transform=ax.transAxes, fontsize=6.5, color="#6b7280")
    ax.legend(fontsize=7, loc="best", frameon=False)


def panel_expression(ax):
    df = pd.read_csv("validation/results/cpp_deltadeltaCt_holm_round2.csv")
    # 列：Gene,Group,Mean,StdDev,PValue,Significance
    df = df.sort_values("Group")
    groups = df["Group"].astype(str).tolist()
    means = df["Mean"].astype(float).tolist()
    sds = df["StdDev"].astype(float).fillna(0).tolist()
    sigs = df["Significance"].fillna("").astype(str).tolist()
    x = np.arange(len(groups))
    # 显示单个生物学重复，避免只用均值±SD掩蔽分布；柱间留白便于区分组别。
    # 直接从原始 Cq 重建每个生物学重复的表达量，避免误用旧版（全样本基准）
    # check_example_stats.csv 造成点与柱不对应。
    cq = pd.read_csv("examples/cq.csv")
    design = pd.read_csv("examples/design.csv")
    merged = cq.merge(design, on="Position")
    wide = merged.pivot_table(
        index=["Group", "BioRep"], columns="Gene", values="Cq", aggfunc="mean"
    ).reset_index()
    wide["dCt"] = wide["fos-glo-myc"] - wide["Beta Actin"]
    control_mean = wide.loc[wide["Group"].astype(str) == "0", "dCt"].mean()
    wide["expr"] = 2 ** (-(wide["dCt"] - control_mean))
    rng = np.random.default_rng(20261029)
    for i, group in enumerate(groups):
        values = wide.loc[wide["Group"].astype(str) == group, "expr"].to_numpy(dtype=float)
        offsets = np.linspace(-0.13, 0.13, len(values)) if len(values) > 1 else np.array([0.0])
        ax.scatter(
            np.full(len(values), x[i]) + offsets,
            values,
            s=22,
            facecolors="white",
            edgecolors="#333333",
            linewidths=0.8,
            zorder=4,
        )
    bars = ax.bar(x, means, yerr=sds, capsize=4, color=COLORS[: len(groups)],
                  edgecolor="black", linewidth=0.5, width=0.52)
    for bar, mean, sd, sig in zip(bars, means, sds, sigs):
        if sig:
            y = mean + sd + max(means) * 0.06
            ax.text(bar.get_x() + bar.get_width() / 2, y, sig,
                    ha="center", va="bottom", fontsize=10, fontweight="bold")
    ax.set_xticks(x)
    ax.set_xticklabels(groups)
    ax.set_xlabel("Treatment group")
    ax.set_ylabel("Relative expression")
    ax.set_title("(B) Expression (fos-glo-myc)")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def main():
    os.makedirs(os.path.dirname(FIG_OUT), exist_ok=True)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(17.0, 8.0))
    panel_standard_curve(ax1)
    panel_expression(ax2)
    fig.tight_layout()
    fig.savefig(FIG_OUT, dpi=600)
    png_out = os.path.splitext(FIG_OUT)[0] + ".png"
    fig.savefig(png_out, dpi=600)
    print(f"[plot] -> {FIG_OUT}")
    print(f"[plot] -> {png_out}")


if __name__ == "__main__":
    main()
