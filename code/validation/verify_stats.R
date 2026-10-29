#!/usr/bin/env Rscript
# LEGACY: superseded by validation/reference_base_r.R and
# validation/compare_to_reference.R. Kept only for historical comparison with
# the 1.2.0 validation output.
# validation/verify_stats.R
#
# 统计层的"独立参照"对账：把应用（C++，1.2.0）在捆绑示例数据上给出的
# 均值 / 标准差 / Welch p 值 / ANOVA p 值 / Tukey 字母，用**零依赖的 base R**
# 从原始 Cq 值重新算一遍，逐项比对后写表。
#
# 为什么单独一个脚本（导师 2026-10-14 意见「一」第 3 条"等价性报告表要含 p 值与
# Tukey 字母"，以及「四」的验证要求）：
#   1) run_R.R 依赖已安装的 qPCRtools R 包，审稿人不一定能装；
#      本脚本只依赖 base R，任何人 clone 仓库后都能复现；
#   2) 统计层是审稿意见集中点（p 值口径、多重比较、Tukey 字母），
#      需要"应用 → R 包 → 独立参照"三方对账，而不是只跟 R 包比。
#
# 归一化口径（Livak & Schmittgen 2001，与 1.2.0 一致）：
#   ΔΔCt = ΔCt − mean(ΔCt of the CONTROL group)：校准样品＝对照组，所以对照组 ≈ 1。
#
# 运行（在 code/ 目录下）：
#   Rscript validation/verify_stats.R
# 前置：先跑 C++ 侧导出当前版本结果（文件名带 _v12 后缀，不覆盖历史结果）
#   cmake -S . -B /tmp/qpcr_val -DQPCRTOOLS_BUILD_VALIDATION=ON -DCMAKE_BUILD_TYPE=Release
#   cmake --build /tmp/qpcr_val --target run_cpp
#   (cd . && /tmp/qpcr_val/run_cpp v12)      # 见 validation/README.md

cat(R.version.string, "\n")
ok <- TRUE
note <- function(pass, msg) {
    if (!pass) ok <<- FALSE
    cat(if (pass) "[OK]   " else "[FAIL] ", msg, "\n", sep = "")
}

# ---------------------------------------------------------------------------
# 1) 独立参照：只用 base R，从原始 Cq 值算起
# ---------------------------------------------------------------------------
cq <- read.csv("examples/cq.csv", stringsAsFactors = FALSE)
de <- read.csv("examples/design.csv", stringsAsFactors = FALSE)

d <- merge(cq, de, by = "Position")
d <- reshape(d[, c("Group", "BioRep", "Gene", "Cq")],
             idvar = c("Group", "BioRep"), timevar = "Gene", direction = "wide")
names(d) <- sub("^Cq[.]", "", names(d))

target <- "fos-glo-myc"
ref    <- "Beta Actin"
control <- "0"

d$dCt  <- d[[target]] - d[[ref]]
d$ddCt <- d$dCt - mean(d$dCt[d$Group == control])
d$expr <- 2^(-d$ddCt)
d <- d[order(d$Group, d$BioRep), ]

groups <- as.character(sort(unique(d$Group)))   # 字符型：避免 [[0]] 这类数值下标
ref_mean <- sapply(groups, function(g) mean(d$expr[d$Group == g]))
ref_sd   <- sapply(groups, function(g) sd(d$expr[d$Group == g]))

# Welch t 检验（对照组 vs 其余各组），与应用的"t-test (Welch)"口径一致
ref_p <- sapply(setdiff(groups, control), function(g)
    t.test(d$expr[d$Group == g], d$expr[d$Group == control])$p.value)

# 单因素方差分析 + Tukey HSD，并折算成字母标记（与应用的 ANOVA 路径口径一致）
fit   <- aov(expr ~ factor(Group, levels = groups), data = d)
ref_anova_p <- summary(fit)[[1]][["Pr(>F)"]][1]
thsd  <- TukeyHSD(fit)
thsd_pairs <- data.frame(
    pair = rownames(thsd$`factor(Group, levels = groups)`),
    p    = thsd$`factor(Group, levels = groups)`[, "p adj"],
    stringsAsFactors = FALSE)

# 字母标记：Piepho (2004) 的 insert-and-absorb 算法
#   ① 按均值从大到小处理，每处理一个组先单独开一列；
#   ② 再把它并入所有"与列内各组都未达显著"的列；
#   ③ 最后删除被其它列完全覆盖的冗余列。
# 结果满足"组间不显著 ⇔ 至少共用一个字母"。
# 注：只做"贪心并入已有列、不做③"会得到 b/a/a 这种错标记——0 与 0.5 明明不显著却
#     不共用字母（2026-10-15 实测过这个错误版本，故记在此处）。
letter_groups <- function(means, pairs, alpha = 0.05) {
    ord <- names(sort(means, decreasing = TRUE))
    sig <- function(a, b) {
        key1 <- paste(a, b, sep = "-")
        key2 <- paste(b, a, sep = "-")
        hit <- pairs$p[pairs$pair == key1 | pairs$pair == key2]
        if (length(hit) == 0) return(FALSE)     # 未列出的比较（含自身）视为不显著
        hit[1] < alpha
    }
    columns <- list()
    for (g in ord) {
        columns[[length(columns) + 1]] <- g        # ① 先单开一列
        for (i in seq_along(columns)) {            # ② 并入所有兼容列
            if (g %in% columns[[i]]) next          # 自己那列不再重复追加
            if (all(!vapply(columns[[i]], function(h) sig(g, h), logical(1)))) {
                columns[[i]] <- c(columns[[i]], g)
            }
        }
    }
    # ③ 吸收：删掉"被另一列完全包含"的列（注意是包含于**某一列**，
    #    不是包含于其它列的并集——后者会把唯一区分 0 与 0.5 的那列也删掉）
    repeat {
        n <- length(columns); drop <- NULL
        for (i in seq_len(n)) {
            for (j in seq_len(n)) {
                if (i == j) next
                if (length(columns[[i]]) <= length(columns[[j]]) &&
                    all(columns[[i]] %in% columns[[j]])) { drop <- i; break }
            }
            if (!is.null(drop)) break
        }
        if (is.null(drop)) break
        columns <- columns[-drop]
    }
    letters_by_group <- setNames(rep("", length(means)), names(means))
    for (i in seq_along(columns))
        for (g in columns[[i]])
            letters_by_group[[g]] <- paste0(letters_by_group[[g]], letters[i])
    letters_by_group
}
ref_letters <- letter_groups(ref_mean, thsd_pairs)

cat("\n--- 独立参照（base R，零依赖）---\n")
for (g in groups)
    cat(sprintf("  group %-4s mean=%.10f sd=%.10f letter=%s\n",
                g, ref_mean[[g]], ref_sd[[g]], ref_letters[[g]]))
for (nm in names(ref_p))
    cat(sprintf("  Welch p (0 vs %-4s) = %.10f\n", nm, ref_p[[nm]]))
cat(sprintf("  ANOVA p = %.10f\n", ref_anova_p))
cat("  TukeyHSD p adj:\n"); print(thsd_pairs, row.names = FALSE)

# ---------------------------------------------------------------------------
# 2) 应用侧（C++ 1.2.0）输出
# ---------------------------------------------------------------------------
cpp_t <- read.csv("validation/results/cpp_deltadeltaCt_v12.csv", stringsAsFactors = FALSE)
cpp_a <- read.csv("validation/results/cpp_deltadeltaCt_anova_v12.csv", stringsAsFactors = FALSE)

rows <- list()
add <- function(quantity, key, a, b, tol, tol_type = "rel") {
    diff <- abs(a - b)
    rel  <- if (abs(b) > 0) diff / abs(b) else diff
    pass <- if (tol_type == "rel") rel <= tol else diff <= tol
    rows[[length(rows) + 1]] <<- data.frame(
        method = "deltadeltaCt", quantity = quantity, key = key,
        app_cpp = format(a, digits = 12), independent_base_r = format(b, digits = 12),
        abs_diff = diff, rel_diff = rel, agrees = pass, stringsAsFactors = FALSE)
    pass
}

for (g in groups) {
    ct <- cpp_t[cpp_t$Group == g, ]
    ok_mean <- add("mean", g, ct$Mean[1], ref_mean[[g]], 1e-12)
    ok_sd   <- add("sd",   g, ct$StdDev[1], ref_sd[[g]], 1e-12)
    note(ok_mean && ok_sd, sprintf("组 %s 的 mean / sd 与独立参照一致", g))
}
for (g in names(ref_p)) {
    ct <- cpp_t[cpp_t$Group == g, ]
    note(add("welch_p_vs_control", paste0(control, "_vs_", g), ct$PValue[1], ref_p[[g]], 1e-6),
         sprintf("Welch p（%s vs %s）与独立参照一致", control, g))
}
ca <- cpp_a[cpp_a$Group == groups[1], ]
note(add("anova_p", "all_groups", ca$PValue[1], ref_anova_p, 1e-6),
     "ANOVA 整体 p 与独立参照一致")
for (g in groups) {
    ca_g <- cpp_a[cpp_a$Group == g, ]
    same <- identical(trimws(ca_g$Significance[1]), trimws(ref_letters[[g]]))
    rows[[length(rows) + 1]] <- data.frame(
        method = "deltadeltaCt", quantity = "tukey_letter", key = g,
        app_cpp = ca_g$Significance[1], independent_base_r = ref_letters[[g]],
        abs_diff = NA_real_, rel_diff = NA_real_, agrees = same, stringsAsFactors = FALSE)
    note(same, sprintf("Tukey 字母（组 %s）一致：应用 %s / 参照 %s",
                       g, ca_g$Significance[1], ref_letters[[g]]))
}

out <- do.call(rbind, rows)
dir.create("validation/results", showWarnings = FALSE, recursive = TRUE)
write.csv(out, "validation/results/equivalence_stats.csv", row.names = FALSE)
cat("\nWritten: validation/results/equivalence_stats.csv\n")
cat(if (ok) "\nSTATS-VERIFY PASS\n" else "\nSTATS-VERIFY FAIL\n")
quit(status = if (ok) 0 else 1)
