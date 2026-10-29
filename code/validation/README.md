# Independent Validation Package

This directory checks the numerical implementation of qPCRtools against an
independent base-R reference. The reference is computed directly from the raw
Cq tables and does not load or call the `qPCRtools` R package.

## Scope

Three quantification methods are covered:

| Method | Base-R reference | C++ implementation | Input |
|---|---|---|---|
| ΔΔCt | `validation/reference_base_r.R` | `ExpressionCalculator::calculateByDeltaDeltaCt` | `examples/cq.csv` + `examples/design.csv` |
| RqPCR | `validation/reference_base_r.R` | `ExpressionCalculator::calculateByStandardCurve` | `examples/rqpcr_cq.csv` + `examples/rqpcr_design.csv` |
| Standard curve | `validation/reference_base_r.R` | `StandardCurve::calculate` | `web/calsc.cq.txt` + `web/calsc.info.txt` |

`CalExpCurve` (slope–intercept) and `CalRTable` (RT volume) are not ported to
C++ and remain outside the validation scope.

## Statistical convention

The bundled example has two treatment groups compared with one control. Welch
and Wilcoxon P values are adjusted by Holm's method within each gene, while
the raw values remain in the application output. The example values on the
linear relative-expression scale are:

| Group | mean | SD | raw Welch P | Holm P |
|---|---|---|---|---|
| 0 | 1.0145 | 0.2098 | — (control) | — |
| 0.5 | 1.9169 | 0.2060 | 0.006027 | 0.012055 |
| 1 | 2.7403 | 0.5580 | 0.022191 | 0.022191 |

## Reproduce

From `code/`:

```bash
# 1. Independent base-R reference
/opt/homebrew/bin/Rscript validation/reference_base_r.R

# 2. C++ output
cmake -S . -B /tmp/qpcr_validation -DQPCRTOOLS_BUILD_VALIDATION=ON
cmake --build /tmp/qpcr_validation --target run_cpp
/tmp/qpcr_validation/run_cpp round2

# 3. Strict numerical comparison
/opt/homebrew/bin/Rscript validation/compare_to_reference.R

# 4. Figures
/Users/lixiang/miniforge3/envs/python/bin/python3 validation/plot_examples.py
/Users/lixiang/miniforge3/envs/python/bin/python3 validation/compare.py
```

`compare_to_reference.R` exits with status 0 and prints
`REFERENCE-COMPARE PASS` only when all compared quantities are within the
tolerance. The current maximum relative differences are:

| Method | Compared quantities | Maximum relative difference |
|---|---|---:|
| ΔΔCt | mean, SD, raw and Holm P | 1.7 × 10^-14 |
| RqPCR | mean, SD, SE, raw and Holm P | 1.9 × 10^-14 |
| Standard curve | slope, intercept, R², efficiency | 4.2 × 10^-15 |

Numerical agreement establishes consistency with the specified algorithms,
not biological correctness.
