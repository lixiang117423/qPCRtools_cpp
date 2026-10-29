#!/usr/bin/env python3
"""Verify the real-data qPCR workflow inputs and exported GUI results."""

from __future__ import annotations

import csv
import math
import struct
from collections import defaultdict
from pathlib import Path

CODE = Path(__file__).resolve().parents[1]
DATA = CODE / "examples" / "real_workflow"
EXPECTED = DATA / "expected"


def read_table(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def close(actual: float, expected: float, tolerance: float = 1e-6) -> None:
    if not math.isclose(actual, expected, rel_tol=tolerance, abs_tol=tolerance):
        raise AssertionError(f"expected {expected}, observed {actual}")


def linear_fit(x: list[float], y: list[float]) -> tuple[float, float, float]:
    if len(x) != len(y) or len(x) < 2:
        raise AssertionError("invalid regression input")
    x_mean = sum(x) / len(x)
    y_mean = sum(y) / len(y)
    sxx = sum((value - x_mean) ** 2 for value in x)
    sxy = sum((xv - x_mean) * (yv - y_mean) for xv, yv in zip(x, y))
    slope = sxy / sxx
    intercept = y_mean - slope * x_mean
    fitted = [intercept + slope * value for value in x]
    ss_res = sum((yv - yh) ** 2 for yv, yh in zip(y, fitted))
    ss_total = sum((value - y_mean) ** 2 for value in y)
    return slope, intercept, 1 - ss_res / ss_total


def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"not a PNG: {path}")
    width, height = struct.unpack(">II", data[16:24])
    return width, height


def verify_standard_curve() -> None:
    cq_rows = read_table(DATA / "01_standard_curve_cq.csv")
    concentration_rows = read_table(DATA / "01_standard_curve_concentration.csv")
    if len(cq_rows) != 96 or len(concentration_rows) != 96:
        raise AssertionError("standard-curve inputs must each contain 96 wells")

    concentrations = {
        row["Position"]: (row["Gene"], float(row["Conc"]))
        for row in concentration_rows
    }
    grouped: dict[tuple[str, float], list[float]] = defaultdict(list)
    for row in cq_rows:
        gene, concentration = concentrations[row["Position"]]
        if gene != row["Gene"]:
            raise AssertionError(f"gene mismatch at {row['Position']}")
        grouped[(gene, concentration)].append(float(row["Cq"]))

    expected = {
        row["Gene"]: row
        for row in read_table(EXPECTED / "02_standard_curve_results.csv")
    }
    if set(expected) != {"Gene1", "Gene2", "Gene3", "Gene4"}:
        raise AssertionError("standard-curve result must contain four genes")

    for gene, expected_row in expected.items():
        points = sorted(
            (concentration, sum(values) / len(values))
            for (point_gene, concentration), values in grouped.items()
            if point_gene == gene and 4 <= concentration <= 4096
        )
        x = [math.log(concentration, 4) for concentration, _ in points]
        y = [cq for _, cq in points]
        slope, intercept, r_squared = linear_fit(x, y)
        efficiency = 4 ** (-1 / slope) - 1
        close(slope, float(expected_row["Slope"]))
        close(intercept, float(expected_row["Intercept"]))
        close(r_squared, float(expected_row["R2"]))
        close(efficiency, float(expected_row["Efficiency"]))

    close(float(expected["Gene1"]["R2"]), 0.750897417)
    if float(expected["Gene1"]["R2"]) >= 0.9:
        raise AssertionError("Gene1 must remain the documented failed-quality curve")


def verify_group_analysis() -> None:
    cq_rows = read_table(DATA / "02_group_cq.csv")
    design_rows = read_table(DATA / "02_group_design.csv")
    if len(cq_rows) != 96 or len(design_rows) != 96:
        raise AssertionError("group-analysis inputs must each contain 96 wells")

    summary_rows = read_table(EXPECTED / "04_group_summary.csv")
    raw_rows = read_table(EXPECTED / "05_group_raw.csv")
    if len(summary_rows) != 10:
        raise AssertionError("group summary must contain 5 genes x 2 groups")
    if len(raw_rows) != 40:
        raise AssertionError("raw group table must contain 5 genes x 2 groups x 4 BioRep")

    raw_by_key: dict[tuple[str, str], list[float]] = defaultdict(list)
    for row in raw_rows:
        raw_by_key[(row["Gene"], row["Group"])].append(float(row["Expression"]))

    for row in summary_rows:
        values = raw_by_key[(row["Gene"], row["Group"])]
        if len(values) != 4:
            raise AssertionError(f"expected four biological replicates for {row['Gene']}/{row['Group']}")
        mean = sum(values) / len(values)
        sd = math.sqrt(sum((value - mean) ** 2 for value in values) / (len(values) - 1))
        close(mean, float(row["Mean"]))
        close(sd, float(row["StdDev"]))

    p_values = {
        row["Gene"]: float(row["PValue"])
        for row in summary_rows
        if row["Group"] == "Treatment"
    }
    expected_p = {
        "OSPOX8": 0.2096334029,
        "OsCeBip": 0.7829176402,
        "OsPR10": 0.03682723355,
        "OsRBBI2": 0.1469629511,
        "OsWAK91": 0.002303468722,
    }
    for gene, value in expected_p.items():
        close(p_values[gene], value, tolerance=1e-8)

    patterns = {
        row["Gene"]: row["Significance"]
        for row in summary_rows
        if row["Group"] == "Treatment"
    }
    if patterns != {
        "OSPOX8": "NS",
        "OsCeBip": "NS",
        "OsPR10": "*",
        "OsRBBI2": "NS",
        "OsWAK91": "**",
    }:
        raise AssertionError(f"unexpected significance pattern: {patterns}")

    record = (EXPECTED / "06_analysis_record.txt").read_text(encoding="utf-8")
    required = [
        "Software version        : 1.3.1",
        "Reference gene(s)       : OsUBQ",
        "Control group           : CK",
        "P-value adjustment      : holm",
        "Statistical results     : 5",
        "raw P values are retained",
    ]
    missing = [item for item in required if item not in record]
    if missing:
        raise AssertionError(f"analysis record missing: {missing}")


def verify_exports() -> None:
    required = [
        "01_standard_curve_chart.png",
        "02_standard_curve_results.csv",
        "04_group_summary.csv",
        "05_group_raw.csv",
        "06_analysis_record.txt",
        "07_group_expression.png",
    ]
    for name in required:
        path = EXPECTED / name
        if not path.is_file() or path.stat().st_size == 0:
            raise AssertionError(f"missing or empty export: {path}")
    for name in ("01_standard_curve_chart.png", "07_group_expression.png"):
        width, height = png_size(EXPECTED / name)
        if width < 800 or height < 400:
            raise AssertionError(f"{name} is too small: {width}x{height}")


def main() -> None:
    verify_standard_curve()
    verify_group_analysis()
    verify_exports()
    print("REAL-WORKFLOW PASS")


if __name__ == "__main__":
    main()
