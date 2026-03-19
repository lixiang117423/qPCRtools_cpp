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

# 源文件
set(SOURCES
    src/main_simple.cpp

    # Core
    src/Core/StandardCurve.cpp
    src/Core/ExpressionCalculator.cpp
    src/Core/StatisticalTest.cpp

    # Data
    src/Data/DataFrame.cpp
    src/Data/CSVParser.cpp
    src/Data/ExcelImporter.cpp

    # GUI
    src/GUI/WebMainWindow.cpp
    src/GUI/WebBridge.cpp

    # Utils - 暂时注释，Web界面使用JavaScript实现ggplot2主题
    # src/Utils/GgplotTheme.cpp
)

# 头文件
set(HEADERS
    # Core
    include/Core/StandardCurve.h
    include/Core/ExpressionCalculator.h
    include/Core/StatisticalTest.h

    # Data
    include/Data/DataFrame.h
    include/Data/CSVParser.h
    include/Data/ExcelImporter.h

    # GUI
    include/GUI/WebMainWindow.h
    include/GUI/WebBridge.h

    # Utils - 暂时注释
    # include/Utils/GgplotTheme.h
)

# UI 文件 (暂时不用，我们用纯代码创建 UI)
# set(UI_FILES
#     resources/ui/mainwindow.ui
#     resources/ui/importpage.ui
#     resources/ui/configpage.ui
#     resources/ui/resultspage.ui
# )

# 创建可执行文件
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    # ${UI_FILES}
)

# 设置 macOS app bundle 属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.qpcrtools.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/macos/Info.plist
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineCore
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::PrintSupport
    Eigen3::Eigen
)

# 如果找到 GSL
if(TARGET GSL::gsl)
    target_link_libraries(${PROJECT_NAME} PRIVATE GSL::gsl GSL::gslcblas)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_GSL)
endif()

# 如果找到 OpenXLSX
if(TARGET OpenXLSX::OpenXLSX)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenXLSX::OpenXLSX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_OPENXLSX)
else()
    message(WARNING "OpenXLSX not found, Excel support will be disabled")
endif()

# 包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
#
## 开发者选项
#option(BUILD_TESTS "Build test suite" OFF)
#if(BUILD_TESTS)
#    enable_testing()
#    add_subdirectory(tests)
#endif()
