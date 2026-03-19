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
├── CMakeLists.txt           # Main CMake configuration
├── README.md                # This file
├── src/                     # Source files
│   ├── Core/                # Core algorithms
│   │   ├── StandardCurve.cpp
│   │   ├── ExpressionCalculator.cpp
│   │   └── StatisticalTest.cpp
│   ├── Data/                # Data structures and parsers
│   │   ├── DataFrame.cpp
│   │   ├── CSVParser.cpp
│   │   └── ExcelImporter.cpp
│   ├── GUI/                 # User interface
│   │   ├── WebMainWindow.cpp
│   │   └── WebBridge.cpp
│   └── Utils/               # Utility functions
├── include/                 # Header files
│   ├── Core/
│   ├── Data/
│   ├── GUI/
│   └── Utils/
├── web/                     # Web interface files
│   ├── index.html           # Main HTML
│   ├── css/
│   │   └── style.css        # Stylesheets
│   └── js/
│       ├── app.js           # Application logic
│       ├── i18n.js          # Internationalization
│       ├── jquery.min.js
│       ├── bootstrap.bundle.min.js
│       └── echarts.min.js   # Chart library
├── examples/                # Example data files
│   ├── cq.csv              # Sample Cq data
│   ├── design.csv          # Sample experimental design
│   └── README.md           # Example data documentation
└── translations/            # i18n translation files
```

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

**📖 Detailed Windows build guide:** See [WINDOWS_BUILD.md](WINDOWS_BUILD.md) for step-by-step instructions.

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

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build . --parallel

# Run the application
./qPCRtools  # macOS/Linux
# or
qPCRtools.exe  # Windows
```

## Citation

If you use this software in your research, please cite:

> **Li X, Wang Y, Li J, et al. qPCRtools: An R package for qPCR data processing and visualization.** Frontiers in Genetics, 2022, 13: 1002704.
>
> DOI: [10.3389/fgene.2022.1002704](https://doi.org/10.3389/fgene.2022.1002704)
>
> 📄 Full article: https://www.frontiersin.org/journals/genetics/articles/10.3389/fgene.2022.1002704/full

**BibTeX:**
```bibtex
@article{li2022qpcrtools,
  title={qPCRtools: An R package for qPCR data processing and visualization},
  author={Li, Xiang and Wang, Yan and Li, Jun and Li, Xueying and Zhang, Yage and Zhang, Yinping and Wang, Ji-Hong},
  journal={Frontiers in Genetics},
  volume={13},
  pages={1002704},
  year={2022},
  publisher={Frontiers Media SA},
  doi={10.3389/fgene.2022.1002704}
}
```

## License

MIT License - see LICENSE file for details

## Authors

- Xiang LI (lixiang117423@gmail.com)

## Acknowledgments

This is a C++ reimplementation of the original R package qPCRtools.
