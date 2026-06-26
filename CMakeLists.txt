cmake_minimum_required(VERSION 3.20)
project(qPCRtools VERSION 1.0.0 LANGUAGES CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt6 配置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    WebEngineCore
    WebEngineWidgets
    WebChannel
    PrintSupport
    LinguistTools
)

# 查找第三方库
find_package(Eigen3 REQUIRED)
find_package(GSL 2.7 QUIET)
find_package(OpenXLSX QUIET)

# =============================================================================
# qpcr_core：核心算法 + 数据处理静态库（app 与 tests 共用，避免重复编译）
# =============================================================================
add_library(qpcr_core STATIC
    # Core
    src/Core/StandardCurve.cpp
    src/Core/ExpressionCalculator.cpp
    src/Core/ExpressionCalculatorInternal.cpp
    src/Core/ExpressionCalculatorDeltaCt.cpp
    src/Core/ExpressionCalculatorDeltaDeltaCt.cpp
    src/Core/ExpressionCalculatorStandardCurve.cpp
    src/Core/StatisticalTest.cpp
    # Data
    src/Data/DataFrame.cpp
    src/Data/CSVParser.cpp
    src/Data/ExcelImporter.cpp
)

target_include_directories(qpcr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Core 依赖 Qt6::Core（容器/字符串）与 Eigen3
target_link_libraries(qpcr_core PUBLIC Qt6::Core Eigen3::Eigen)

# 可选：GSL（StatisticalTest 精确计算）
if(TARGET GSL::gsl)
    target_link_libraries(qpcr_core PUBLIC GSL::gsl GSL::gslcblas)
    target_compile_definitions(qpcr_core PUBLIC HAS_GSL)
endif()

# 可选：OpenXLSX（Excel 导入）
if(TARGET OpenXLSX::OpenXLSX)
    target_link_libraries(qpcr_core PUBLIC OpenXLSX::OpenXLSX)
    target_compile_definitions(qpcr_core PUBLIC HAS_OPENXLSX)
else()
    message(WARNING "OpenXLSX not found, Excel support will be disabled")
endif()

# =============================================================================
# 主程序：Qt6 + WebEngine 图形界面
# =============================================================================
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    src/main_simple.cpp
    src/GUI/WebMainWindow.cpp
    src/GUI/WebBridge.cpp
    include/GUI/WebMainWindow.h
    include/GUI/WebBridge.h
)

# 设置 macOS app bundle 属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.qpcrtools.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macos/Info.plist
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    qpcr_core
    Qt6::Widgets
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::PrintSupport
)

# 国际化支持
set(TS_FILES
    translations/qpcr_tools_zh_CN.ts
    translations/qpcr_tools_en_US.ts
)

# 处理翻译文件（qt_add_translations 会自动调用 qt_add_lupdate）
qt_add_translations(${PROJECT_NAME}
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# 安装配置
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装翻译文件
install(FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/translations
)

# 安装示例数据文件
install(DIRECTORY examples/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/examples
    FILES_MATCHING PATTERN "*.csv"
)

# 打包配置
#include(cpack/CPackConfig.cmake)

# =============================================================================
# 测试（默认关闭；开启时通过 FetchContent 拉取 GoogleTest）
# =============================================================================
option(BUILD_TESTS "Build test suite" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
