// 冒烟测试：验证 GoogleTest + qpcr_core 链接、CMake 接线、ctest 发现机制。
// 全部为可手算验证的数学恒等式，不依赖任何 R 参考。
#include <gtest/gtest.h>

#include "Core/ExpressionCalculator.h"
#include "Core/StandardCurve.h"

// ΔCt = Ct(target) - Ct(reference)
TEST(Smoke, DeltaCt) {
    EXPECT_DOUBLE_EQ(qpcr::ExpressionCalculator::calculateDeltaCt(10.0, 6.0), 4.0);
}

// 相对表达量 = 2^-ΔCt；2^-5 = 0.03125
TEST(Smoke, ExpressionFromDeltaCt) {
    EXPECT_NEAR(qpcr::ExpressionCalculator::calculateExpressionFromDeltaCt(5.0),
                0.03125, 1e-9);
}

// 扩增效率 E = dilution^(-1/slope) - 1；slope=-2, dilution=4 → 4^0.5 - 1 = 1.0 (100%)
TEST(Smoke, EfficiencyAt100Percent) {
    EXPECT_NEAR(qpcr::StandardCurve::calculateEfficiency(-2.0, 4.0), 1.0, 1e-9);
}
