#!/usr/bin/env Rscript
# validation/check_example_stats.R
#
# 复算论文 Results "典型分析流程" 一节的数值——ΔΔCt 各组的均值、标准差，以及
# 各剂量组对对照组的 Welch t 检验 p 值——用示例数据独立算一遍，供审稿人核对。
#
# 为什么要单独一个脚本：run_R.R 依赖已安装的 qPCRtools R 包；本脚本不依赖任何包，
# 按 ΔΔCt 的定义从原始 Cq 值算起，因此论文里给的数字可以"零依赖"复现。
#
# 归一化口径（与 R 版 CalExp2ddCt、C++ 实现一致）：ΔΔCt = ΔCt − mean(ΔCt over ALL samples)，
# 即按全部样本的 ΔCt 均值归一化，因此对照组的均值并不强制等于 1。
#
# 运行（code/ 目录，即 CMake 工程根）： Rscript validation/check_example_stats.R
# 依赖：base R（无需任何外部包）
#
# 实测（R 4.6.1, aarch64-apple-darwin27.0.0, 2026-10-13）：
#   group 0     mean 0.5872  sd 0.1214
#   group 0.5   mean 1.1095  sd 0.1192   Welch t-test vs group 0: p = 0.006027
#   group 1     mean 1.5861  sd 0.3229   Welch t-test vs group 0: p = 0.022191

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
d$ddCt <- d$dCt - mean(d$dCt)          # 全样本 ΔCt 均值归一化
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
for (g in setdiff(sort(unique(d$Group)), 0)) {
    tt <- t.test(d$expr[d$Group == g], d$expr[d$Group == 0])
    cat(sprintf("  dose %-5s t=%.4f  df=%.4f  p=%.6f\n",
                g, tt$statistic, tt$parameter, tt$p.value))
}

dir.create("validation/results", showWarnings = FALSE, recursive = TRUE)
out <- "validation/results/check_example_stats.csv"
write.csv(d[, c("Group", "BioRep", "dCt", "ddCt", "expr")], out, row.names = FALSE)
cat("\nWritten:", out, "\n")
