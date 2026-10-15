# qPCRtools C++ Version

A cross-platform desktop application for qPCR data analysis, rewritten in C++ from the R package [qPCRtools](https://github.com/lixiang117423/qPCRtools).

## Download

**Pre-built binaries are available for download from [GitHub Releases](https://github.com/lixiang117423/qPCRtools_cpp/releases)**

- **Windows**: `qPCRtools-windows.zip` - Extract and run `qPCRtools.exe`
- **macOS**: `qPCRtools-macos.dmg` - Open DMG and drag qPCRtools.app to Applications

No installation required! Just download and run.

## Features

- **Modern Web-based Interface**
  - Built with Qt6 QWebEngineView
  - Responsive design using Bootstrap 5.3
  - Interactive charts powered by ECharts
  - Bilingual support (Chinese/English)
  - Clean, modern UI inspired by ggplot2 aesthetics

- **Multiple Analysis Methods**
  - Standard Curve Calculation
  - ΔCt (Delta Ct) Method
  - ΔΔCt (Delta-Delta Ct) Method

- **Data Import**
  - CSV/TXT file support
  - Excel (.xlsx) file support (planned)
  - Interactive data preview
  - Example data for quick start

- **Statistical Analysis**
  - t-test (independent and paired)
  - Wilcoxon test (rank-sum and signed-rank)
  - ANOVA with Tukey's HSD
  - Outlier detection (IQR method)

- **Visualization**
  - ggplot2-style charts
  - Standard curve plots
  - Box plots
  - Bar plots with error bars
  - Significance labels
  - Multiple color palettes

- **Multi-language Support**
  - 中文 (Chinese)
  - English

## Usage

### Quick Start with Example Data

Try qPCRtools with built-in example data:

1. Download and extract the pre-built application
2. Launch qPCRtools
3. Click "Load Example Data" to load sample qPCR data
4. Select analysis method and click "Run Analysis"

**Or manually load example files** from the [examples/](examples/) directory:
- `examples/cq.csv` - Cq values from qPCR experiment
- `examples/design.csv` - Experimental design mapping

### Using Your Own Data

1. **Import Data**
   - Load Cq data file (CSV format)
   - Load experimental design file (optional)
   - Preview data in the table

2. **Configure Analysis**
   - Select analysis method (Standard Curve, ΔCt, ΔΔCt)
   - Set reference gene and control group
   - Choose statistical test method
   - Configure visualization options

3. **Run Analysis**
   - Click "Run Analysis" button
   - View results in multiple tabs:
     - Data Table: Expression values
     - Charts: ggplot2-style visualizations
     - Statistics: Test results with p-values

4. **Export Results**
   - Export charts as images
   - Export data tables
   - Export statistical results

## Project Structure

```
qPCRtools_cpp/
├── README.md  CHANGELOG.md  CONTRIBUTING.md  LICENSE
├── code/                           # THE SOFTWARE (CMake project root)
│   ├── CMakeLists.txt  vcpkg.json  # Build configuration
│   ├── src/  include/              # C++17 sources and headers: Core/ (algorithms), Data/, GUI/, Utils/
│   ├── web/                        # HTML/CSS/JS interface loaded by QWebEngineView
│   ├── tests/                      # GoogleTest suites + JS smoke test (fixtures generated from R)
│   ├── validation/                 # R-versus-C++ equivalence harness (see its README)
│   ├── examples/                   # Example datasets used by the app and the tests
│   ├── translations/  macos/       # Qt translations; bundled Info.plist (used by CMake)
│   └── scripts/                    # Build, packaging and release scripts
├── docs/                           # Documentation, numbered 01_...; docs/00_README.md maps everything
├── manuscript/                     # Manuscript sources (LaTeX, figures, submission checklist)
├── paper.md  paper.bib             # Short software paper (JOSS submission, must stay at the root)
├── archive/                        # Superseded files kept for reference (do not reuse)
└── 软件著作权申请资料/                # Software-copyright registration package (Chinese)
```

For a one-line description of every top-level item, see
[docs/00_README.md](docs/00_README.md).

## Building from Source

If you prefer to build from source or contribute to development:

### Build Dependencies

- CMake >= 3.20
- C++17 compiler (MSVC 2019+, GCC 10+, Clang 12+)
- Qt6 >= 6.5 (Core, Widgets, WebEngineWidgets, WebChannel)
- Eigen3 >= 3.4
- GSL (GNU Scientific Library) >= 2.7 (optional)

### Platform-Specific Setup

#### macOS
```bash
# Install dependencies via Homebrew
brew install cmake qt@6 eigen gsl
```

#### Windows
- Install Qt6 from the [official installer](https://www.qt.io/download-qt-installer)
- Install Visual Studio 2019 or later
- Install CMake from the [official website](https://cmake.org/download/)

**📖 Detailed Windows build guide:** See [docs/04_windows_build.md](docs/04_windows_build.md) for step-by-step instructions.

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install cmake qt6-base-dev qt6-webengine-dev libeigen3-dev libgsl-dev
```

### Build Instructions

```bash
# Clone repository
git clone https://github.com/lixiang117423/qPCRtools_cpp.git
cd qPCRtools_cpp

# The CMake project lives in code/; the build directory stays at the repository root
cmake -S code -B build

# Build
cmake --build build --parallel

# Run the application (macOS: open build/qPCRtools.app)
./build/qPCRtools        # Linux
build\Release\qPCRtools.exe   # Windows
```

Everything that belongs to the software itself — sources, tests, the
equivalence-validation harness, the example datasets and the build/packaging
scripts — lives under `code/`, which is the project root for CMake and for the
scripts in `code/scripts/`. Run those commands (and `Rscript
code/tests/generate_reference.R`, `Rscript code/validation/run_R.R`) from the
repository root as shown, or from inside `code/` with the paths adjusted.

## Citation

If you use this software in your research, please cite the original R package
it reimplements (algorithms and validation reference) and, if relevant, this
desktop application:

> **Li X, Wang Y, Li J, et al. qPCRtools: An R package for qPCR data processing and visualization.** Frontiers in Genetics, 2022, 13: 1002704.
>
> DOI: [10.3389/fgene.2022.1002704](https://doi.org/10.3389/fgene.2022.1002704)
>
> 📄 Full article: https://www.frontiersin.org/journals/genetics/articles/10.3389/fgene.2022.1002704/full

**BibTeX:**
```bibtex
@article{li2022qpcrtools,
  title={qPCRtools: An R package for qPCR data processing and visualization},
  author={Li, Xiang and Wang, Yingmin and Li, Jingyu and Mei, Xinyue and Liu, Yixiang and Huang, Huichuan},
  journal={Frontiers in Genetics},
  volume={13},
  pages={1002704},
  year={2022},
  publisher={Frontiers Media SA},
  doi={10.3389/fgene.2022.1002704}
}
```

> Author list corrected 2026-10-13 against Crossref; the previous list in this
> file was wrong.

## Contributing, support and governance

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for how to
report a bug, propose a change, or add a new analysis method, and for how
decisions are made and released. In short: reports and pull requests are
handled publicly in
[GitHub issues](https://github.com/lixiang117423/qPCRtools_cpp/issues); the
project is currently maintained by a single author, who reviews every change,
and no contribution requires a signed agreement. For analysis questions
(interpreting a result, choosing a method), please open an issue rather than
emailing, so that the answer stays searchable for other users.

## Manuscript and validation

- `manuscript/` — manuscript describing the software, with the LaTeX sources,
  figures and the submission checklist.
- `validation/` — the R-versus-C++ equivalence harness; `validation/README.md`
  explains how to regenerate the comparison table and figures.

## License

MIT License - see LICENSE file for details

## Authors

- Xiang LI (lixiang117423@gmail.com)

## Acknowledgments

This is a C++ reimplementation of the original R package qPCRtools.
