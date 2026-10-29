// validation/run_cpp.cpp
//
// qPCRtools 等价性验证 driver：链接 qpcr_core 核心算法库，用与 R 版
// 独立 base-R 参照完全相同的输入数据，输出 CSV 供逐项对比。
//
// 构建（在 code/ 目录下）：cmake -S . -B ../build-val -DQPCRTOOLS_BUILD_VALIDATION=ON
//                          && cmake --build ../build-val --target run_cpp
// 运行（在 code/ 目录下）：../build-val/run_cpp [输出文件名后缀]
//   不给后缀 → 写 cpp_deltadeltaCt.csv 等（历史文件名，对应 1.1.0 时代的口径）；
//   给后缀（如 v12）→ 写 cpp_deltadeltaCt_v12.csv 等，便于在不覆盖历史结果的前提下
//   重新生成当前版本（1.3.1，ΔΔCt 校准基准＝对照组）的输出，供统计层对照使用。
//
// 覆盖方法：ΔΔCt、RqPCR（效率法）与标准曲线（CalCurve）。
//
// 2026-10-29（第二轮审稿）：
//   ① 不再把“与未修正 R 包数值一致”当作正确性证据。当前版本的结果由
//      validation/reference_base_r.R 从原始 Cq 独立重算并逐项比对；
//   ② t 检验/秩和检验默认输出 Holm 校正后的 PValue，同时保留 RawPValue；
//   ③ 不加后缀默认输出当前版本（不再写 1.1.0 时代的同名旧口径文件）。

#include "Data/DataFrame.h"
#include "Data/CSVParser.h"
#include "Core/ExpressionCalculator.h"
#include "Core/StandardCurve.h"

#include <QCoreApplication>
#include <QDir>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace qpcr;

// 输出文件名后缀（命令行第 1 个参数，可为空）
static QString g_suffix;

static void writeExpressionResult(const QString& tag, const ExpressionResult& res) {
    QDir().mkpath("validation/results");
    QString path = QString("validation/results/cpp_%1%2.csv").arg(tag, g_suffix);
    res.table.saveCSV(path);

    std::cout << "[cpp] " << tag.toStdString()
              << "  rows=" << res.table.rowCount()
              << "  -> " << path.toStdString() << std::endl;
    for (const StatisticalResult& s : res.statistics) {
        std::cout << "    " << s.gene.toStdString()
                  << " [" << s.group1.toStdString()
                  << " vs " << s.group2.toStdString()
                  << "] p=" << s.pValue
                  << " (" << s.significance.toStdString() << ")"
                  << std::endl;
    }
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QDir().mkpath("validation/results");
    if (argc > 1) {
        g_suffix = QString("_") + QString::fromLocal8Bit(argv[1]);
    }

    // ---- 方法 1: ΔΔCt（对应 R CalExp2ddCt）----
    // 数据：examples/cq.csv + examples/design.csv
    {
        DataFrame cq     = DataFrame::fromCSV("examples/cq.csv");
        DataFrame design = DataFrame::fromCSV("examples/design.csv");

        DeltaDeltaCtParams p;
        p.cqTable        = cq;
        p.designTable    = design;
        p.referenceGene  = "Beta Actin";
        p.controlGroup   = "0";
        p.removeOutliers = false;

        ExpressionResult resHolm = ExpressionCalculator::calculateByDeltaDeltaCt(
            p, "t.test", "holm");
        writeExpressionResult("deltadeltaCt_holm", resHolm);

        ExpressionResult resRaw = ExpressionCalculator::calculateByDeltaDeltaCt(
            p, "t.test", "none");
        writeExpressionResult("deltadeltaCt_raw", resRaw);

        // 统计层：ANOVA + Tukey HSD（字母标记），供与 R 的 TukeyHSD / 独立参照对照。
        // 检验尺度与 t.test 一致（线性相对表达量），只是换成单因素方差分析 + Tukey。
        ExpressionResult resAnova = ExpressionCalculator::calculateByDeltaDeltaCt(p, "anova");
        writeExpressionResult("deltadeltaCt_anova", resAnova);
    }

    // ---- 方法 2: RqPCR（对应 R CalExpRqPCR）----
    // calculateByStandardCurve 实际只用 cqTable + designTable(含 Eff)；curveTable 未使用。
    // 参考基因 OsUBQ+OsRBBI2 与 R(CalExpRqPCR, ref_gene=NULL) 的自动选择一致
    // (R 输出中 OsUBQ/OsRBBI2 未作为目标基因出现)。
    {
        DataFrame rqCq     = DataFrame::fromCSV("examples/rqpcr_cq.csv");
        DataFrame rqDesign = DataFrame::fromCSV("examples/rqpcr_design.csv");

        StandardCurveParams p;
        p.cqTable       = rqCq;
        p.designTable   = rqDesign;
        p.curveTable    = DataFrame();
        p.referenceGene = "OsUBQ,OsRBBI2";
        p.controlGroup  = "CK";

        ExpressionResult resHolm = ExpressionCalculator::calculateByStandardCurve(
            p, "t.test", "holm");
        writeExpressionResult("rqpcr_holm", resHolm);

        ExpressionResult resRaw = ExpressionCalculator::calculateByStandardCurve(
            p, "t.test", "none");
        writeExpressionResult("rqpcr_raw", resRaw);
    }

    // ---- 方法 3: 标准曲线 CalCurve（对应 R CalCurve）----
    // 数据：web/calsc.cq.txt (Position,Cq) + web/calsc.info.txt (Position,Gene,Conc)
    // tab 分隔；与 R 版 demo 同源，lowest=4, highest=4096, dilution=4, byMean=true。
    {
        CSVParser parser;
        parser.setSeparator('\t');
        DataFrame scCq   = parser.parse("web/calsc.cq.txt");
        DataFrame scInfo = parser.parse("web/calsc.info.txt");

        QVector<StandardCurveResult> scResults = StandardCurve::calculate(
            scCq, scInfo, /*lowestConcen*/4.0, /*highestConcen*/4096.0,
            /*dilution*/4.0, /*byMean*/true);

        std::ofstream ofs(QString("validation/results/cpp_standardcurve%1.csv")
                              .arg(g_suffix).toStdString());
        ofs << std::setprecision(17);
        ofs << "Gene,Slope,Intercept,RSquared,PValue,Efficiency\n";
        for (const StandardCurveResult& sc : scResults) {
            ofs << sc.gene.toStdString() << ","
                << sc.slope << "," << sc.intercept << ","
                << sc.rSquared << "," << sc.pValue << ","
                << sc.efficiency << "\n";
            std::cout << "    " << sc.gene.toStdString()
                      << "  slope=" << sc.slope
                      << "  R2=" << sc.rSquared
                      << "  E=" << sc.efficiency
                      << "  (" << sc.formula.toStdString() << ")\n";
        }
        std::cout << "[cpp] standard curve -> validation/results/cpp_standardcurve"
                  << g_suffix.toStdString() << ".csv"
                  << std::endl;
    }

    std::cout << "[cpp] done." << std::endl;
    return 0;
}
