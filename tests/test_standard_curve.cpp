// StandardCurve 回归测试：与 R lm() 的期望值对比（golden fixtures）。
#include <gtest/gtest.h>
#include <QJsonArray>
#include <QJsonObject>

#include "Core/StandardCurve.h"
#include "fixture_loader.h"

// slope / intercept / rSquared / efficiency 应与 R 紧密一致。
// pValue 故意不对比：C++ 用粗略阶跃近似（见 StandardCurve.cpp:251），并非真实 p 值。
TEST(StandardCurve, RegressionMatchesR) {
    auto cases  = testfix::load_json("expected_standard_curve.json").array();
    auto inputs = testfix::load_json("data.json").object().value("standard_curve").toArray();
    ASSERT_EQ(cases.size(), inputs.size());

    for (int i = 0; i < cases.size(); ++i) {
        auto exp = cases[i].toObject();
        auto inp = inputs[i].toObject();
        QString id = exp.value("id").toString();
        SCOPED_TRACE(("standard_curve id=" + id).toStdString());

        auto cq   = testfix::to_doubles(inp.value("cq").toArray());
        auto conc = testfix::to_doubles(inp.value("conc").toArray());
        double dilution = inp.value("dilution").toDouble();

        qpcr::StandardCurveResult r = qpcr::StandardCurve::calculateSingle(cq, conc, id, dilution);

        testfix::expect_close(r.slope,      exp.value("slope").toDouble());
        testfix::expect_close(r.intercept,  exp.value("intercept").toDouble());
        testfix::expect_close(r.rSquared,   exp.value("rSquared").toDouble());
        testfix::expect_close(r.efficiency, exp.value("efficiency").toDouble());
    }
}

// 纯数学恒等式：E = dilution^(-1/slope) - 1。
TEST(StandardCurve, EfficiencyFormula) {
    // slope=-2, dilution=4 → 4^0.5 - 1 = 1.0 (100%)
    EXPECT_NEAR(qpcr::StandardCurve::calculateEfficiency(-2.0, 4.0), 1.0, 1e-9);
    // slope=-1, dilution=10 → 10^1 - 1 = 9.0 (900%)
    EXPECT_NEAR(qpcr::StandardCurve::calculateEfficiency(-1.0, 10.0), 9.0, 1e-9);
}

// 公式字符串格式（两位小数）。
TEST(StandardCurve, FormulaString) {
    EXPECT_EQ(qpcr::StandardCurve::formatFormula(-3.0, 20.0).toStdString(), "y = -3.00*x + 20.00");
}
