#!/usr/bin/env Rscript
# Independent base-R reference for the three methods implemented by qPCRtools.
#
# This script deliberately does not load qPCRtools. It reconstructs each
# quantity from the raw Cq tables and the method definitions, so that the
# comparison tests correctness rather than agreement with a previous version.
#
# Run from code/:
#   /opt/homebrew/bin/Rscript validation/reference_base_r.R
#
# Outputs (validation/results/):
#   reference_ddct_holm.csv   Delta-Delta-Ct means, SD, raw and Holm P values
#   reference_ddct_anova.csv  ANOVA and Tukey letter display
#   reference_rqpcr_holm.csv  efficiency-method group summaries and P values
#   reference_standardcurve.csv slopes, intercepts, R2 and efficiency

options(stringsAsFactors = FALSE)

results_dir <- "validation/results"
dir.create(results_dir, showWarnings = FALSE, recursive = TRUE)

holm_adjust <- function(p) {
    p.adjust(p, method = "holm")
}

geometric_mean <- function(x) {
    exp(mean(log(x)))
}

read_csv <- function(path) {
    read.csv(path, check.names = FALSE)
}

# ---------------------------------------------------------------------------
# 1. Delta-Delta-Ct
# ---------------------------------------------------------------------------
ddct_reference <- function(cq, design, reference_gene, control_group) {
    merged <- merge(design, cq, by = "Position")
    target_genes <- setdiff(unique(merged$Gene), reference_gene)
    rows <- list()

    for (gene in target_genes) {
        sub <- merged[merged$Gene %in% c(gene, reference_gene), ]

        # Technical replicates are averaged within each biological sample.
        keys <- unique(sub[, c("Group", "BioRep", "Gene")])
        per_rep <- do.call(rbind, lapply(seq_len(nrow(keys)), function(i) {
            k <- keys[i, ]
            values <- sub$Cq[
                sub$Group == k$Group &
                sub$BioRep == k$BioRep &
                sub$Gene == k$Gene
            ]
            data.frame(
                Group = k$Group,
                BioRep = k$BioRep,
                Gene = k$Gene,
                mean_cq = mean(values, na.rm = TRUE),
                stringsAsFactors = FALSE
            )
        }))

        wide <- reshape(
            per_rep[, c("Group", "BioRep", "Gene", "mean_cq")],
            idvar = c("Group", "BioRep"),
            timevar = "Gene",
            direction = "wide"
        )
        names(wide) <- sub("^mean_cq[.]", "", names(wide))
        wide$dCt <- wide[[gene]] - wide[[reference_gene]]

        control_dct <- mean(
            wide$dCt[as.character(wide$Group) == as.character(control_group)],
            na.rm = TRUE
        )
        wide$expression <- 2^(-(wide$dCt - control_dct))
        wide$Gene <- gene

        groups <- sort(unique(as.character(wide$Group)))
        group_rows <- do.call(rbind, lapply(groups, function(group) {
            values <- wide$expression[as.character(wide$Group) == group]
            data.frame(
                Gene = gene,
                Group = group,
                Mean = mean(values, na.rm = TRUE),
                SD = stats::sd(values, na.rm = TRUE),
                n = sum(!is.na(values)),
                stringsAsFactors = FALSE
            )
        }))

        treated <- setdiff(groups, as.character(control_group))
        raw_p <- vapply(treated, function(group) {
            stats::t.test(
                wide$expression[as.character(wide$Group) == group],
                wide$expression[as.character(wide$Group) == as.character(control_group)],
                var.equal = FALSE
            )$p.value
        }, numeric(1))
        adjusted_p <- holm_adjust(raw_p)

        group_rows$RawPValue <- NA_real_
        group_rows$AdjustedPValue <- NA_real_
        group_rows$PAdjustMethod <- NA_character_
        for (group in treated) {
            idx <- group_rows$Group == group
            group_rows$RawPValue[idx] <- raw_p[[group]]
            group_rows$AdjustedPValue[idx] <- adjusted_p[[group]]
            group_rows$PAdjustMethod[idx] <- "holm"
        }
        group_rows$Significance <- ifelse(
            is.na(group_rows$AdjustedPValue),
            NA_character_,
            ifelse(
                group_rows$AdjustedPValue < 0.001, "***",
                ifelse(
                    group_rows$AdjustedPValue < 0.01, "**",
                    ifelse(group_rows$AdjustedPValue < 0.05, "*", "NS")
                )
            )
        )

    rows[[length(rows) + 1]] <- group_rows
    }

    do.call(rbind, rows)
}

cq_ddct <- read_csv("examples/cq.csv")
design_ddct <- read_csv("examples/design.csv")

reference_ddct <- ddct_reference(
    cq_ddct,
    design_ddct,
    reference_gene = "Beta Actin",
    control_group = "0"
)
write.csv(
    reference_ddct,
    file.path(results_dir, "reference_ddct_holm.csv"),
    row.names = FALSE
)

# Scale sensitivity: the same Welch comparisons on the Delta-Ct scale.
merged_scale <- merge(design_ddct, cq_ddct, by = "Position")
per_rep_scale <- aggregate(
    Cq ~ Group + BioRep + Gene,
    data = merged_scale,
    FUN = mean
)
wide_scale <- reshape(
    per_rep_scale,
    idvar = c("Group", "BioRep"),
    timevar = "Gene",
    direction = "wide"
)
names(wide_scale) <- sub("^Cq[.]", "", names(wide_scale))
wide_scale$dCt <- wide_scale[["fos-glo-myc"]] - wide_scale[["Beta Actin"]]
scale_groups <- setdiff(sort(unique(as.character(wide_scale$Group))), "0")
scale_p <- vapply(scale_groups, function(group) {
    stats::t.test(
        wide_scale$dCt[as.character(wide_scale$Group) == group],
        wide_scale$dCt[as.character(wide_scale$Group) == "0"],
        var.equal = FALSE
    )$p.value
}, numeric(1))
write.csv(
    data.frame(
        Group = scale_groups,
        RawPValue = unname(scale_p),
        HolmPValue = unname(holm_adjust(scale_p)),
        PAdjustMethod = "holm",
        stringsAsFactors = FALSE
    ),
    file.path(results_dir, "reference_ddct_dct_scale.csv"),
    row.names = FALSE
)

# ANOVA and Tukey HSD on the same Delta-Delta-Ct expression values.
merged_ddct <- merge(design_ddct, cq_ddct, by = "Position")
genes_ddct <- setdiff(unique(merged_ddct$Gene), "Beta Actin")
anova_rows <- list()
for (gene in genes_ddct) {
    sub <- merged_ddct[merged_ddct$Gene %in% c(gene, "Beta Actin"), ]
    per_rep <- aggregate(
        Cq ~ Group + BioRep + Gene,
        data = sub,
        FUN = mean
    )
    wide <- reshape(
        per_rep,
        idvar = c("Group", "BioRep"),
        timevar = "Gene",
        direction = "wide"
    )
    names(wide) <- sub("^Cq[.]", "", names(wide))
    wide$dCt <- wide[[gene]] - wide[["Beta Actin"]]
    control_dct <- mean(
        wide$dCt[as.character(wide$Group) == "0"],
        na.rm = TRUE
    )
    wide$expression <- 2^(-(wide$dCt - control_dct))
    wide$Group <- factor(
        wide$Group,
        levels = sort(unique(as.character(wide$Group)))
    )
    fit <- stats::aov(expression ~ Group, data = wide)
    anova_p <- summary(fit)[[1]][["Pr(>F)"]][1]
    tukey <- stats::TukeyHSD(fit)$Group
    tukey_df <- data.frame(
        comparison = rownames(tukey),
        adjusted_p = tukey[, "p adj"],
        stringsAsFactors = FALSE
    )
    anova_rows[[length(anova_rows) + 1]] <- data.frame(
        Gene = gene,
        ANOVAP = anova_p,
        TukeyComparisons = paste(
            paste0(tukey_df$comparison, "=", signif(tukey_df$adjusted_p, 8)),
            collapse = ";"
        ),
        stringsAsFactors = FALSE
    )
}
write.csv(
    do.call(rbind, anova_rows),
    file.path(results_dir, "reference_ddct_anova.csv"),
    row.names = FALSE
)

# ---------------------------------------------------------------------------
# 2. Efficiency-based method (RqPCR)
# ---------------------------------------------------------------------------
rqpcr_reference <- function(cq, design, reference_genes, control_group) {
    merged <- merge(design, cq, by = "Position")
    keys <- unique(merged[, c("Group", "Gene", "BioRep")])
    per_rep <- do.call(rbind, lapply(seq_len(nrow(keys)), function(i) {
        k <- keys[i, ]
        sub <- merged[
            merged$Group == k$Group &
            merged$Gene == k$Gene &
            merged$BioRep == k$BioRep,
        ]
        sub <- sub[!duplicated(sub[, c("TechRep", "Cq")]), ]
        data.frame(
            Group = k$Group,
            Gene = k$Gene,
            BioRep = k$BioRep,
            mean_cq = mean(sub$Cq, na.rm = TRUE),
            sd_cq = if (nrow(sub) > 1) stats::sd(sub$Cq, na.rm = TRUE) else 0,
            Eff = sub$Eff[1],
            stringsAsFactors = FALSE
        )
    }))
    per_rep$sd_cq[is.na(per_rep$sd_cq)] <- 0
    per_rep$min_mean_cq <- ave(
        per_rep$mean_cq,
        paste(per_rep$BioRep, per_rep$Gene, sep = "|"),
        FUN = min
    )
    per_rep$QCq <- per_rep$Eff^(per_rep$min_mean_cq - per_rep$mean_cq)
    per_rep$SD_QCq <- per_rep$sd_cq * per_rep$QCq * log(per_rep$Eff)

    ref_rows <- per_rep[per_rep$Gene %in% reference_genes, ]
    factors <- aggregate(
        QCq ~ Group + BioRep,
        data = ref_rows,
        FUN = geometric_mean
    )
    names(factors)[names(factors) == "QCq"] <- "factor"
    per_rep <- merge(per_rep, factors, by = c("Group", "BioRep"))
    per_rep$expression <- per_rep$QCq / per_rep$factor

    target_rows <- per_rep[!per_rep$Gene %in% reference_genes, ]
    group_rows <- do.call(rbind, lapply(
        split(target_rows, list(target_rows$Gene, target_rows$Group), drop = TRUE),
        function(part) {
            data.frame(
                Gene = part$Gene[1],
                Group = part$Group[1],
                Mean = mean(part$expression, na.rm = TRUE),
                SD = stats::sd(part$expression, na.rm = TRUE),
                n = sum(!is.na(part$expression)),
                stringsAsFactors = FALSE
            )
        }
    ))
    group_rows$Group <- as.character(group_rows$Group)
    minimum_mean <- ave(group_rows$Mean, group_rows$Gene, FUN = min)
    group_rows$Mean <- group_rows$Mean / minimum_mean
    group_rows$SD <- group_rows$SD / minimum_mean
    group_rows$SE <- group_rows$SD / sqrt(group_rows$n)

    p_rows <- list()
    for (gene in unique(group_rows$Gene)) {
        comparison_groups <- setdiff(
            unique(group_rows$Group[group_rows$Gene == gene]),
            as.character(control_group)
        )
        gene_p <- vapply(comparison_groups, function(group) {
            stats::t.test(
                target_rows$expression[
                    target_rows$Gene == gene &
                    as.character(target_rows$Group) == group
                ],
                target_rows$expression[
                    target_rows$Gene == gene &
                    as.character(target_rows$Group) == as.character(control_group)
                ],
                var.equal = FALSE
            )$p.value
        }, numeric(1))
        p_rows[[gene]] <- data.frame(
            Gene = gene,
            Group = comparison_groups,
            RawPValue = unname(gene_p),
            AdjustedPValue = unname(holm_adjust(gene_p)),
            stringsAsFactors = FALSE
        )
    }
    p_values <- do.call(rbind, p_rows)
    group_rows <- merge(
        group_rows,
        p_values,
        by = c("Gene", "Group"),
        all.x = TRUE,
        sort = FALSE
    )
    group_rows$Significance <- ifelse(
        is.na(group_rows$AdjustedPValue),
        NA_character_,
        ifelse(
            group_rows$AdjustedPValue < 0.001, "***",
            ifelse(
                group_rows$AdjustedPValue < 0.01, "**",
                ifelse(group_rows$AdjustedPValue < 0.05, "*", "NS")
            )
        )
    )
    group_rows$PAdjustMethod <- ifelse(
        is.na(group_rows$AdjustedPValue),
        NA_character_,
        "holm"
    )
    group_rows
}

cq_rqpcr <- read_csv("examples/rqpcr_cq.csv")
design_rqpcr <- read_csv("examples/rqpcr_design.csv")
reference_rqpcr <- rqpcr_reference(
    cq_rqpcr,
    design_rqpcr,
    reference_genes = c("OsUBQ", "OsRBBI2"),
    control_group = "CK"
)
write.csv(
    reference_rqpcr,
    file.path(results_dir, "reference_rqpcr_holm.csv"),
    row.names = FALSE
)

# ---------------------------------------------------------------------------
# 3. Standard curve
# ---------------------------------------------------------------------------
standard_curve_reference <- function(cq_path, info_path, lowest, highest, dilution) {
    cq <- read.delim(cq_path, check.names = FALSE)
    info <- read.delim(info_path, check.names = FALSE)
    merged <- merge(cq, info, by = "Position")
    merged <- merged[merged$Conc >= lowest & merged$Conc <= highest, ]
    merged$log_conc <- log(merged$Conc, base = dilution)
    means <- aggregate(
        Cq ~ Gene + Conc + log_conc,
        data = merged,
        FUN = mean
    )

    rows <- list()
    for (gene in unique(means$Gene)) {
        sub <- means[means$Gene == gene, ]
        fit <- stats::lm(Cq ~ log_conc, data = sub)
        coefs <- stats::coef(fit)
        slope <- unname(coefs[["log_conc"]])
        intercept <- unname(coefs[["(Intercept)"]])
        r2 <- summary(fit)$r.squared
        rows[[length(rows) + 1]] <- data.frame(
            Gene = gene,
            Slope = slope,
            Intercept = intercept,
            RSquared = r2,
            Efficiency = dilution^(-1 / slope) - 1,
            stringsAsFactors = FALSE
        )
    }
    do.call(rbind, rows)
}

reference_sc <- standard_curve_reference(
    "web/calsc.cq.txt",
    "web/calsc.info.txt",
    lowest = 4,
    highest = 4096,
    dilution = 4
)
write.csv(
    reference_sc,
    file.path(results_dir, "reference_standardcurve.csv"),
    row.names = FALSE
)

cat("Wrote independent base-R reference tables to", results_dir, "\n")
