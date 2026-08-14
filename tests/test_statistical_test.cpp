// StatisticalTest 测试：与 base R 期望值对比（golden fixtures）。
// 注意 C++ 已链接 GSL（HAS_GSL），t/F 的 p 值用精确分布，可与 R 精确对齐。
#include <gtest/gtest.h>
#include <QJsonArray>
#include <QJsonObject>

#include "Core/StatisticalTest.h"
#include "fixture_loader.h"

static QVector<double> col(const QJsonObject& o, const char* key) {
    return testfix::to_doubles(o.value(key).toArray());
}

// 独立样本 t 检验：Welch（默认）与 Student（合并方差）均对比统计量与 p 值。
TEST(StatisticalTest, TTestMatchesR) {
    auto cases  = testfix::load_json("expected_stat.json").object().value("ttest").toArray();
    auto inputs = testfix::load_json("data.json").object().value("ttest").toArray();
    ASSERT_EQ(cases.size(), inputs.size());

    for (int i = 0; i < cases.size(); ++i) {
        auto e = cases[i].toObject();
        auto in = inputs[i].toObject();
        QString id = e.value("id").toString();
        auto g1 = col(in, "g1");
        auto g2 = col(in, "g2");

        SCOPED_TRACE(("ttest welch id=" + id).toStdString());
        auto w = e.value("welch").toObject();
        auto rw = qpcr::StatisticalTest::tTest(g1, g2, /*equalVariance=*/false);
        testfix::expect_close(rw.statistic, w.value("statistic").toDouble());
        testfix::expect_close(rw.pValue,   w.value("pvalue").toDouble());

        SCOPED_TRACE(("ttest student id=" + id).toStdString());
        auto s = e.value("student").toObject();
        auto rs = qpcr::StatisticalTest::tTest(g1, g2, /*equalVariance=*/true);
        testfix::expect_close(rs.statistic, s.value("statistic").toDouble());
        testfix::expect_close(rs.pValue,   s.value("pvalue").toDouble());
    }
}

// 配对 t 检验：C++ 差值 = after - before。
TEST(StatisticalTest, PairedTTestMatchesR) {
    auto cases  = testfix::load_json("expected_stat.json").object().value("paired").toArray();
    auto inputs = testfix::load_json("data.json").object().value("paired").toArray();
    ASSERT_EQ(cases.size(), inputs.size());

    for (int i = 0; i < cases.size(); ++i) {
        auto e = cases[i].toObject();
        auto in = inputs[i].toObject();
        SCOPED_TRACE(("paired id=" + e.value("id").toString()).toStdString());
        auto before = col(in, "before");
        auto after  = col(in, "after");
        auto r = qpcr::StatisticalTest::pairedTTest(before, after);
        testfix::expect_close(r.statistic, e.value("statistic").toDouble());
        testfix::expect_close(r.pValue,   e.value("pvalue").toDouble());
    }
}

// Wilcoxon 秩和（Mann-Whitney）：C++ 用平均秩 + 并列方差校正、无连续性校正，
// 故 R 亦用 exact=FALSE, correct=FALSE 对比；统计量按 R 报告 group1 的秩和 W。
TEST(StatisticalTest, WilcoxonMatchesR) {
    auto cases  = testfix::load_json("expected_stat.json").object().value("wilcox").toArray();
    auto inputs = testfix::load_json("data.json").object().value("wilcox").toArray();
    ASSERT_EQ(cases.size(), inputs.size());

    for (int i = 0; i < cases.size(); ++i) {
        auto e = cases[i].toObject();
        auto in = inputs[i].toObject();
        SCOPED_TRACE(("wilcox id=" + e.value("id").toString()).toStdString());
        auto g1 = col(in, "g1");
        auto g2 = col(in, "g2");
        auto r = qpcr::StatisticalTest::wilcoxonTest(g1, g2);
        testfix::expect_close(r.statistic, e.value("statistic").toDouble());
        testfix::expect_close(r.pValue, e.value("pvalue").toDouble());
    }
}

// Wilcoxon 符号秩（配对）：统计量 V 按 R 约定 = (before - after) 正差值的秩和。
TEST(StatisticalTest, WilcoxonSignedRankMatchesR) {
    auto cases  = testfix::load_json("expected_stat.json").object().value("wilcox_signed").toArray();
    auto inputs = testfix::load_json("data.json").object().value("wilcox_signed").toArray();
    ASSERT_EQ(cases.size(), inputs.size());

    for (int i = 0; i < cases.size(); ++i) {
        auto e = cases[i].toObject();
        auto in = inputs[i].toObject();
        SCOPED_TRACE(("wilcox_signed id=" + e.value("id").toString()).toStdString());
        auto before = col(in, "before");
        auto after  = col(in, "after");
        auto r = qpcr::StatisticalTest::wilcoxonSignedRankTest(before, after);
        if (e.value("statistic").isString()) {
            EXPECT_TRUE(std::isnan(r.statistic));
            EXPECT_TRUE(std::isnan(r.pValue));
        } else {
            testfix::expect_close(r.statistic, e.value("statistic").toDouble());
            testfix::expect_close(r.pValue, e.value("pvalue").toDouble());
        }
    }
}

// 单因素 ANOVA：F 统计量与 p 值。
TEST(StatisticalTest, AnovaMatchesR) {
    auto cases  = testfix::load_json("expected_stat.json").object().value("anova").toArray();
    auto inputs = testfix::load_json("data.json").object().value("anova").toArray();
    ASSERT_EQ(cases.size(), inputs.size());

    for (int i = 0; i < cases.size(); ++i) {
        auto e = cases[i].toObject();
        auto in = inputs[i].toObject();
        SCOPED_TRACE(("anova id=" + e.value("id").toString()).toStdString());
        const auto groups_json = in.value("groups").toArray();
        QVector<QVector<double>> groups;
        for (const auto& g : groups_json) groups.append(testfix::to_doubles(g.toArray()));
        auto r = qpcr::StatisticalTest::anova(groups);
        testfix::expect_close(r.statistic, e.value("statistic").toDouble());
        testfix::expect_close(r.pValue,   e.value("pvalue").toDouble());
    }
}
