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
# geNorm 参考基因选择的参考实现（C++ selectReferenceGenesByGeNorm 的对照）
#   · use_relative = TRUE：按 geNorm 原文，先在**相对量** Q = 2^(minCq − Cq) 上算
#     （效率按 2；等价于看两个基因 Cq 差值在样品间的波动）；
#     FALSE：旧实现的 log2(原始 Cq 相除)，用来演示两者差异。
#   · stepwise = TRUE：逐步剔除 M 最大的基因直到剩 2 个（geNorm 原法）；
#     FALSE：只算一轮 M 取最小的两个（旧 C++ 行为）。
# 长表 cq_long 需含 Gene / Group / BioRep / TechRep / Cq 五列。
# 说明：R 包 qPCRtools::find_ref_gene() 用的是「原始 Cq + 逐步剔除」这一组合。
# -----------------------------------------------------------------------------
geNormSelect <- function(cq_long, use_relative = TRUE, stepwise = TRUE) {
  genes <- unique(cq_long$Gene)
  keys  <- paste(cq_long$Group, cq_long$BioRep, cq_long$TechRep)
  skeys <- unique(keys)
  wide  <- matrix(NA_real_, length(genes), length(skeys),
                  dimnames = list(genes, skeys))
  wide[cbind(match(cq_long$Gene, genes), match(keys, skeys))] <- cq_long$Cq

  remaining <- genes
  first_M   <- NULL
  repeat {
    M <- sapply(remaining, function(g) {
      sds <- sapply(setdiff(remaining, g), function(k) {
        d <- if (use_relative) {
          (wide[g, ] - min(wide[g, ], na.rm = TRUE)) -
            (wide[k, ] - min(wide[k, ], na.rm = TRUE))
        } else {
          log2(wide[g, ] / wide[k, ])
        }
        stats::sd(d, na.rm = TRUE)
      })
      mean(sds, na.rm = TRUE)
    })
    if (is.null(first_M)) first_M <- M
    if (!stepwise || length(remaining) <= 2) break
    remaining <- setdiff(remaining, remaining[which.max(M)[1]])
  }
  list(selected = if (stepwise) remaining else names(sort(M))[1:2],
       first_M  = first_M)
}

# -----------------------------------------------------------------------------
# RqPCR（SATQPCR / qPCRtools::CalExpRqPCR）参考实现
#   · dedupValues = TRUE ：复现 R 包 CalExpRqPCR 的口径 —— 组内汇总用
#     mean(unique(expression))、sd(unique(expression))、se 除以 unique(biorep) 数。
#   · dedupValues = FALSE：修正口径 —— 数值相同但来自不同样品的重复仍是独立观测，
#     组内均值/标准差对**所有生物学重复**计算，se = sd/sqrt(n_biorep)。
#   · dedupRows：是否按（TechRep, Cq）剔除完全重复的行（数据整理产物）。
# 已核对：dedupValues = TRUE 时本实现与 R 包生成的旧 fixture 完全一致（~1e-12），
# 故可作为不依赖 qPCRtools 包的参考实现。
# -----------------------------------------------------------------------------
rqpcrReference <- function(cqTable, designTable, refGenes,
                           dedupValues = FALSE, dedupRows = TRUE) {
  df <- merge(designTable, cqTable, by = "Position")

  # 1) 每个 (Group, BioRep, Gene) 的平均 Cq 与 SD（技术重复平均）
  keys <- unique(df[, c("Group", "Gene", "BioRep")])
  perRep <- do.call(rbind, lapply(seq_len(nrow(keys)), function(i) {
    k   <- keys[i, ]
    sel <- df$Group == k$Group & df$Gene == k$Gene & df$BioRep == k$BioRep
    sub <- df[sel, ]
    if (dedupRows) sub <- sub[!duplicated(paste(sub$TechRep, sub$Cq, sep = "|")), ]
    cqs <- sub$Cq
    data.frame(Group = k$Group, Gene = k$Gene, BioRep = k$BioRep,
               mean.cq = mean(cqs, na.rm = TRUE),
               sd.cq   = if (length(cqs) > 1) stats::sd(cqs, na.rm = TRUE) else 0,
               Eff     = sub$Eff[1], stringsAsFactors = FALSE)
  }))
  perRep$sd.cq[is.na(perRep$sd.cq)] <- 0

  # 2) min.mean.cq = 同一 BioRep 内、跨 Group 的最小平均 Cq（R 包的"编号配对"口径）
  perRep$min.mean.cq <- ave(perRep$mean.cq,
                            paste(perRep$BioRep, perRep$Gene, sep = "|"), FUN = min)
  perRep$QCq    <- perRep$Eff^(perRep$min.mean.cq - perRep$mean.cq)
  perRep$SD_QCq <- perRep$sd.cq * perRep$QCq * log(perRep$Eff)

  # 3) 归一化因子：每个 (Group, BioRep) 上参考基因 QCq 的几何均值（广播到所有基因）
  gm    <- function(x) exp(mean(log(x)))
  isref <- perRep$Gene %in% refGenes
  k3    <- paste(perRep$Group, perRep$BioRep, sep = "|")
  facKeys <- unique(k3[isref])
  facByKey <- vapply(facKeys, function(k) gm(perRep$QCq[isref & k3 == k]), numeric(1))
  names(facByKey) <- facKeys
  perRep$factor <- facByKey[match(k3, names(facByKey))]

  # 4) 校正表达量
  perRep$expression <- perRep$QCq / perRep$factor

  # 5) 组内汇总
  gkeys <- unique(df[, c("Group", "Gene")])
  groupDf <- do.call(rbind, lapply(seq_len(nrow(gkeys)), function(i) {
    g    <- gkeys[i, ]
    sel  <- perRep$Group == g$Group & perRep$Gene == g$Gene
    vals <- perRep$expression[sel]
    nRep <- sum(sel)
    if (dedupValues) {
      vals <- unique(vals)
      nRep <- length(unique(perRep$BioRep[sel]))
    }
    s <- if (length(vals) > 1) stats::sd(vals, na.rm = TRUE) else 0
    data.frame(Group = g$Group, Gene = g$Gene,
               mean.expr = mean(vals, na.rm = TRUE),
               sd.expr   = s, se.expr = s / sqrt(nRep),
               stringsAsFactors = FALSE)
  }))

  # 6) 按基因内最小 group 均值归一化
  groupDf <- do.call(rbind, lapply(split(groupDf, groupDf$Gene), function(sub) {
    m <- min(sub$mean.expr)
    sub$mean.expr <- sub$mean.expr / m
    sub$sd.expr   <- sub$sd.expr / m
    sub$se.expr   <- sub$se.expr / m
    sub
  }))
  rownames(groupDf) <- NULL
  groupDf
}

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
# geNorm 参考基因选择的小数据集（5 基因 × 5 样品，长表）
# 这组数值刻意挑成"能区分两种错误"：正确的 geNorm（相对量 + 逐步剔除）选 G1/G3；
# 用原始 Cq 相除、或只算一轮 M 取最小的两个，都会选成 G1/G5。写出 CSV 供 R 与 C++ 共用。
# -----------------------------------------------------------------------------
genorm_matrix <- matrix(c(
  26.411, 25.065, 25.609, 25.431, 26.337,   # G1
  23.339, 26.595, 19.419, 20.290, 22.437,   # G2
  17.131, 20.124, 20.468, 17.561, 19.897,   # G3
  21.501, 25.130, 14.908, 28.449, 29.787,   # G4
  29.003, 28.961, 27.464, 31.109, 27.225    # G5
), nrow = 5, byrow = TRUE,
   dimnames = list(paste0("G", 1:5), paste0("S", 1:5)))

genorm_table <- data.frame(
  Gene    = rep(rownames(genorm_matrix), times = ncol(genorm_matrix)),
  Group   = "G",
  BioRep  = rep(colnames(genorm_matrix), each = nrow(genorm_matrix)),
  TechRep = "1",
  Cq      = as.numeric(genorm_matrix)
)
write.csv(genorm_table, file.path(fix_dir, "data_genorm.csv"),
          row.names = FALSE, quote = FALSE)

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
  }),
  # geNorm 参考基因选择：读入上面写出的 data_genorm.csv，按 geNorm 原文
  # （相对量 Q = 2^(minCq − Cq) + 逐步剔除最不稳定基因）独立实现一份参考。
  # 同时记录另三种组合，便于说明与 R 包 qPCRtools::find_ref_gene()（原始 Cq 相除 + 逐步）
  # 以及旧 C++ 实现（原始 Cq + 只算一轮 M）的差别。
  genorm = local({
    a <- geNormSelect(genorm_table, use_relative = TRUE, stepwise = TRUE)
    list(list(id                        = "discriminating",
              selected                  = as.character(a$selected),
              first_round_M             = as.numeric(round(a$first_M, 6)),
              variant_relative_one_round = as.character(geNormSelect(genorm_table, TRUE,  FALSE)$selected),
              variant_raw_stepwise       = as.character(geNormSelect(genorm_table, FALSE, TRUE)$selected),
              variant_raw_one_round      = as.character(geNormSelect(genorm_table, FALSE, FALSE)$selected)))
  }),
  # 同一套算法在论文示例数据（examples/rqpcr_*.csv，96 孔）上的结果 —— 真实数据核对
  genorm_example = local({
    ex_cq  <- read.csv("examples/rqpcr_cq.csv",     check.names = FALSE)
    ex_des <- read.csv("examples/rqpcr_design.csv", check.names = FALSE)
    ex     <- merge(ex_des, ex_cq, by = "Position")
    a      <- geNormSelect(ex, use_relative = TRUE, stepwise = TRUE)
    list(list(id                        = "rqpcr_example",
              n_genes                   = length(unique(ex$Gene)),
              n_samples                 = length(unique(paste(ex$Group, ex$BioRep, ex$TechRep))),
              selected                  = as.character(a$selected),
              variant_relative_one_round = as.character(geNormSelect(ex, TRUE,  FALSE)$selected),
              variant_raw_stepwise       = as.character(geNormSelect(ex, FALSE, TRUE)$selected),
              variant_raw_one_round      = as.character(geNormSelect(ex, FALSE, FALSE)$selected)))
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
# ExpressionCalculator 标准曲线(效率法) RqPCR 的组均值（examples/rqpcr_*.csv）
# 期望值来自本文件的 rqpcrReference()（不依赖 qPCRtools 包）：
#   · by_gene            = 修正口径 —— 组内均值/标准差对**所有**生物学重复计算
#   · by_gene_r_original = R 包 CalExpRqPCR 口径 —— mean(unique(expression))、se 除以 unique(biorep) 数
# 两者在示例数据上不同（最大相对差 ~10%），属**有意偏离**：R 的按数值去重会把数值碰巧相同的
# 独立生物学重复合并掉（导师 2026-10-14 意见第 8 条）。已核对 by_gene_r_original 与 R 包旧
# fixture 完全一致（~1e-12），说明本参考实现忠实复现了 R 包口径。
# 注意：这是 CalExpRqPCR（Eff 法，QCq=Eff^(min.meanCq-meanCq)、参考基因几何均值校正、
# 按基因内最小组均值归一化），不是 CalExpCurve（斜率截距法）。ref_gene 固定为 OsUBQ
# 以隔离表达量计算（避免参考基因自动选择引入额外变量）。
# -----------------------------------------------------------------------------
rqpcr_cq       <- read.csv("examples/rqpcr_cq.csv",     check.names = FALSE)
rqpcr_design   <- read.csv("examples/rqpcr_design.csv", check.names = FALSE)
rqpcr_ref_gene <- "OsUBQ"
rqpcr_fixed <- rqpcrReference(rqpcr_cq, rqpcr_design, rqpcr_ref_gene,
                              dedupValues = FALSE, dedupRows = TRUE)
rqpcr_rpkg  <- rqpcrReference(rqpcr_cq, rqpcr_design, rqpcr_ref_gene,
                              dedupValues = TRUE,  dedupRows = FALSE)
# 与 R 包一致：输出表只含目标基因（参考基因不进结果表）
rqpcr_fixed <- rqpcr_fixed[!(rqpcr_fixed$Gene %in% rqpcr_ref_gene), ]
rqpcr_rpkg  <- rqpcr_rpkg[!(rqpcr_rpkg$Gene %in% rqpcr_ref_gene), ]
J(list(ref_gene = rqpcr_ref_gene,
       by_gene            = lapply(split(rqpcr_fixed, rqpcr_fixed$Gene), function(d) sort(d$mean.expr)),
       by_gene_r_original = lapply(split(rqpcr_rpkg,  rqpcr_rpkg$Gene),  function(d) sort(d$mean.expr))),
  "expected_expression_rqpcr.json")
cat("RqPCR fixture: ", length(unique(rqpcr_fixed$Gene)), "个目标基因（by_gene = 修正口径，by_gene_r_original = R 包口径）\n")

cat("OK -> wrote fixtures to", normalizePath(fix_dir), "\n")
cat("  ttest:", length(stat$ttest),
    " paired:", length(stat$paired),
    " wilcox:", length(stat$wilcox),
    " anova:", length(stat$anova),
    " tukey:", length(stat$tukey),
    " genorm:", length(stat$genorm),
    " standard_curve:", length(sc),
    " expression_dct:", if (!is.null(dct)) length(by_gene) else 0,
    " expression_ddct:", if (!is.null(ddct)) length(by_gene_ddct) else 0,
    " expression_rqpcr:", if (!is.null(rqpcr_fixed)) length(unique(rqpcr_fixed$Gene)) else 0, "\n")
