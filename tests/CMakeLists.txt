# 通过 FetchContent 获取 GoogleTest（仅在 BUILD_TESTS=ON 时触发，联网下载一次）
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
# 防止 gtest 覆盖父项目的编译器/链接器设置
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 测试可执行文件（链接 qpcr_core 复用核心算法，不重复编译）
add_executable(qpcr_tests
    test_smoke.cpp
)
target_link_libraries(qpcr_tests PRIVATE
    qpcr_core
    GTest::gtest_main
)

# 让 ctest 自动发现用例
include(GoogleTest)
gtest_discover_tests(qpcr_tests)
