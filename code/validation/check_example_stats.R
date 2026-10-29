#!/usr/bin/env Rscript
# validation/check_example_stats.R
#
# 复算论文 Results "典型分析流程" 一节的数值——ΔΔCt 各组的均值、标准差，以及
# 各剂量组对对照组的 Welch t 检验 p 值——用示例数据独立算一遍，供审稿人核对。
#
# 为什么要单独一个脚本：run_R.R 依赖已安装的 qPCRtools R 包；本脚本不依赖任何包，
# 按 ΔΔCt 的定义从原始 Cq 值算起，因此论文里给的数字可以"零依赖"复现。
#
# 归一化口径（Livak & Schmittgen 2001，与修正后的 C++ / R 版一致）：
#   ΔΔCt = ΔCt − mean(ΔCt of the CONTROL group)，即校准样品 = 对照组，
#   因此对照组表达量 ≈ 1（严格说是几何均值 = 1，算术均值略高于 1）。
#   注：R 包 1.0.1 曾用「全部样本的 ΔCt 均值」做基准（对照组 ≈ 0.59），已于 2026-10-15 按文献更正。
#
# 运行（code/ 目录，即 CMake 工程根）： Rscript validation/check_example_stats.R
# 依赖：base R（无需任何外部包）
#
# 实测（R 4.6.1, aarch64-apple-darwin27.0.0, 2026-10-15，对照组基准）：
#   group 0     mean 1.0145  sd 0.2098
#   group 0.5   mean 1.9169  sd 0.2060   Welch raw p = 0.006027; Holm p = 0.012055
#   group 1     mean 2.7403  sd 0.5580   Welch raw p = 0.022191; Holm p = 0.022191

cat(R.version.string, "\n")

cq <- read.table("examples/cq.csv",     header = TRUE, sep = ",",
                 stringsAsFactors = FALSE)
de <- read.table("examples/design.csv", header = TRUE, sep = ",",
                 stringsAsFactors = FALSE)

d <- merge(cq, de, by = "Position")
d <- reshape(d[, c("Group", "BioRep", "Gene", "Cq")],
             idvar = c("Group", "BioRep"), timevar = "Gene",
             direction = "wide")
names(d) <- sub("^Cq[.]", "", names(d))

target <- "fos-glo-myc"
ref    <- "Beta Actin"

d$dCt  <- d[[target]] - d[[ref]]
controlGroup <- "0"
d$ddCt <- d$dCt - mean(d$dCt[d$Group == controlGroup])   # 校准样品 = 对照组
d$expr <- 2^(-d$ddCt)

d <- d[order(d$Group, d$BioRep), ]
print(d[, c("Group", "BioRep", "dCt", "ddCt", "expr")], row.names = FALSE)

cat("\nGroup-level summary (relative expression):\n")
for (g in sort(unique(d$Group))) {
    x <- d$expr[d$Group == g]
    cat(sprintf("  group %-5s n=%d  mean=%.4f  sd=%.4f\n",
                g, length(x), mean(x), sd(x)))
}

cat("\nWelch two-sample t-test vs control (group 0), on expression values:\n")
raw_p <- numeric(0)
for (g in setdiff(sort(unique(d$Group)), 0)) {
    tt <- t.test(d$expr[d$Group == g], d$expr[d$Group == 0])
    raw_p <- c(raw_p, tt$p.value)
    cat(sprintf("  dose %-5s t=%.4f  df=%.4f  p=%.6f\n",
                g, tt$statistic, tt$parameter, tt$p.value))
}
cat("\nHolm adjustment within this gene (comparisons against the control):\n")
for (i in seq_along(raw_p)) {
    cat(sprintf("  comparison %d  raw p=%.6f  Holm p=%.6f\n",
                i, raw_p[i], p.adjust(raw_p, method = "holm")[i]))
}

dir.create("validation/results", showWarnings = FALSE, recursive = TRUE)
out <- "validation/results/check_example_stats.csv"
write.csv(d[, c("Group", "BioRep", "dCt", "ddCt", "expr")], out, row.names = FALSE)
cat("\nWritten:", out, "\n")
