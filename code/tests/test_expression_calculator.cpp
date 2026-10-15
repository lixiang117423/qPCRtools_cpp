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

// ΔΔCt 方法：R qPCRtools::CalExp2ddCt 用「全数据均值」做归一化基准（不是对照组），
// 故对照组表达量 ≠ 1。C++ 必须与之对齐，否则对照组被钉死在 1.0。
// 同样用升序集合对比，规避 group 在 C++/R 间字符串/数值表示的差异。
TEST(ExpressionCalculator, DeltaDeltaCtMatchesR) {
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

// 标准曲线（效率法）表达量：对应 R qPCRtools::CalExpRqPCR —— 基于 Eff 的
// QCq = Eff^(min.meanCq - meanCq)、参考基因几何均值校正、按基因内最小组均值归一化。
// 注意：这不是 CalExpCurve（斜率截距法）。用 rqpcr 示例数据（examples/rqpcr_*.csv），
// 升序集合对比。
TEST(ExpressionCalculator, StandardCurveRqPCRMatchesR) {
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
