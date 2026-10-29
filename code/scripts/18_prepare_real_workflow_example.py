#!/usr/bin/env python3
"""从已发表 qPCRtools 示例数据构建单一、可追溯的完整工作流。"""

from __future__ import annotations

import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT.parent / "qPCRtools" / "CRAN" / "qPCRtools" / "inst" / "examples"
OUT = ROOT / "code" / "examples" / "real_workflow"


def read_table(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        sample = handle.read(2048)
        handle.seek(0)
        delimiter = "\t" if "\t" in sample.splitlines()[0] else ","
        return list(csv.DictReader(handle, delimiter=delimiter))


def write_table(path: Path, fieldnames: list[str], rows: list[dict[str, str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def digest(path: Path) -> dict[str, str]:
    data = path.read_bytes()
    return {
        "bytes": str(len(data)),
        "md5": hashlib.md5(data).hexdigest(),
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)

    curve_cq = read_table(SOURCE / "calsc.cq.txt")
    curve_info = read_table(SOURCE / "calsc.info.txt")
    info_by_position = {row["Position"]: row for row in curve_info}
    curve_cq_out = [
        {
            "Position": row["Position"],
            "Gene": info_by_position[row["Position"]]["Gene"],
            "Cq": row["Cq"],
        }
        for row in curve_cq
    ]
    write_table(OUT / "01_standard_curve_cq.csv", ["Position", "Gene", "Cq"], curve_cq_out)
    write_table(
        OUT / "01_standard_curve_concentration.csv",
        ["Position", "Gene", "Conc"],
        curve_info,
    )

    group_design = read_table(SOURCE / "cal.expre.rqpcr.design.txt")
    group_cq = read_table(SOURCE / "cal.expre.rqpcr.cq.txt")
    gene_by_position = {row["Position"]: row["Gene"] for row in group_design}
    group_cq_out = [
        {
            "Position": row["Position"],
            "Gene": gene_by_position[row["Position"]],
            "Cq": row["Cq"],
        }
        for row in group_cq
    ]
    write_table(OUT / "02_group_cq.csv", ["Position", "Gene", "Cq"], group_cq_out)
    write_table(
        OUT / "02_group_design.csv",
        ["Position", "Group", "Gene", "BioRep", "TechRep", "Eff"],
        group_design,
    )

    curve_params = read_table(SOURCE / "cal.expre.curve.sdc.txt")
    genes = sorted({row["Gene"] for row in group_design})
    selected = [row for row in curve_params if row["Gene"] in genes]
    write_table(
        OUT / "03_target_gene_curve_parameters.csv",
        ["Gene", "Formula", "Slope", "Intercept", "R2", "P.value", "max.Cq", "min.Cq", "Date"],
        selected,
    )

    source_files = [
        SOURCE / "calsc.cq.txt",
        SOURCE / "calsc.info.txt",
        SOURCE / "cal.expre.rqpcr.cq.txt",
        SOURCE / "cal.expre.rqpcr.design.txt",
        SOURCE / "cal.expre.curve.sdc.txt",
    ]
    metadata = {
        "source": "qPCRtools 2022 Frontiers in Genetics example data",
        "doi": "10.3389/fgene.2022.1002704",
        "source_files": {
            str(path): digest(path)
            for path in source_files
        },
        "workflow": [
            "fit the real dilution-series plate with CalCurve",
            "screen target-gene standard curves and select efficiency correction",
            "run the real treatment plate with Standard Curve Expression",
            "export the summary table and analysis record",
        ],
    }
    (OUT / "provenance.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    readme = """# Real-data end-to-end workflow

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
5. Export the summary table, raw table, figure and **Analysis record**.

The files preserve the original Cq values and design fields; only the Cq and
design columns are joined to make the GUI inputs self-contained.
"""
    (OUT / "README.md").write_text(readme, encoding="utf-8")

    for path in sorted(OUT.iterdir()):
        print(f"written {path} ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
