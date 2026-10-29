# Real-data end-to-end workflow

This directory contains a single reproducible workflow built from the real
qPCR example data distributed with qPCRtools and described in:

Li et al. (2022) *qPCRtools: An R package for qPCR data processing and
visualization*. Frontiers in Genetics 13:1002704.
https://doi.org/10.3389/fgene.2022.1002704

## Files

| File | Purpose |
| --- | --- |
| `01_standard_curve_cq.csv` | Real four-gene, four-fold dilution-series Cq plate derived from `calsc.cq.txt` and `calsc.info.txt`. |
| `01_standard_curve_concentration.csv` | Well-level concentrations for the standard-curve plate. |
| `02_group_cq.csv` | Real treatment-plate Cq values (`cal.expre.rqpcr.cq.txt`) joined to gene names. |
| `02_group_design.csv` | Plate design with two groups, four biological replicates, two technical replicates and measured efficiency per gene. |
| `03_target_gene_curve_parameters.csv` | Published target-gene standard-curve parameters from `cal.expre.curve.sdc.txt`. |
| `provenance.json` | Source paths, byte counts, MD5 and SHA-256. |
| `expected/01_standard_curve_chart.png` | GUI export of the four fitted standard curves. |
| `expected/02_standard_curve_results.csv` | GUI export of slope, intercept, R², P value and efficiency for all four curves. |
| `expected/04_group_summary.csv` | Group means, SD, SE, raw/Holm P values and significance. |
| `expected/05_group_raw.csv` | Biological-replicate-level expression values. |
| `expected/06_analysis_record.txt` | Reproducibility record: inputs, parameters, version and QC warnings. |
| `expected/07_group_expression.png` | GUI export of the group-expression figure. |

## Workflow

1. Open **Standard Curve** and load files `01_*`. Use four-fold dilution,
   concentration range 4–4096, and mean Cq by concentration.
2. Inspect slope, R² and efficiency. The curve plate includes `Gene1` with
   R² = 0.75, which is intentionally retained as a failed-quality example.
3. Inspect `03_target_gene_curve_parameters.csv`; target-gene efficiencies are
   not all equal to the reference gene (`OsUBQ`). Use the efficiency-corrected
   **Standard Curve Expression** path rather than 2^-ΔΔCt.
4. Open **ΔCt / ΔΔCt / statistics**, load `02_group_cq.csv` and
   `02_group_design.csv`, select `OsUBQ`, control group `CK`, Welch test and
   Holm correction.
5. Export the summary table, raw table, both figures and **Analysis record**.

The files preserve the original Cq values and design fields; only the Cq and
design columns are joined to make the GUI inputs self-contained.

## Verification

```bash
python3 code/scripts/19_verify_real_workflow.py
```

The check recomputes the standard curves from well-level data, verifies the
group means/SDs and recorded Welch/Holm P values, confirms the expected
quality-control findings, and checks that all exported tables and PNG files
exist and are readable.
