# Example Data for qPCRtools

This directory contains sample qPCR data to help you get started with qPCRtools.

## Files

### cq.csv
Cq (quantification cycle) data from a qPCR experiment.

**Columns:**
- `Position`: Well position on the qPCR plate (e.g., A1, B1, C1)
- `Gene`: Gene name (target gene or reference gene)
- `Cq`: Quantification cycle value

**Experiment:**
- Target gene: `fos-glo-myc`
- Reference gene: `Beta Actin`
- 3 treatment groups: 0, 0.5, 1
- 3 biological replicates per group

### design.csv
Experimental design file mapping well positions to sample groups.

**Columns:**
- `Position`: Well position on the qPCR plate
- `Group`: Treatment group identifier (0, 0.5, 1)
- `BioRep`: Biological replicate number (1, 2, 3)

## How to Use

1. Launch qPCRtools application
2. Click "Load Cq Data" and select `cq.csv`
3. (Optional) Click "Load Design" and select `design.csv`
4. Select analysis method (e.g., ΔCt, ΔΔCt)
5. Configure reference gene: `Beta Actin`
6. Configure control group: `0`
7. Click "Run Analysis"

## Expected Results

With this dataset, you should be able to:
- Calculate standard curve for each gene
- Analyze gene expression using ΔCt or ΔΔCt methods
- Compare expression across treatment groups (0, 0.5, 1)
- Perform statistical tests (t-test, ANOVA)
- Generate ggplot2-style visualizations

## Data Format

Your own data should follow the same format:

**Cq data file (CSV):**
```csv
Position,Gene,Cq
A1,GeneName,20.5
B1,GeneName,21.2
...
```

**Design file (CSV, optional):**
```csv
Position,Group,BioRep
A1,Control,1
B1,Treatment,1
...
```
