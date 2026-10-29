#!/usr/bin/env Rscript
# Compare qPCRtools C++ output with the independent base-R reference.
#
# Run from code/ after:
#   /opt/homebrew/bin/Rscript validation/reference_base_r.R
#   /Users/lixiang/tmp/qpcr_round2_build/run_cpp round2
#   /opt/homebrew/bin/Rscript validation/compare_to_reference.R
#
# Exit status is non-zero if any compared quantity exceeds its tolerance.

options(stringsAsFactors = FALSE)

results_dir <- "validation/results"
suffix <- "_round2"
tolerance_scale <- 1e-10

read_result <- function(name) {
    read.csv(file.path(results_dir, name), check.names = FALSE)
}

assert_columns <- function(df, columns, label) {
    missing <- setdiff(columns, names(df))
    if (length(missing) > 0) {
        stop(label, " lacks required column(s): ", paste(missing, collapse = ", "))
    }
}

compare_numeric <- function(reference, observed, key, columns, label) {
    assert_columns(reference, c(key, columns), paste(label, "reference"))
    assert_columns(observed, c(key, columns), paste(label, "application"))

    merged <- merge(
        reference[, c(key, columns), drop = FALSE],
        observed[, c(key, columns), drop = FALSE],
        by = key,
        suffixes = c("_ref", "_cpp"),
        all = FALSE,
        sort = FALSE
    )

    if (nrow(merged) == 0) {
        stop(label, ": no matched rows after merging on ", paste(key, collapse = ", "))
    }

    rows <- list()
    for (column in columns) {
        ref <- merged[[paste0(column, "_ref")]]
        cpp <- merged[[paste0(column, "_cpp")]]
        valid <- !is.na(ref) & !is.na(cpp)
        difference <- abs(ref[valid] - cpp[valid])
        scale <- pmax(1, abs(ref[valid]))
        relative <- difference / scale
        worst <- if (length(relative) == 0) 0 else max(relative)
        rows[[column]] <- data.frame(
            comparison = label,
            quantity = column,
            matched_rows = sum(valid),
            max_abs_difference = if (length(difference) == 0) NA_real_ else max(difference),
            max_scaled_difference = worst,
            tolerance = tolerance_scale,
            pass = worst <= tolerance_scale,
            stringsAsFactors = FALSE
        )
    }
    do.call(rbind, rows)
}

compare_with_aliases <- function(reference, observed, key, columns, aliases, label) {
    for (target in names(aliases)) {
        alias <- aliases[[target]]
        if (!alias %in% names(observed)) {
            stop(label, " application lacks column ", alias,
                 " (needed for ", target, ")")
        }
        if (!target %in% names(observed)) {
            observed[[target]] <- observed[[alias]]
        }
    }
    compare_numeric(reference, observed, key, columns, label)
}

ddct_reference <- read_result("reference_ddct_holm.csv")
ddct_cpp <- read_result(paste0("cpp_deltadeltaCt_holm", suffix, ".csv"))
ddct_checks <- compare_with_aliases(
    ddct_reference,
    ddct_cpp,
    key = c("Gene", "Group"),
    columns = c("Mean", "SD", "RawPValue", "AdjustedPValue"),
    aliases = c(SD = "StdDev"),
    label = "Delta-Delta-Ct"
)

ddct_raw_reference <- ddct_reference
ddct_raw_cpp <- read_result(paste0("cpp_deltadeltaCt_raw", suffix, ".csv"))
ddct_raw_checks <- compare_with_aliases(
    ddct_raw_reference,
    ddct_raw_cpp,
    key = c("Gene", "Group"),
    columns = c("RawPValue"),
    aliases = c(RawPValue = "RawPValue"),
    label = "Delta-Delta-Ct (raw P)"
)

rqpcr_reference <- read_result("reference_rqpcr_holm.csv")
rqpcr_cpp <- read_result(paste0("cpp_rqpcr_holm", suffix, ".csv"))
rqpcr_checks <- compare_with_aliases(
    rqpcr_reference,
    rqpcr_cpp,
    key = c("Gene", "Group"),
    columns = c("Mean", "SD", "SE", "RawPValue", "AdjustedPValue"),
    aliases = c(SD = "StdDev", SE = "SE"),
    label = "RqPCR"
)

sc_reference <- read_result("reference_standardcurve.csv")
sc_cpp <- read_result(paste0("cpp_standardcurve", suffix, ".csv"))
sc_checks <- compare_numeric(
    sc_reference,
    sc_cpp,
    key = "Gene",
    columns = c("Slope", "Intercept", "RSquared", "Efficiency"),
    label = "Standard curve"
)

checks <- do.call(rbind, list(
    ddct_checks,
    ddct_raw_checks,
    rqpcr_checks,
    sc_checks
))

print(checks, row.names = FALSE)
write.csv(
    checks,
    file.path(results_dir, "reference_comparison.csv"),
    row.names = FALSE
)

if (!all(checks$pass)) {
    failed <- checks[!checks$pass, c("comparison", "quantity", "max_scaled_difference")]
    print(failed, row.names = FALSE)
    stop("Reference comparison FAILED")
}

cat("\nREFERENCE-COMPARE PASS\n")
