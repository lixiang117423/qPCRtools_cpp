#!/usr/bin/env Rscript
# validation/run_R.R
#
# 用 R 版 qPCRtools 跑与 C++ driver (run_cpp) 完全相同的数据，输出 CSV 供逐项
# 对比（见 validation/compare.py）。这是论文 Results 中"与 R 版等价"的真实数据来源。
#
# 运行（仓库根目录）： Rscript validation/run_R.R
# 依赖： install.packages("qPCRtools")
#
# 说明：三套数据均来自本仓库（examples/ 与 web/），与 R 包内置 demo 同源。
#       C++ 侧 (run_cpp) 读取相同的文件，确保"同输入"。

library(qPCRtools)
dir.create("validation/results", showWarnings = FALSE, recursive = TRUE)

# ---------------------------------------------------------------------------
# 方法 1: ΔΔCt  —— CalExp2ddCt
# 数据：examples/cq.csv (Position,Gene,Cq) + examples/design.csv (Position,Group,BioRep)
# ---------------------------------------------------------------------------
cq_ddct     <- read.table("examples/cq.csv",     header = TRUE, sep = ",")
design_ddct <- read.table("examples/design.csv", header = TRUE, sep = ",")

# C++ examples 用 Group 列；若你安装的 R 版 CalExp2ddCt 期望 Treatment 列，
# 取消下一行注释（作者请按本地 ?qPCRtools::CalExp2ddCt 确认）：
# names(design_ddct)[names(design_ddct) == "Group"] <- "Treatment"

res_ddct <- CalExp2ddCt(
    cq_table     = cq_ddct,
    design_table = design_ddct,
    ref_gene     = "Beta Actin",
    ref_group    = "0",
    stat_method  = "t.test",
    fig_type     = "bar",
    fig_ncol     = NULL
)
write.csv(res_ddct[["table"]], "validation/results/R_deltadeltaCt.csv", row.names = FALSE)
cat("[R] ΔΔCt -> validation/results/R_deltadeltaCt.csv\n")

# ---------------------------------------------------------------------------
# 方法 2: RqPCR —— CalExpRqPCR
# 数据：examples/rqpcr_cq.csv + examples/rqpcr_design.csv (含 BioRep/TechRep/Eff)
# ---------------------------------------------------------------------------
cq_rq     <- read.table("examples/rqpcr_cq.csv",     header = TRUE, sep = ",")
design_rq <- read.table("examples/rqpcr_design.csv", header = TRUE, sep = ",")

res_rq <- CalExpRqPCR(
    cq_table     = cq_rq,
    design_table = design_rq,
    ref_gene     = NULL,
    ref_group    = "CK",
    stat_method  = "t.test",
    fig_type     = "bar",
    fig_ncol     = NULL
)
write.csv(res_rq[["table"]], "validation/results/R_rqpcr.csv", row.names = FALSE)
cat("[R] RqPCR -> validation/results/R_rqpcr.csv\n")

# ---------------------------------------------------------------------------
# 方法 3: 标准曲线 —— CalCurve
# 数据：web/calsc.cq.txt + web/calsc.info.txt (与 R 包 demo 同名同内容，4 倍稀释)
# ---------------------------------------------------------------------------
sc_cq   <- read.table("web/calsc.cq.txt",   header = TRUE)
sc_info <- read.table("web/calsc.info.txt", header = TRUE)

res_sc <- CalCurve(
    cq_table      = sc_cq,
    concen_table  = sc_info,
    lowest_concen  = 4,
    highest_concen = 4096,
    dilution      = 4,
    by_mean       = TRUE
)
write.csv(res_sc[["table"]], "validation/results/R_standardcurve.csv", row.names = FALSE)
cat("[R] standard curve -> validation/results/R_standardcurve.csv\n")

cat("[R] done.\n")
