#!/usr/bin/env Rscript
# =============================================================================
# 生成 qPCRtools_cpp 测试用的"黄金参考"（golden fixtures）。
# 用 base R stats / lm() 计算 C++ 实现应当复现的期望值。
# 可重跑（在 code/ 目录下）： Rscript tests/generate_reference.R
# 输出： tests/fixtures/{data.json, expected_stat.json, expected_standard_curve.json}
# =============================================================================
suppressPackageStartupMessages(library(jsonlite))

# qPCRtools 是可选的：只有表达量部分（CalExp2dCt/CalExp2ddCt/CalExpRqPCR）需要它。
# 未安装时仍可重新生成统计类 fixture（data.json / expected_stat.json /
# expected_standard_curve.json），表达量部分会打印 Skipped 并跳过。
has_qpcr <- requireNamespace("qPCRtools", quietly = TRUE)
if (has_qpcr) {
  suppressPackageStartupMessages(library(qPCRtools))
} else {
  message("qPCRtools not installed -> 表达量类 fixture 将跳过（其余仍会生成）")
}

# 期望值来自本机 base R，须记录版本以便复现（2026-10-15 实测 R 4.6.1）
cat("R version:", R.version.string, "\n")

fix_dir <- file.path("tests", "fixtures")
dir.create(fix_dir, showWarnings = FALSE, recursive = TRUE)

J <- function(x, file) write_json(x, file.path(fix_dir, file),
                                  auto_unbox = TRUE, digits = NA, pretty = TRUE)

# -----------------------------------------------------------------------------
# 输入数据集（单一事实源：R 与 C++ 共用同一份 data.json，避免漂移）
# -----------------------------------------------------------------------------
data <- list(
  ttest = list(
    list(id = "basic",   g1 = c(1, 2, 3, 4, 5),   g2 = c(2, 4, 6, 8, 10)),
    list(id = "similar", g1 = c(5.1, 5.0, 4.9, 5.2, 4.8),
                         g2 = c(5.0, 4.9, 5.1, 5.2, 5.0))
  ),
  paired = list(
    list(id = "shift", before = c(1, 2, 3, 4, 5), after = c(3, 5, 4, 7, 6))
  ),
  wilcox = list(
    list(id = "shifted",   g1 = c(1, 2, 3, 4, 5), g2 = c(6, 7, 8, 9, 10)),
    list(id = "ties1",     g1 = c(1, 2, 2, 3, 4), g2 = c(2, 2, 3, 3, 5)),
    list(id = "ties2",     g1 = c(1, 1, 2, 2, 3), g2 = c(2, 2, 3, 3, 4)),
    list(id = "ties3",     g1 = c(1, 2, 2, 3, 3, 4, 4, 5),
                           g2 = c(2, 3, 3, 4, 4, 5, 5, 6)),
    list(id = "identical", g1 = c(2, 2, 3, 3, 4), g2 = c(2, 2, 3, 3, 4))
  ),
  # 2026-10-15：以下 9 个用例此前只存在于 fixture 里（生成脚本无对应条目，属两套口径），
  # 现全部补回脚本，使 fixture 可由本脚本一键重生成。
  wilcox_signed = list(
    list(id = "signed1", before = c(1, 2, 3, 4, 5, 6), after = c(2, 3, 2, 6, 5, 8)),
    list(id = "signed2", before = c(1, 2, 3, 4, 5, 6), after = c(2, 3, 3, 6, 4, 8)),
    list(id = "signed3", before = c(1, 2, 3, 4, 5, 6, 7, 8),
                         after  = c(3, 4, 5, 6, 7, 8, 9, 10)),
    list(id = "signed4", before = c(5, 6, 7, 8, 9, 10), after = c(5, 6, 7, 8, 9, 10))
  ),
  anova = list(
    list(id = "3groups",
         groups = list(c(1, 2, 3), c(4, 5, 6), c(7, 8, 9)),
         names  = c("A", "B", "C"))
  ),
  # Tukey HSD（Tukey–Kramer）两两比较：期望值用 R 的 ptukey 生成，并与
  # TukeyHSD() 的 p adj 在脚本内交叉核对（见下方 tukey 段）。
  tukey = list(
    list(id = "three_groups",
         groups = list(c(1, 2, 3, 4), c(4, 5, 6, 4.5), c(8, 9, 7, 9.5)),
         names  = c("A", "B", "C")),
    list(id = "four_groups_unequal",
         groups = list(c(5, 6, 5.5, 6.5), c(6, 7, 6.5, 7.5, 6.8),
                       c(4.5, 5, 5.2), c(7, 8, 7.5, 8.5, 8.2, 7.9)),
         names  = c("CK", "T1", "T2", "T3")),
    list(id = "close_means",
         groups = list(c(10, 10.2, 9.8, 10.1), c(10.05, 9.95, 10.15, 10),
                       c(10.02, 10.08, 9.99, 10.04)),
         names  = c("A", "B", "C")),
    list(id = "wide_spread",
         groups = list(c(1, 1.5, 2, 1.2), c(5, 5.5, 6, 5.2), c(9, 9.5, 10, 9.2)),
         names  = c("A", "B", "C"))
  ),
  standard_curve = list(
    list(id = "perfect", conc = c(16, 4, 1, 0.25), cq = c(14, 17, 20, 23), dilution = 4),
    list(id = "noisy",   conc = c(16, 4, 1, 0.25), cq = c(14.2, 16.8, 20.3, 22.7), dilution = 4)
  )
)
J(data, "data.json")

# -----------------------------------------------------------------------------
# 统计检验期望值
# -----------------------------------------------------------------------------
stat <- list(
  ttest = lapply(data$ttest, function(c) {
    welch   <- t.test(c$g1, c$g2, var.equal = FALSE)   # C++ tTest(equalVariance=false)
    student <- t.test(c$g1, c$g2, var.equal = TRUE)    # C++ tTest(equalVariance=true)
    list(id = c$id,
         welch   = list(statistic = unname(welch$statistic),   pvalue = welch$p.value),
         student = list(statistic = unname(student$statistic), pvalue = student$p.value))
  }),
  paired = lapply(data$paired, function(c) {
    # C++ 用 after - before 作为差值；R t.test(x, y) 用 x - y，故传 (after, before)
    tt <- t.test(c$after, c$before, paired = TRUE)
    list(id = c$id, statistic = unname(tt$statistic), pvalue = tt$p.value)
  }),
  wilcox = lapply(data$wilcox, function(c) {
    # 2026-10-15：C++ 已改为与 R 默认一致 —— 小样本（n1,n2 ≤ 25）走精确分布、
    # 含并列值同样给精确 p（对应新版 R），更大样本才用正态近似。因此这里直接用
    # R 的默认调用（exact = NULL），期望值即 R 的默认输出。
    w <- suppressWarnings(wilcox.test(c$g1, c$g2))
    # statistic 与 C++ 的 U1 定义一致（R 的 W = group1 秩和 − n1(n1+1)/2）
    list(id = c$id, statistic = unname(w$statistic), pvalue = w$p.value)
  }),
  wilcox_signed = lapply(data$wilcox_signed, function(c) {
    # 符号秩检验（配对）：C++ 侧目前仍为正态近似（并列方差校正、无连续性校正），
    # 故用 exact = FALSE, correct = FALSE 生成期望值以与实现一致；
    # 全零差值（signed4）R 给 p = NaN、C++ 返回 NaN，fixture 统一写成字符串 "NaN"
    # （测试见到字符串即断言双方均为 NaN）。
    w <- suppressWarnings(wilcox.test(c$before, c$after, paired = TRUE,
                                      exact = FALSE, correct = FALSE))
    if (is.nan(w$p.value) || is.na(w$p.value)) {
      list(id = c$id, statistic = "NaN", pvalue = "NaN")
    } else {
      list(id = c$id, statistic = unname(w$statistic), pvalue = w$p.value)
    }
  }),
  anova = lapply(data$anova, function(c) {
    vals <- unlist(c$groups)
    grp  <- factor(rep(c$names, sapply(c$groups, length)))
    tab  <- summary(aov(vals ~ grp))[[1]]   # Df Sum Sq Mean Sq F value Pr(>F)
    list(id = c$id, statistic = unname(tab[1, 4]), pvalue = unname(tab[1, 5]))
  }),
  # Tukey HSD（Tukey–Kramer）两两比较。
  # 统计量与 C++ 的定义一致：Q = |mean_i − mean_j| / sqrt(MSE/2·(1/n_i + 1/n_j))，
  # p = ptukey(Q, k, dfWithin, lower.tail = FALSE)；脚本内与 TukeyHSD()$`p adj`
  # 逐对核对（差异打印在末尾的汇总里）。
  # 说明：这里刻意选用 dfWithin ≥ 6 的数据集 —— R 的 ptukey 在极小自由度处精度有限
  # （实测 df = 2、k = 2、q = 12 时 R 给 0.0134441，高精度积分给 0.0136061，
  #  2e7 次蒙特卡洛给 0.013639 ± 0.000051），该极端情形另在 C++ 测试中单独覆盖。
  tukey = local({
    out <- list()
    max_diff <- 0
    for (c in data$tukey) {
      vals  <- unlist(c$groups)
      grp   <- factor(rep(c$names, sapply(c$groups, length)))
      fit   <- aov(vals ~ grp)
      tab   <- summary(fit)[[1]]
      mse   <- tab["Residuals", "Mean Sq"]
      dfw   <- tab["Residuals", "Df"]
      k     <- length(c$groups)
      means <- sapply(c$groups, mean)
      ns    <- sapply(c$groups, length)
      ref   <- TukeyHSD(fit)[[1]]              # p adj 交叉核对用

      comps <- list()
      for (i in seq_len(k - 1)) {
        for (j in seq.int(i + 1, k)) {
          qstat <- abs(means[i] - means[j]) / sqrt(mse / 2 * (1 / ns[i] + 1 / ns[j]))
          pval  <- ptukey(qstat, k, dfw, lower.tail = FALSE)
          key   <- paste0(c$names[j], "-", c$names[i])   # TukeyHSD 行名顺序
          if (key %in% rownames(ref)) max_diff <- max(max_diff, abs(ref[key, "p adj"] - pval))
          comps[[length(comps) + 1]] <- list(
            g1 = c$names[i], g2 = c$names[j],
            statistic = qstat, pvalue = pval
          )
        }
      }
      out[[length(out) + 1]] <- list(
        id = c$id, dfWithin = unname(dfw), mse = unname(mse), comparisons = comps
      )
    }
    cat(sprintf("Tukey: ptukey 与 TukeyHSD p adj 的最大差异 = %.3e\n", max_diff))
    out
  })
)
J(stat, "expected_stat.json")

# -----------------------------------------------------------------------------
# 标准曲线期望值（C++ 用 Cq ~ log_dilution(Conc) 回归；此处复现同一 x 变换）
# -----------------------------------------------------------------------------
sc <- lapply(data$standard_curve, function(c) {
  x    <- log(c$conc) / log(c$dilution)   # 复现 C++ logConcentrations
  fit  <- lm(c$cq ~ x)
  smry <- summary(fit)
  slope     <- unname(coef(fit)[2])
  intercept <- unname(coef(fit)[1])
  list(
    id         = c$id,
    slope      = slope,
    intercept  = intercept,
    rSquared   = unname(smry$r.squared),
    efficiency = c$dilution^(-1.0 / slope) - 1.0,
    # 注意：C++ 的 pValue 是粗略阶跃近似（见 StandardCurve.cpp:251），并非真实 p 值，
    # 故 C++ 测试不对比 pValue；这里写真实值仅作记录。
    pValue_real = unname(smry$coefficients[2, 4])
  )
})
J(sc, "expected_standard_curve.json")

# -----------------------------------------------------------------------------
# ExpressionCalculator ΔCt vs R qPCRtools::CalExp2dCt（examples 数据）
# expre = 2^-(targetCq - refMeanCq)，mean.expre 为各 group 的均值。
# 每个 gene 取其各 group 的 mean.expre 升序列表，C++ 按同方式收集 table.Mean 对比。
# -----------------------------------------------------------------------------
cq_tb     <- read.csv("examples/cq.csv",     check.names = FALSE)
design_tb <- read.csv("examples/design.csv", check.names = FALSE)
ref_gene  <- "Beta Actin"
dct <- tryCatch(
  CalExp2dCt(cq_table = cq_tb, design_table = design_tb, ref_gene = ref_gene),
  error = function(e) { message("CalExp2dCt ERROR: ", conditionMessage(e)); NULL }
)
if (!is.null(dct)) {
  by_gene <- lapply(split(dct, dct$gene), function(d) sort(unique(d$mean.expre)))
  J(list(ref_gene = ref_gene, by_gene = by_gene), "expected_expression_dct.json")
} else {
  message("Skipped expected_expression_dct.json (CalExp2dCt failed)")
}

# -----------------------------------------------------------------------------
# ExpressionCalculator ΔΔCt vs R qPCRtools::CalExp2ddCt（examples 数据）
# 关键：R 的 CalExp2ddCt 用「全数据均值」做归一化基准（平均所有 group 的 target/ref，
# 不是只取对照组），故对照组表达量 ≠ 1。C++ 端必须用同一基准，否则与对照组基准相差一个
# 常数因子。remove_outliers=FALSE：C++ 不做 IQR 离群值剔除，此处对齐以隔离归一化对比。
# -----------------------------------------------------------------------------
ddct <- tryCatch(
  CalExp2ddCt(cq_table = cq_tb, design_table = design_tb,
              ref_gene = ref_gene, ref_group = "0", remove_outliers = FALSE),
  error = function(e) { message("CalExp2ddCt ERROR: ", conditionMessage(e)); NULL }
)
if (!is.null(ddct)) {
  ddct_t <- ddct[["table"]]
  by_gene_ddct <- lapply(split(ddct_t, ddct_t$gene), function(d) sort(unique(d$mean.expre)))
  J(list(ref_gene = ref_gene, by_gene = by_gene_ddct), "expected_expression_ddct.json")
} else {
  message("Skipped expected_expression_ddct.json (CalExp2ddCt failed)")
}

# -----------------------------------------------------------------------------
# ExpressionCalculator 标准曲线(效率法) vs R qPCRtools::CalExpRqPCR（examples/rqpcr_*.csv）
# 注意：这是 CalExpRqPCR（Eff 法，QCq=Eff^(min.meanCq-meanCq)、参考基因几何均值校正、
# 按基因内最小组均值归一化），不是 CalExpCurve（斜率截距法）。ref_gene 固定为 OsUBQ
# 以隔离表达量计算（避免 GeNorm 自动选择引入额外变量）。
# -----------------------------------------------------------------------------
rqpcr_cq     <- read.csv("examples/rqpcr_cq.csv",     check.names = FALSE)
rqpcr_design <- read.csv("examples/rqpcr_design.csv", check.names = FALSE)
rqpcr <- tryCatch(
  CalExpRqPCR(cq_table = rqpcr_cq, design_table = rqpcr_design,
              ref_gene = "OsUBQ", ref_group = "CK", stat_method = "t.test"),
  error = function(e) { message("CalExpRqPCR ERROR: ", conditionMessage(e)); NULL }
)
if (!is.null(rqpcr)) {
  rqpcr_t <- rqpcr[["table"]]
  by_gene_rqpcr <- lapply(split(rqpcr_t, rqpcr_t$gene), function(d) sort(unique(d$Expression)))
  J(list(ref_gene = "OsUBQ", by_gene = by_gene_rqpcr), "expected_expression_rqpcr.json")
} else {
  message("Skipped expected_expression_rqpcr.json (CalExpRqPCR failed)")
}

cat("OK -> wrote fixtures to", normalizePath(fix_dir), "\n")
cat("  ttest:", length(stat$ttest),
    " paired:", length(stat$paired),
    " wilcox:", length(stat$wilcox),
    " anova:", length(stat$anova),
    " tukey:", length(stat$tukey),
    " standard_curve:", length(sc),
    " expression_dct:", if (!is.null(dct)) length(by_gene) else 0,
    " expression_ddct:", if (!is.null(ddct)) length(by_gene_ddct) else 0,
    " expression_rqpcr:", if (!is.null(rqpcr)) length(by_gene_rqpcr) else 0, "\n")
