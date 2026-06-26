#!/usr/bin/env Rscript
# =============================================================================
# 生成 qPCRtools_cpp 测试用的"黄金参考"（golden fixtures）。
# 用 base R stats / lm() 计算 C++ 实现应当复现的期望值。
# 可重跑： Rscript tests/generate_reference.R
# 输出： tests/fixtures/{data.json, expected_stat.json, expected_standard_curve.json}
# =============================================================================
suppressPackageStartupMessages(library(jsonlite))
suppressPackageStartupMessages(library(qPCRtools))  # CalExp2dCt 等表达量参考

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
    list(id = "shifted", g1 = c(1, 2, 3, 4, 5), g2 = c(6, 7, 8, 9, 10))
  ),
  anova = list(
    list(id = "3groups",
         groups = list(c(1, 2, 3), c(4, 5, 6), c(7, 8, 9)),
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
    # C++ 用正态近似、无连续性校正；令 R 采用相同方法以验证算术正确性
    w <- wilcox.test(c$g1, c$g2, exact = FALSE, correct = FALSE)
    list(id = c$id, pvalue = w$p.value)  # 统计量定义不同(min(U) vs W)，仅对比 p
  }),
  anova = lapply(data$anova, function(c) {
    vals <- unlist(c$groups)
    grp  <- factor(rep(c$names, sapply(c$groups, length)))
    tab  <- summary(aov(vals ~ grp))[[1]]   # Df Sum Sq Mean Sq F value Pr(>F)
    list(id = c$id, statistic = unname(tab[1, 4]), pvalue = unname(tab[1, 5]))
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

cat("OK -> wrote fixtures to", normalizePath(fix_dir), "\n")
cat("  ttest:", length(stat$ttest),
    " paired:", length(stat$paired),
    " wilcox:", length(stat$wilcox),
    " anova:", length(stat$anova),
    " standard_curve:", length(sc),
    " expression_dct:", if (!is.null(dct)) length(by_gene) else 0, "\n")
