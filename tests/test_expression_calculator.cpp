// ExpressionCalculator 集成测试：与 R qPCRtools 的表达量结果对比（golden fixtures）。
#include <gtest/gtest.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>

#include <algorithm>

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
