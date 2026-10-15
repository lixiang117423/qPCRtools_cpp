// ExpressionCalculator 集成测试：与 R qPCRtools 的表达量结果对比（golden fixtures）。
#include <gtest/gtest.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

#include <algorithm>
#include <limits>

#include "Core/ExpressionCalculator.h"
#include "Data/CSVParser.h"
#include "fixture_loader.h"

// ΔCt 方法：每个 gene 各 group 的平均表达量应与 R qPCRtools::CalExp2dCt 的 mean.expre 一致。
// 用升序集合对比，规避 group 在 C++/R 间字符串/数值表示的差异。
TEST(ExpressionCalculator, DeltaCtMatchesR) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");
    ASSERT_GT(cq.rowCount(), 0);
    ASSERT_GT(design.rowCount(), 0);

    auto fix    = testfix::load_json("expected_expression_dct.json").object();
    QString ref = fix.value("ref_gene").toString();

    qpcr::DeltaCtParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = ref;
    params.controlGroup  = "0";

    qpcr::ExpressionResult result =
        qpcr::ExpressionCalculator::calculateByDeltaCt(params, "t.test");

    auto byGene = fix.value("by_gene").toObject();
    for (const QString& gene : byGene.keys()) {
        SCOPED_TRACE(("expression_dct gene=" + gene).toStdString());
        QVector<double> expected = testfix::to_doubles(byGene.value(gene).toArray());  // R 已升序

        QVector<double> got;
        auto genes = result.table.getStringColumn("Gene");
        auto means = result.table.getNumericColumn("Mean");
        ASSERT_EQ(genes.size(), means.size());
        for (int i = 0; i < genes.size(); ++i) {
            if (genes[i] == gene) got.append(means[i]);
        }
        ASSERT_EQ(got.size(), expected.size())
            << "gene '" << gene.toStdString() << "' group-count mismatch (C++=" << got.size()
            << " vs R=" << expected.size() << ")";
        std::sort(got.begin(), got.end());
        for (int i = 0; i < got.size(); ++i) {
            testfix::expect_close(got[i], expected[i]);
        }
    }
}

// geNorm 参考基因选择（导师 2026-10-14 意见第 7 条）：
//   ① 稳定性 M 必须建立在相对量 Q = 2^(minCq−Cq) 上，不是 log2(原始 Cq 相除)；
//   ② 必须逐步剔除最不稳定基因，而不是只算一轮 M 后取最小的两个。
// 期望值由 generate_reference.R 按 geNorm 原文独立实现（相对量 + 逐步剔除）生成。
// data_genorm.csv 刻意构造成：用原始 Cq 相除、或只算一轮 M，都会选成 G1/G5。
TEST(ExpressionCalculator, GeNormSelectsReferenceGenes) {
    qpcr::CSVParser parser;
    qpcr::DataFrame df = parser.parse(QString(QPCR_TEST_FIXTURES_DIR) + "/data_genorm.csv");
    ASSERT_GT(df.rowCount(), 0);

    auto e = testfix::load_json("expected_stat.json").object().value("genorm").toArray()
                 .first().toObject();
    QStringList expected;
    for (const auto& v : e.value("selected").toArray()) expected << v.toString();

    QStringList got;
    for (const QString& gene : qpcr::ExpressionCalculator::selectReferenceGenesByGeNorm(df)) {
        got << gene;
    }

    QStringList wrong;   // 旧行为（原始 Cq + 单轮 M）会选出的基因
    for (const auto& v : e.value("variant_raw_one_round").toArray()) wrong << v.toString();

    expected.sort();
    got.sort();
    wrong.sort();

    EXPECT_EQ(got.size(), 2);
    EXPECT_EQ(got.join(",").toStdString(), expected.join(",").toStdString());
    // 若实现退化成「原始 Cq 相除」或「只算一轮 M」，本断言会失败
    EXPECT_NE(got.join(",").toStdString(), wrong.join(",").toStdString());
}

// 同一套 geNorm 实现跑在论文示例数据（96 孔，6 个基因 × 16 个样品）上，
// 与 R 参考实现（generate_reference.R 的 geNormSelect）对齐。
// 顺带记录：在这份真实数据上四种组合选出的基因集合恰好相同（OsRBBI2 / OsUBQ，
// 也正是验证脚本手选的那对内参），所以示例数据的结论不受本次改正影响。
TEST(ExpressionCalculator, GeNormMatchesReferenceOnExampleData) {
    qpcr::CSVParser parser;
    auto cq  = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/rqpcr_cq.csv");
    auto des = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/rqpcr_design.csv");
    ASSERT_GT(cq.rowCount(), 0);
    ASSERT_GT(des.rowCount(), 0);

    // 按 Position 合并出 geNorm 需要的五列（Gene/Group/BioRep/TechRep/Cq）
    QList<QVariant> geneV, groupV, biorepV, techrepV, cqV;
    for (int i = 0; i < des.rowCount(); ++i) {
        const QString pos = des.get(i, "Position").toString();
        double cqValue = std::numeric_limits<double>::quiet_NaN();
        for (int j = 0; j < cq.rowCount(); ++j) {
            if (cq.get(j, "Position").toString() == pos) {
                cqValue = cq.get(j, "Cq").toDouble();
                break;
            }
        }
        geneV.append(des.get(i, "Gene"));
        groupV.append(des.get(i, "Group"));
        biorepV.append(des.get(i, "BioRep"));
        techrepV.append(des.get(i, "TechRep"));
        cqV.append(QVariant(cqValue));
    }
    qpcr::DataFrame df;
    df.addColumn("Gene", geneV);
    df.addColumn("Group", groupV);
    df.addColumn("BioRep", biorepV);
    df.addColumn("TechRep", techrepV);
    df.addColumn("Cq", cqV);

    auto e = testfix::load_json("expected_stat.json").object().value("genorm_example")
                 .toArray().first().toObject();
    QStringList expected;
    for (const auto& v : e.value("selected").toArray()) expected << v.toString();

    QStringList got;
    for (const QString& gene : qpcr::ExpressionCalculator::selectReferenceGenesByGeNorm(df)) {
        got << gene;
    }
    expected.sort();
    got.sort();

    EXPECT_EQ(got.join(",").toStdString(), expected.join(",").toStdString());
}

// 有意偏离的记录：OsWAK91 在示例数据上有数值相同的生物学重复，
// R 包口径（按数值去重）给 3.6823903149，修正口径给 3.3319094218（相对差 ~10%）。
TEST(ExpressionCalculator, RqPCRGroupMeansDeviateFromRPackage) {
    auto fix = testfix::load_json("expected_expression_rqpcr.json").object();
    auto oldStyle = fix.value("by_gene_r_original").toObject();
    auto newStyle = fix.value("by_gene").toObject();

    QVector<double> oldWak = testfix::to_doubles(oldStyle.value("OsWAK91").toArray());
    QVector<double> newWak = testfix::to_doubles(newStyle.value("OsWAK91").toArray());
    ASSERT_EQ(oldWak.size(), 2);
    ASSERT_EQ(newWak.size(), 2);

    EXPECT_NEAR(oldWak[1], 3.68239031492738, 1e-9);   // R 包 mean(unique(...)) 口径
    EXPECT_NEAR(newWak[1], 3.33190942175262, 1e-9);   // 修正口径（全部生物学重复）
    EXPECT_GT(std::abs(oldWak[1] - newWak[1]), 0.3);
}

// RqPCR 组内汇总必须按**样本身份**去重，不能按数值去重（导师 2026-10-14 意见第 8 条）：
// 两个生物学重复的表达量碰巧相同（例如 1、1、4）仍是三个独立观测。
// 数据构造（Eff = 2；REF 为参考基因，G1 为目标基因）：
//   BioRep 1/2：REF CK=20 / Treatment=22 → QCq_T = 0.25；G1 CK=25 / Treatment=28 → QCq_T = 0.125
//               → 归一化因子 Treatment = 0.25 → G1 表达量 Treatment = 0.5（两个重复完全相同）
//   BioRep 3  ：REF CK=20 / Treatment=26 → QCq_T = 0.015625；G1 CK=25 / Treatment=31 → QCq_T = 0.015625
//               → 归一化因子 = 0.015625 → G1 表达量 Treatment = 1.0
// 于是 Treatment 组三个重复表达量 = 0.5、0.5、1.0：
//   正确均值 = (0.5+0.5+1.0)/3 = 2/3 → 按基因内最小组均值(2/3)归一化后 CK = 1/(2/3) = 1.5
//   旧行为（mean(unique(...))）= (0.5+1.0)/2 = 0.75 → CK = 1/0.75 = 1.3333…（会把结论夸大）
TEST(ExpressionCalculator, RqPCRGroupMeanKeepsIdenticalReplicates) {
    qpcr::DataFrame df;
    QList<QVariant> genes, groups, bioreps, techreps, effs, cqs;
    auto add = [&](const QString& gene, const QString& group, int biorep, double cq) {
        genes << QVariant(gene);
        groups << QVariant(group);
        bioreps << QVariant(biorep);
        techreps << QVariant(1);
        effs << QVariant(2.0);
        cqs << QVariant(cq);
    };
    for (int b = 1; b <= 3; ++b) {
        add("REF", "CK", b, 20.0);
        add("REF", "Treatment", b, b <= 2 ? 22.0 : 26.0);
        add("G1", "CK", b, 25.0);
        add("G1", "Treatment", b, b <= 2 ? 28.0 : 31.0);
    }
    df.addColumn("Gene", genes);
    df.addColumn("Group", groups);
    df.addColumn("BioRep", bioreps);
    df.addColumn("TechRep", techreps);
    df.addColumn("Eff", effs);
    df.addColumn("Cq", cqs);

    qpcr::StandardCurveParams params;
    params.cqTable       = df;
    params.referenceGene = "REF";
    params.controlGroup  = "CK";

    qpcr::ExpressionResult result =
        qpcr::ExpressionCalculator::calculateByStandardCurve(params, "t.test");
    ASSERT_GT(result.table.rowCount(), 0);

    double ckMean = std::numeric_limits<double>::quiet_NaN();
    double treatMean = std::numeric_limits<double>::quiet_NaN();
    for (int i = 0; i < result.table.rowCount(); ++i) {
        if (result.table.get(i, "Gene").toString() != "G1") continue;
        if (result.table.get(i, "Group").toString() == "CK") {
            ckMean = result.table.get(i, "Mean").toDouble();
        } else {
            treatMean = result.table.get(i, "Mean").toDouble();
        }
    }
    EXPECT_NEAR(treatMean, 1.0, 1e-12);          // 最小组被归一化为 1
    EXPECT_NEAR(ckMean, 1.5, 1e-12);             // = 1 / (2/3)
    EXPECT_GT(std::abs(ckMean - 4.0 / 3.0), 1e-6);   // 旧行为（按数值去重）会给 1.3333

    // 原始表里 Treatment 组应保留 3 个生物学重复（没有被数值去重吃掉）
    int treatReps = 0;
    for (int i = 0; i < result.rawData.rowCount(); ++i) {
        if (result.rawData.get(i, "Gene").toString() == "G1" &&
            result.rawData.get(i, "Group").toString() == "Treatment") {
            ++treatReps;
        }
    }
    EXPECT_EQ(treatReps, 3);
}

// performANOVA 的输出行序必须由输入决定。旧实现遍历 QSet，行序随每次进程启动的
// 随机哈希种子变化（2026-10-15 实测 validation 输出两次运行不一致）。
TEST(ExpressionCalculator, AnovaStatisticsFollowInputOrder) {
    qpcr::DataFrame df;
    const QStringList genes  = {"G3", "G3", "G1", "G1", "G1", "G3"};
    const QStringList groups = {"B",  "A",  "A",  "B",  "A",  "B"};
    const QList<double> values = {1.0, 2.0, 3.0, 4.0, 3.5, 1.5};

    QList<QVariant> geneV, groupV, valueV;
    for (int i = 0; i < genes.size(); ++i) {
        geneV.append(genes[i]);
        groupV.append(groups[i]);
        valueV.append(values[i]);
    }
    df.addColumn("Gene", geneV);
    df.addColumn("Group", groupV);
    df.addColumn("Value", valueV);

    auto res = qpcr::ExpressionCalculator::performANOVA(df, "Gene", "Group", "Value");
    ASSERT_EQ(res.size(), 4);   // 2 个基因 × 2 个组

    // 基因与组都按在输入表中的首次出现顺序，且组序是全局的（第 0 行 G3/B 先用 B）：
    // 基因 G3 → G1；组 B → A
    EXPECT_EQ(res[0].gene.toStdString(), "G3");
    EXPECT_EQ(res[0].group.toStdString(), "B");
    EXPECT_EQ(res[1].gene.toStdString(), "G3");
    EXPECT_EQ(res[1].group.toStdString(), "A");
    EXPECT_EQ(res[2].gene.toStdString(), "G1");
    EXPECT_EQ(res[2].group.toStdString(), "B");
    EXPECT_EQ(res[3].gene.toStdString(), "G1");
    EXPECT_EQ(res[3].group.toStdString(), "A");
}

// ΔΔCt 方法：按文献（Livak & Schmittgen 2001）以**校准样品 = 对照组**为基准，故对照组表达量 ≈ 1
// （每个样本对"对照组平均 ΔCt"取 2^-ΔΔCt，对照组均值略高于 1、几何均值 = 1）。
// 期望值来自 generate_reference.R 的 deltaDeltaCtReference()（对照组基准）；
// R 包 CalExp2ddCt 1.0.1 用「所有组」平均 ΔCt 的旧口径值保留在 fixture 的 by_gene_r_original，
// 两者相差一个常数因子（本示例中 1/0.5872 ≈ 1.703）。同样用升序集合对比。
TEST(ExpressionCalculator, DeltaDeltaCtMatchesReference) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");
    ASSERT_GT(cq.rowCount(), 0);
    ASSERT_GT(design.rowCount(), 0);

    auto fix    = testfix::load_json("expected_expression_ddct.json").object();
    QString ref = fix.value("ref_gene").toString();

    qpcr::DeltaDeltaCtParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = ref;
    params.controlGroup  = "0";

    qpcr::ExpressionResult result =
        qpcr::ExpressionCalculator::calculateByDeltaDeltaCt(params, "t.test");

    auto byGene = fix.value("by_gene").toObject();
    for (const QString& gene : byGene.keys()) {
        SCOPED_TRACE(("expression_ddct gene=" + gene).toStdString());
        QVector<double> expected = testfix::to_doubles(byGene.value(gene).toArray());  // R 已升序

        QVector<double> got;
        auto genes = result.table.getStringColumn("Gene");
        auto means = result.table.getNumericColumn("Mean");
        ASSERT_EQ(genes.size(), means.size());
        for (int i = 0; i < genes.size(); ++i) {
            if (genes[i] == gene) got.append(means[i]);
        }
        ASSERT_EQ(got.size(), expected.size())
            << "gene '" << gene.toStdString() << "' group-count mismatch (C++=" << got.size()
            << " vs R=" << expected.size() << ")";
        std::sort(got.begin(), got.end());
        for (int i = 0; i < got.size(); ++i) {
            testfix::expect_close(got[i], expected[i]);
        }
    }
}

// 校准基准必须是**对照组**（导师 2026-10-14 意见第 2 条 / R 包 issue #5、#1）：
//   ① 对照组表达量应 ≈ 1（旧实现给 0.5872）；
//   ② 换 controlGroup 必须改变结果（旧实现换 ref_group 结果不变）；
//   ③ 旧口径值应等于新值除以对照组旧均值（常数因子）。
TEST(ExpressionCalculator, DeltaDeltaCtCalibratesToControlGroup) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/design.csv");
    ASSERT_GT(cq.rowCount(), 0);

    auto fix = testfix::load_json("expected_expression_ddct.json").object();
    const QString ref = fix.value("ref_gene").toString();
    const QString gene = fix.value("by_gene").toObject().keys().first();

    auto runWith = [&](const QString& control) {
        qpcr::DeltaDeltaCtParams p;
        p.cqTable       = cq;
        p.designTable   = design;
        p.referenceGene = ref;
        p.controlGroup  = control;
        return qpcr::ExpressionCalculator::calculateByDeltaDeltaCt(p, "t.test");
    };
    auto meanOf = [&](const qpcr::ExpressionResult& r, const QString& group) {
        for (int i = 0; i < r.table.rowCount(); ++i) {
            if (r.table.get(i, "Gene").toString() == gene &&
                r.table.get(i, "Group").toString() == group) {
                return r.table.get(i, "Mean").toDouble();
            }
        }
        return std::numeric_limits<double>::quiet_NaN();
    };

    // ① 对照组（"0"）≈ 1
    auto withControl0 = runWith("0");
    EXPECT_NEAR(meanOf(withControl0, "0"), 1.01446560323372, 1e-9);
    EXPECT_LT(std::abs(meanOf(withControl0, "0") - 1.0), 0.05);

    // ② 换对照组（"1"）结果必须变化
    auto withControl1 = runWith("1");
    // 新校准组的均值 ≈ 1（严格说是几何均值 = 1，算术均值略高于 1，取决于该组自身离散度）
    EXPECT_LT(std::abs(meanOf(withControl1, "1") - 1.0), 0.05);
    EXPECT_GT(std::abs(meanOf(withControl1, "0") - meanOf(withControl0, "0")), 0.1);

    // ③ 旧口径（by_gene_r_original）与新口径相差常数因子 1/0.587162224960086
    QVector<double> oldMeans = testfix::to_doubles(
        fix.value("by_gene_r_original").toObject().value(gene).toArray());
    QVector<double> newMeans = testfix::to_doubles(
        fix.value("by_gene").toObject().value(gene).toArray());
    ASSERT_EQ(oldMeans.size(), newMeans.size());
    for (int i = 0; i < oldMeans.size(); ++i) {
        testfix::expect_close(oldMeans[i] * 1.01446560323372 / 0.587162224960086, newMeans[i]);
    }
}

// 标准曲线（效率法）表达量：对应 RqPCR 方法（Hilliou & Tran 2013 / SATQPCR）—— 基于 Eff 的
// QCq = Eff^(min.meanCq - meanCq)、参考基因几何均值校正、按基因内最小组均值归一化。
// 注意：这不是 CalExpCurve（斜率截距法）。用 rqpcr 示例数据（examples/rqpcr_*.csv），升序集合对比。
//
// 期望值来自 generate_reference.R 的 rqpcrReference()（**修正口径**：组内均值/标准差对全部
// 生物学重复计算）。R 包 CalExpRqPCR 用 mean(unique(expression))，会把数值碰巧相同的独立
// 生物学重复合并掉，与原始方法论文（Hilliou & Tran 2013 §2.2.4：对生物学重复取均值；全文
// 未出现按数值去重）不符 —— 属**有意偏离**，旧口径的值保留在 fixture 的 by_gene_r_original 里。
TEST(ExpressionCalculator, StandardCurveRqPCRMatchesReference) {
    qpcr::CSVParser parser;
    qpcr::DataFrame cq     = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/rqpcr_cq.csv");
    qpcr::DataFrame design = parser.parse(QString(QPCR_EXAMPLES_DIR) + "/rqpcr_design.csv");
    ASSERT_GT(cq.rowCount(), 0);
    ASSERT_GT(design.rowCount(), 0);

    auto fix    = testfix::load_json("expected_expression_rqpcr.json").object();
    QString ref = fix.value("ref_gene").toString();

    qpcr::StandardCurveParams params;
    params.cqTable       = cq;
    params.designTable   = design;
    params.referenceGene = ref;
    params.controlGroup  = "CK";

    qpcr::ExpressionResult result =
        qpcr::ExpressionCalculator::calculateByStandardCurve(params, "t.test");

    auto byGene = fix.value("by_gene").toObject();
    for (const QString& gene : byGene.keys()) {
        SCOPED_TRACE(("expression_rqpcr gene=" + gene).toStdString());
        QVector<double> expected = testfix::to_doubles(byGene.value(gene).toArray());  // R 已升序

        QVector<double> got;
        auto genes = result.table.getStringColumn("Gene");
        auto means = result.table.getNumericColumn("Mean");
        ASSERT_EQ(genes.size(), means.size());
        for (int i = 0; i < genes.size(); ++i) {
            if (genes[i] == gene) got.append(means[i]);
        }
        ASSERT_EQ(got.size(), expected.size())
            << "gene '" << gene.toStdString() << "' group-count mismatch (C++=" << got.size()
            << " vs R=" << expected.size() << ")";
        std::sort(got.begin(), got.end());
        for (int i = 0; i < got.size(); ++i) {
            testfix::expect_close(got[i], expected[i]);
        }
    }
}
