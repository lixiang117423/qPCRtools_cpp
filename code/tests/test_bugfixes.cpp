// 回归测试：针对 2026-06 修复的缺陷（占位 p 值、字母标记、原始表对齐、
// 对照组回退、异常值过滤、CSV 转义、byMean 确定性顺序等）。
#include <gtest/gtest.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QTemporaryDir>

#include <cmath>

#include "Core/ExpressionCalculator.h"
#include "Core/StatisticalTest.h"
#include "Core/StandardCurve.h"
#include "Data/CSVParser.h"
#include "Data/DataFrame.h"
#include "fixture_loader.h"

using qpcr::StatisticalTest;
using qpcr::ExpressionCalculator;

//------------------------------------------------------------------------------
// StatisticalTest：退化/边界情形
//------------------------------------------------------------------------------

TEST(Bugfix, TTestDegenerateGroups) {
    // 两组完全相同 → t=0, p=1
    auto r = StatisticalTest::tTest({1, 2, 3}, {1, 2, 3});
    EXPECT_EQ(r.statistic, 0.0);
    EXPECT_DOUBLE_EQ(r.pValue, 1.0);

    // 两组常数但不同 → 差异无穷显著（旧实现除零得 NaN）
    auto r2 = StatisticalTest::tTest({5, 5, 5}, {1, 1, 1});
    EXPECT_TRUE(std::isinf(r2.statistic));
    EXPECT_DOUBLE_EQ(r2.pValue, 0.0);
}

//------------------------------------------------------------------------------
// StatisticalTest：Wilcoxon 小样本精确检验（2026-10-15 修复）
//
// 背景：原实现只有正态近似，3 vs 3 完全分开时给 p ≈ 0.0495 —— 在 α=0.05 下
// 判为显著，而精确检验的最小 p 是 0.1（假阳性）。现对 n1,n2 ≤ 25 走精确分布。
//
// 所有期望值均取自 R 4.6.1（/opt/homebrew/bin/Rscript）：
//   wilcox.test(group1, group2, exact = TRUE)$p.value           ← 精确
//   wilcox.test(group1, group2, exact = FALSE, correct = FALSE)$p.value  ← 正态近似
//------------------------------------------------------------------------------

TEST(Bugfix, WilcoxonExactSeparatedSamples) {
    // 3 个重复、两组完全分开：精确 p = 0.1（不是正态近似的 0.0495）
    auto r = StatisticalTest::wilcoxonTest({1, 2, 3}, {4, 5, 6});
    EXPECT_NEAR(r.pValue, 0.1, 1e-9);
    EXPECT_FALSE(r.isSignificant);   // α = 0.05 下不应判为显著
}

TEST(Bugfix, WilcoxonExactPartiallyOverlapping) {
    // 3 vs 3 部分重叠：R exact = 0.2（正态近似 0.0765）
    auto r = StatisticalTest::wilcoxonTest({1, 2, 3}, {3, 4, 5});
    EXPECT_NEAR(r.pValue, 0.2, 1e-9);
}

TEST(Bugfix, WilcoxonExact4v4) {
    // 4 vs 4 完全分开：R exact = 0.0285714…（= 2/70）
    auto r = StatisticalTest::wilcoxonTest({1, 2, 3, 4}, {5, 6, 7, 8});
    EXPECT_NEAR(r.pValue, 0.028571428571, 1e-9);
    EXPECT_TRUE(r.isSignificant);
}

TEST(Bugfix, WilcoxonExact10v10) {
    // 10 vs 10 完全分开：精确值 = 2 / C(20,10) = 1.082508822446903e-05
    // （R 4.6.1 wilcox.test 默认输出同值；正态近似给 1.57e-04，差一个量级）
    QVector<double> a, b;
    for (int i = 5; i <= 14; ++i) a.append(i);
    for (int i = 20; i <= 29; ++i) b.append(i);
    auto r = StatisticalTest::wilcoxonTest(a, b);
    EXPECT_NEAR(r.pValue, 1.082508822446903e-05, 1e-12);
}

TEST(Bugfix, WilcoxonExactWithTies) {
    // 含并列值：新版 R 也会给精确 p（R 4.6.1 exact = 0.3），不再退回正态近似
    auto r = StatisticalTest::wilcoxonTest({1, 2, 3}, {3, 3, 4});
    EXPECT_NEAR(r.pValue, 0.3, 1e-9);
}

TEST(Bugfix, WilcoxonExactWithTies5v5) {
    // 含并列值的 5 vs 5：R exact = 0.174603（正态近似 0.110492）
    auto r = StatisticalTest::wilcoxonTest({1, 2, 2, 3, 4}, {2, 3, 4, 5, 6});
    EXPECT_NEAR(r.pValue, 0.174603174603, 1e-9);
}

TEST(Bugfix, WilcoxonLargeSampleMatchesNormalApproximation) {
    // 30 vs 30（超过精确检验阈值 N ≤ 50 之外的每侧 25）→ 走正态近似，
    // 与 R wilcox.test(exact = FALSE, correct = FALSE) = 0.002097 一致
    QVector<double> a, b;
    for (int i = 5; i <= 34; ++i) { a.append(i); b.append(i + 8); }
    auto r = StatisticalTest::wilcoxonTest(a, b);
    EXPECT_NEAR(r.pValue, 0.002097, 1e-6);
}

TEST(Bugfix, WilcoxonLargeSampleWithTiesMatchesNormalApproximation) {
    // 30 vs 30 含并列 → 正态近似（R 参考值 0.110165，方差含并列校正）
    QVector<double> a, b;
    const double av[] = {1,1,2,2,3,4,5,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27};
    const double bv[] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
    for (double v : av) a.append(v);
    for (double v : bv) b.append(v);
    auto r = StatisticalTest::wilcoxonTest(a, b);
    EXPECT_NEAR(r.pValue, 0.110165, 1e-6);
}

//------------------------------------------------------------------------------
// StatisticalTest：Tukey HSD 必须用学生化极差分布（旧实现是 t + Bonferroni 近似）
//------------------------------------------------------------------------------

TEST(Bugfix, TukeyUsesStudentizedRangeNotBonferroni) {
    // 3 组 × 4 重复（dfWithin = 9）：R 的 TukeyHSD 给出 A-B p adj = 0.0331532。
    // 旧实现（t 分布 + Bonferroni，3 组 → ×3）在同一数据上给出更大的 p，
    // 两者不是同一个量；这里锁住真 Tukey 的取值。
    QVector<QVector<double>> groups = {{1, 2, 3, 4}, {4, 5, 6, 4.5}, {8, 9, 7, 9.5}};
    QStringList names = {"A", "B", "C"};
    auto results = StatisticalTest::tukeyHSD(groups, names, 0.05);
    ASSERT_EQ(results.size(), 3);

    auto findPair = [&](const QString& g1, const QString& g2) {
        for (const auto& r : results) {
            if (r.group1Name == g1 && r.group2Name == g2) return r;
        }
        return qpcr::TestResult{};
    };

    // 期望值来自 R 4.6.1：ptukey(Q, 3, 9, lower.tail = FALSE)
    auto ab = findPair("A", "B");
    EXPECT_NEAR(ab.statistic, 4.32115884839975, 1e-9);
    EXPECT_NEAR(ab.pValue, 0.0331532285600452, 1e-9);

    auto ac = findPair("A", "C");
    EXPECT_NEAR(ac.statistic, 10.6891824144625, 1e-9);
    EXPECT_NEAR(ac.pValue, 9.21185206983477e-05, 1e-9);
}

TEST(Bugfix, TukeyTinyErrorDegreesOfFreedom) {
    // dfWithin = 2（2 组 × 2 重复）是 R 的 ptukey 精度最差的区域：R 对
    // Q = 12、k = 2、df = 2 给 0.0134441。独立核算表明该值偏小：
    //   · 2×10^7 次蒙特卡洛（Q = |N(0,2)| / sqrt(chi2_2/2)）→ 0.013639 ± 0.000051
    //   · 自适应高精度二重积分（SciPy quad，epsabs 1e-17）→ 0.0136060761678563
    // 本实现与后两者一致（相对偏差 < 1e-11），故此处以高精度积分值为期望。
    // 数据构造：means 2 与 26、组内方差各 8 → MSE = 8、seQ = 2 → Q = 12。
    QVector<QVector<double>> groups = {{0, 4}, {24, 28}};
    QStringList names = {"A", "B"};
    auto results = StatisticalTest::tukeyHSD(groups, names, 0.05);
    ASSERT_EQ(results.size(), 1);

    EXPECT_NEAR(results[0].statistic, 12.0, 1e-12);
    EXPECT_NEAR(results[0].pValue, 0.0136060761678563, 1e-9);
}

TEST(Bugfix, TukeyDegenerateInputs) {
    // 只有一组 → 无两两比较
    EXPECT_TRUE(StatisticalTest::tukeyHSD({{1, 2, 3}}, {"A"}).isEmpty());
    // 每组只有一个观测（dfWithin = 0）→ 无法给出 p 值，返回空
    EXPECT_TRUE(StatisticalTest::tukeyHSD({{1}, {2}}, {"A", "B"}).isEmpty());
    // 全部相同的两组 → Q = 0，p = 1
    auto same = StatisticalTest::tukeyHSD({{1, 1, 1}, {1, 1, 1}}, {"A", "B"});
    ASSERT_EQ(same.size(), 1);
    EXPECT_DOUBLE_EQ(same[0].pValue, 1.0);
}

TEST(Bugfix, AnovaDegenerateGroups) {
    // 各组完全相同 → F=0, p=1
    auto r = StatisticalTest::anova({{1, 2, 3}, {1, 2, 3}, {1, 2, 3}});
    EXPECT_DOUBLE_EQ(r.pValue, 1.0);

    // 每组只有一个观测：无法估计组内方差 → NaN（旧实现除零/伪值）
    auto r2 = StatisticalTest::anova({{1}, {2}, {3}});
    EXPECT_TRUE(std::isnan(r2.pValue));

    // 含空组时按非空组计算（旧实现把空组计入 k，自由度失真）
    auto r3 = StatisticalTest::anova({{1, 2, 3}, {}, {4, 5, 6}});
    EXPECT_TRUE(std::isfinite(r3.pValue));
    EXPECT_GT(r3.statistic, 0.0);
}

TEST(Bugfix, ConfidenceIntervalMatchesGSL) {
    // GSL 可用时两者应一致；无 GSL 时内部二分法也应给出近似值。
    auto ci = StatisticalTest::confidenceInterval(0.0, 1.0, 10, 0.95);
    EXPECT_NEAR(ci.second, 2.22813885196494, 1e-4);  // t_{0.975, 10}
}

//------------------------------------------------------------------------------
// ExpressionCalculator：统计结果必须真实（不再有占位 p=0.05）
//------------------------------------------------------------------------------

TEST(Bugfix, ExpressionPValuesAreReal) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");
    ASSERT_GT(cq.rowCount(), 0);

    qpcr::DeltaDeltaCtParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = "Beta Actin";
    params.controlGroup  = "0";

    auto result = ExpressionCalculator::calculateByDeltaDeltaCt(params, "t.test");
    ASSERT_GT(result.statistics.size(), 0);
    for (const auto& stat : result.statistics) {
        EXPECT_TRUE(std::isfinite(stat.pValue)) << "gene=" << stat.gene.toStdString();
        EXPECT_GE(stat.pValue, 0.0);
        EXPECT_LE(stat.pValue, 1.0);
        EXPECT_TRUE(stat.significance == "***" || stat.significance == "**" ||
                    stat.significance == "*"  || stat.significance == "NS");
    }
}

TEST(Bugfix, PerformTTestDelegatesToWelch) {
    const QVector<double> g1{1.0, 2.0, 3.0, 4.0};
    const QVector<double> g2{2.5, 3.5, 4.5, 5.5};
    auto viaExpression = ExpressionCalculator::performTTest(g1, g2, "G", "A", "B");
    auto direct = StatisticalTest::tTest(g1, g2, false);
    testfix::expect_close(viaExpression.pValue, direct.pValue);
    testfix::expect_close(viaExpression.tStatistic, direct.statistic);
    EXPECT_NE(viaExpression.pValue, 0.05);  // 不再是占位值
}

TEST(Bugfix, PerformWilcoxonNotPlaceholder) {
    const QVector<double> g1{1.0, 2.0, 3.0};
    const QVector<double> g2{4.0, 5.0, 6.0};
    auto viaExpression = ExpressionCalculator::performWilcoxonTest(g1, g2, "G", "A", "B");
    auto direct = StatisticalTest::wilcoxonTest(g1, g2);
    testfix::expect_close(viaExpression.pValue, direct.pValue);
    EXPECT_NE(viaExpression.pValue, 0.05);
}

TEST(Bugfix, PerformAnovaRealPValueAndLetters) {
    // 3 组 × 3 重复，组间差异明显 → p < 0.05 且字母标记非空
    qpcr::DataFrame df;
    QVector<QVariant> genes, groups, vals;
    for (const char* g : {"G1", "G1", "G1", "G2", "G2", "G2"}) {
        for (int i = 0; i < 9; ++i) {
            genes.append(QString(g));
        }
    }
    for (int rep = 0; rep < 6; ++rep) {
        for (int i = 0; i < 3; ++i) groups.append("A");
        for (int i = 0; i < 3; ++i) groups.append("B");
        for (int i = 0; i < 3; ++i) groups.append("C");
    }
    const double base[3][3] = {{1, 1.1, 0.9}, {2, 2.1, 1.9}, {5, 5.2, 4.8}};
    for (int rep = 0; rep < 6; ++rep) {
        for (int grp = 0; grp < 3; ++grp) {
            for (int i = 0; i < 3; ++i) vals.append(base[grp][i]);
        }
    }
    df.addColumn("Gene", genes);
    df.addColumn("Group", groups);
    df.addColumn("Expression", vals);

    auto results = ExpressionCalculator::performANOVA(df, "Gene", "Group", "Expression");
    ASSERT_GT(results.size(), 0);
    for (const auto& stat : results) {
        EXPECT_TRUE(std::isfinite(stat.pValue));
        EXPECT_LT(stat.pValue, 0.05);
        EXPECT_FALSE(stat.letterGroup.isEmpty());
        EXPECT_FALSE(stat.significance.isEmpty());
    }
}

//------------------------------------------------------------------------------
// generateLetterGroups：字母标记算法
//------------------------------------------------------------------------------

namespace {

qpcr::TestResult makeTukey(const QString& g1, const QString& g2, double p) {
    qpcr::TestResult r;
    r.testName = QStringLiteral("Tukey HSD: %1 vs %2").arg(g1, g2);
    r.group1Name = g1;
    r.group2Name = g2;
    r.pValue = p;
    return r;
}

QHash<QString, QString> lettersFor(const QHash<QString, double>& means,
                                   const QVector<qpcr::TestResult>& tukey) {
    return ExpressionCalculator::generateLetterGroups(means, tukey, 0.05);
}

} // namespace

TEST(Bugfix, LetterGroupsAllNonSignificant) {
    QHash<QString, double> means{{"A", 3.0}, {"B", 2.0}, {"C", 1.0}};
    auto lt = lettersFor(means, {makeTukey("A", "B", 0.5), makeTukey("A", "C", 0.5), makeTukey("B", "C", 0.5)});
    EXPECT_EQ(lt["A"].toStdString(), "a");
    EXPECT_EQ(lt["B"].toStdString(), "a");
    EXPECT_EQ(lt["C"].toStdString(), "a");
}

TEST(Bugfix, LetterGroupsOnlyABShared) {
    QHash<QString, double> means{{"A", 3.0}, {"B", 2.0}, {"C", 1.0}};
    auto lt = lettersFor(means, {makeTukey("A", "B", 0.5), makeTukey("A", "C", 0.01), makeTukey("B", "C", 0.01)});
    EXPECT_EQ(lt["A"].toStdString(), "a");
    EXPECT_EQ(lt["B"].toStdString(), "a");
    EXPECT_EQ(lt["C"].toStdString(), "b");
}

TEST(Bugfix, LetterGroupsNonTransitiveChain) {
    QHash<QString, double> means{{"A", 3.0}, {"B", 2.0}, {"C", 1.0}};
    auto lt = lettersFor(means, {makeTukey("A", "B", 0.5), makeTukey("A", "C", 0.01), makeTukey("B", "C", 0.5)});
    EXPECT_EQ(lt["A"].toStdString(), "a");
    EXPECT_EQ(lt["B"].toStdString(), "ab");
    EXPECT_EQ(lt["C"].toStdString(), "b");
}

TEST(Bugfix, LetterGroupsBridgeWithDistinctGroup) {
    // 旧算法会让 D 错误地与 C 共享字母（显著差异组共享字母）
    QHash<QString, double> means{{"A", 4.0}, {"B", 3.0}, {"C", 2.0}, {"D", 1.0}};
    auto lt = lettersFor(means, {makeTukey("A", "B", 0.5), makeTukey("B", "C", 0.5),
                                 makeTukey("A", "C", 0.01), makeTukey("A", "D", 0.01),
                                 makeTukey("B", "D", 0.01), makeTukey("C", "D", 0.01)});
    EXPECT_EQ(lt["A"].toStdString(), "a");
    EXPECT_EQ(lt["B"].toStdString(), "ab");
    EXPECT_EQ(lt["C"].toStdString(), "b");
    EXPECT_EQ(lt["D"].toStdString(), "c");
}

TEST(Bugfix, LetterGroupsAllSignificant) {
    QHash<QString, double> means{{"A", 4.0}, {"B", 3.0}, {"C", 2.0}, {"D", 1.0}};
    auto lt = lettersFor(means, {makeTukey("A", "B", 0.001), makeTukey("A", "C", 0.001),
                                 makeTukey("A", "D", 0.001), makeTukey("B", "C", 0.001),
                                 makeTukey("B", "D", 0.001), makeTukey("C", "D", 0.001)});
    EXPECT_EQ(lt["A"].toStdString(), "a");
    EXPECT_EQ(lt["B"].toStdString(), "b");
    EXPECT_EQ(lt["C"].toStdString(), "c");
    EXPECT_EQ(lt["D"].toStdString(), "d");
}

//------------------------------------------------------------------------------
// 异常值过滤：R type-7 分位数
//------------------------------------------------------------------------------

TEST(Bugfix, RemoveOutliersUsesType7Quartiles) {
    // R: Q1=2, Q3=4 → 100 被移除；旧实现 (n/4 下标) 会得到 Q3=4 相同，但 10 应保留
    auto kept = ExpressionCalculator::removeOutliers({1.0, 2.0, 3.0, 4.0, 100.0});
    ASSERT_EQ(kept.size(), 4);
    EXPECT_EQ(kept, QVector<double>({1.0, 2.0, 3.0, 4.0}));

    // Q1=2, Q3=4, IQR=2 → 上界 7：10 为异常值，5 保留
    auto kept2 = ExpressionCalculator::removeOutliers({1.0, 2.0, 3.0, 4.0, 5.0});
    EXPECT_EQ(kept2.size(), 5);

    // 少于 4 个值不过滤
    EXPECT_EQ(ExpressionCalculator::removeOutliers({1.0, 2.0, 100.0}).size(), 3);
}

//------------------------------------------------------------------------------
// ΔCt / ΔΔCt 行为修复
//------------------------------------------------------------------------------

TEST(Bugfix, DeltaCtRawDataMeanAligned) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");

    qpcr::DeltaCtParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = "Beta Actin";
    params.controlGroup  = "0";

    auto result = ExpressionCalculator::calculateByDeltaCt(params, "t.test");
    ASSERT_GT(result.rawData.rowCount(), 0);

    auto rawGenes = result.rawData.getStringColumn("Gene");
    auto rawGroups = result.rawData.getStringColumn("Group");
    auto rawMeans = result.rawData.getNumericColumn("Mean");
    auto rawSDs = result.rawData.getNumericColumn("SD");

    // 每个 raw 行都有对应的 Mean/SD（旧实现的错位补丁会导致列长度不一致）
    ASSERT_EQ(rawMeans.size(), result.rawData.rowCount());
    ASSERT_EQ(rawSDs.size(), result.rawData.rowCount());

    // raw 行的 Mean 应等于该 (gene, group) 组合的汇总均值
    for (int i = 0; i < result.rawData.rowCount(); ++i) {
        double summaryMean = qQNaN();
        for (int j = 0; j < result.table.rowCount(); ++j) {
            if (result.table.get(j, "Gene").toString() == rawGenes[i] &&
                result.table.get(j, "Group").toString() == rawGroups[i]) {
                summaryMean = result.table.get(j, "Mean").toDouble();
                break;
            }
        }
        EXPECT_TRUE(std::isfinite(summaryMean));
        EXPECT_NEAR(rawMeans[i], summaryMean, 1e-9);
    }
}

TEST(Bugfix, DeltaDeltaCtControlGroupFallback) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");

    qpcr::DeltaDeltaCtParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = "Beta Actin";
    params.controlGroup  = "";  // 旧实现：空对照组 → 统计结果静默为空

    auto result = ExpressionCalculator::calculateByDeltaDeltaCt(params, "t.test");
    EXPECT_GT(result.statistics.size(), 0);
}

TEST(Bugfix, DeltaDeltaCtRemoveOutliersDropsBiorep) {
    // 手工构造：CK 组 G1 的 4 个 bioRep 中含一个明显异常值
    qpcr::DataFrame cq, design;
    QVector<QVariant> pos, gene, cqVal;
    QVector<QVariant> dPos, dGroup, dBio;

    const double g1Ck[]   = {20.0, 20.1, 20.2, 30.0};  // 30 为异常
    const double g1Treat[] = {20.5, 20.6, 20.7, 20.8};
    for (int i = 0; i < 4; ++i) {
        pos.append(QStringLiteral("C%1").arg(i + 1));  gene.append("G1");
        cqVal.append(g1Ck[i]);
        dPos.append(pos.last()); dGroup.append("CK"); dBio.append(QString::number(i + 1));
        pos.append(QStringLiteral("T%1").arg(i + 1));  gene.append("G1");
        cqVal.append(g1Treat[i]);
        dPos.append(pos.last()); dGroup.append("T"); dBio.append(QString::number(i + 1));
    }
    for (int i = 0; i < 4; ++i) {
        pos.append(QStringLiteral("RC%1").arg(i + 1)); gene.append("R");
        cqVal.append(18.0);
        dPos.append(pos.last()); dGroup.append("CK"); dBio.append(QString::number(i + 1));
        pos.append(QStringLiteral("RT%1").arg(i + 1)); gene.append("R");
        cqVal.append(18.0);
        dPos.append(pos.last()); dGroup.append("T"); dBio.append(QString::number(i + 1));
    }

    cq.addColumn("Position", pos);
    cq.addColumn("Gene", gene);
    cq.addColumn("Cq", cqVal);
    design.addColumn("Position", dPos);
    design.addColumn("Group", dGroup);
    design.addColumn("BioRep", dBio);

    qpcr::DeltaDeltaCtParams p;
    p.cqTable = cq; p.designTable = design;
    p.referenceGene = "R"; p.controlGroup = "CK";
    p.removeOutliers = false;
    auto noFilter = ExpressionCalculator::calculateByDeltaDeltaCt(p, "t.test");
    int rowsNoFilter = noFilter.rawData.rowCount();

    p.removeOutliers = true;
    auto filtered = ExpressionCalculator::calculateByDeltaDeltaCt(p, "t.test");
    int rowsFiltered = filtered.rawData.rowCount();

    EXPECT_EQ(rowsNoFilter, 8);        // 2 组 × 4 bioRep
    EXPECT_EQ(rowsFiltered, 7);        // CK 组异常 bioRep 被移除
}

TEST(Bugfix, DeltaDeltaCtAnovaProducesLetters) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");

    qpcr::DeltaDeltaCtParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = "Beta Actin";
    params.controlGroup  = "0";

    auto result = ExpressionCalculator::calculateByDeltaDeltaCt(params, "anova");
    ASSERT_GT(result.statistics.size(), 0);
    for (const auto& stat : result.statistics) {
        EXPECT_TRUE(std::isfinite(stat.pValue));
        EXPECT_FALSE(stat.letterGroup.isEmpty());
    }
}

//------------------------------------------------------------------------------
// StandardCurve：byMean 顺序确定性
//------------------------------------------------------------------------------

TEST(Bugfix, StandardCurveByMeanDeterministicOrder) {
    qpcr::DataFrame cq, conc;
    cq.addColumn("Position", {QVariant("A1"), QVariant("A2"), QVariant("A3"),
                              QVariant("B1"), QVariant("B2"), QVariant("B3")});
    cq.addColumn("Gene", {QVariant("G1"), QVariant("G1"), QVariant("G1"),
                          QVariant("G1"), QVariant("G1"), QVariant("G1")});
    cq.addColumn("Cq", {QVariant(20.0), QVariant(20.2), QVariant(20.1),
                        QVariant(25.0), QVariant(25.1), QVariant(24.9)});
    conc.addColumn("Position", {QVariant("A1"), QVariant("A2"), QVariant("A3"),
                                QVariant("B1"), QVariant("B2"), QVariant("B3")});
    conc.addColumn("Gene", {QVariant("G1"), QVariant("G1"), QVariant("G1"),
                            QVariant("G1"), QVariant("G1"), QVariant("G1")});
    conc.addColumn("Conc", {QVariant(100.0), QVariant(100.0), QVariant(100.0),
                            QVariant(10.0), QVariant(10.0), QVariant(10.0)});

    auto results = qpcr::StandardCurve::calculate(cq, conc, 1.0, 1000.0, 10.0, /*byMean=*/true);
    ASSERT_EQ(results.size(), 1);
    // 浓度升序：log10(10)=1 在 log10(100)=2 之前（旧实现 QHash 键序不稳定）
    ASSERT_EQ(results[0].logConcentrations.size(), 2);
    EXPECT_LT(results[0].logConcentrations[0], results[0].logConcentrations[1]);
    EXPECT_NEAR(results[0].logConcentrations[0], 1.0, 1e-12);
    EXPECT_NEAR(results[0].logConcentrations[1], 2.0, 1e-12);
}

TEST(Bugfix, StandardCurveGeneOrderDeterministic) {
    // 基因在输入里故意乱序（G3 → G1 → G2）：结果行序必须与输入顺序、也与
    // QSet 的随机迭代序无关（旧实现直接遍历 QSet，同一二进制两次运行的行序不同，
    // 2026-10-15 实测 validation/results/cpp_standardcurve.csv 两次运行不一致）。
    qpcr::DataFrame cq, conc;
    const QStringList genes = {"G3", "G3", "G1", "G1", "G2", "G2"};
    const QList<double> cqs   = {22.0, 25.0, 20.0, 23.0, 21.0, 24.0};
    const QList<double> concs = {100.0, 10.0, 100.0, 10.0, 100.0, 10.0};
    QList<QVariant> cqPositions, concPositions, cqGenes, concGenes, cqValues, concValues;
    for (int i = 0; i < genes.size(); ++i) {
        const QString pos = QStringLiteral("%1").arg(i + 1);
        cqPositions.append(pos);
        concPositions.append(pos);
        cqGenes.append(genes[i]);
        concGenes.append(genes[i]);
        cqValues.append(cqs[i]);
        concValues.append(concs[i]);
    }
    cq.addColumn("Position", cqPositions);
    cq.addColumn("Gene", cqGenes);
    cq.addColumn("Cq", cqValues);
    conc.addColumn("Position", concPositions);
    conc.addColumn("Gene", concGenes);
    conc.addColumn("Conc", concValues);

    auto results = qpcr::StandardCurve::calculate(cq, conc, 1.0, 1000.0, 10.0, /*byMean=*/false);
    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].gene.toStdString(), "G1");
    EXPECT_EQ(results[1].gene.toStdString(), "G2");
    EXPECT_EQ(results[2].gene.toStdString(), "G3");
}

//------------------------------------------------------------------------------
// DataFrame：CSV 转义往返
//------------------------------------------------------------------------------

TEST(Bugfix, DataFrameSaveCsvQuotesFields) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    qpcr::DataFrame df;
    df.addColumn("name", {QVariant(QStringLiteral("a,b")),
                          QVariant(QStringLiteral("say \"hi\""))});
    df.addColumn("value", {QVariant(1.5), QVariant(2.5)});
    const QString path = dir.path() + "/out.csv";
    ASSERT_TRUE(df.saveCSV(path));

    qpcr::CSVParser parser;
    auto back = parser.parse(path);
    ASSERT_EQ(back.rowCount(), 2);
    EXPECT_EQ(back.get(0, "name").toString().toStdString(), "a,b");
    EXPECT_EQ(back.get(1, "name").toString().toStdString(), "say \"hi\"");
    EXPECT_DOUBLE_EQ(back.get(0, "value").toDouble(), 1.5);
}
