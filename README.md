# qPCRtools C++ Version

A cross-platform desktop application for qPCR data analysis, rewritten in C++ from the R package [qPCRtools](https://github.com/lixiang117423/qPCRtools).

## Features

- **Multiple Analysis Methods**
  - Standard Curve Calculation
  - ΔCt (Delta Ct) Method
  - ΔΔCt (Delta-Delta Ct) Method
  - RqPCR Method
  - Reverse Transcription Volume Calculator

- **Data Import**
  - CSV/TXT file support
  - Excel (.xlsx) file support
  - Interactive data preview

- **Statistical Analysis**
  - t-test
  - Wilcoxon test
  - ANOVA with Tukey's HSD
  - Outlier detection (IQR method)

- **Visualization**
  - ggplot2-style charts
  - Standard curve plots
  - Box plots
  - Bar plots with error bars
  - Significance labels

- **Multi-language Support**
  - English
  - 中文 (Chinese)

## Requirements

### Build Dependencies

- CMake >= 3.20
- C++17 compiler (MSVC 2019+, GCC 10+, Clang 12+)
- Qt6 >= 6.5
- Eigen3 >= 3.4
- GSL (GNU Scientific Library) >= 2.7
- OpenXLSX (optional, for Excel support)

### Platform-Specific

#### Windows
- Visual Studio 2019 or later
- Qt6 (install from Qt installer)

#### macOS
- Xcode 14 or later
- Qt6 (install via Homebrew: `brew install qt@6`)

#### Linux
- GCC 10+ or Clang 12+
- Qt6 development packages

## Building

```bash
# Clone repository
git clone https://github.com/lixiang117423/qPCRtools-cpp.git
cd qPCRtools-cpp

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Install (optional)
cmake --install .
```

## Usage

1. Launch qPCRtools application
2. Select analysis method
3. Import Cq data file (CSV/Excel)
4. Configure parameters (reference gene, control group, etc.)
5. Run analysis
6. View results and export

## Project Structure

```
qPCRtools_cpp/
├── CMakeLists.txt           # Main CMake configuration
├── README.md                # This file
├── src/                     # Source files
│   ├── Core/                # Core algorithms
│   ├── Data/                # Data structures and parsers
│   ├── GUI/                 # User interface
│   └── Utils/               # Utility functions
├── include/                 # Header files
├── tests/                   # Test suite
├── translations/            # i18n translation files
├── resources/               # Resources (icons, examples)
└── packaging/               # Packaging scripts
```

## Citation

If you use this software in your research, please cite:

> Li X, Wang Y, Li J, et al. qPCRtools: An R package for qPCR data processing and visualization[J]. Frontiers in Genetics, 2022, 13: 1002704.

## License

MIT License - see LICENSE file for details

## Authors

- Xiang LI (lixiang117423@gmail.com)

## Acknowledgments

This is a C++ reimplementation of the original R package qPCRtools.
