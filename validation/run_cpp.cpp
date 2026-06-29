// validation/run_cpp.cpp
//
// qPCRtools 等价性验证 driver：链接 qpcr_core 核心算法库，用与 R 版
// (validation/run_R.R) 完全相同的输入数据，输出 CSV 供逐项对比。
//
// 构建：cmake -B build-val -DQPCRTOOLS_BUILD_VALIDATION=ON .
//        && cmake --build build-val --target run_cpp
// 运行（仓库根目录）：./build-val/run_cpp
//
// 覆盖方法：ΔΔCt、标准曲线（CalCurve）。RqPCR 的 C++ 入口
// (calculateByStandardCurve) 需要 curveTable，数据流待确认，暂标注 TODO。

#include "Data/DataFrame.h"
#include "Data/CSVParser.h"
#include "Core/ExpressionCalculator.h"
#include "Core/StandardCurve.h"

#include <QCoreApplication>
#include <QDir>
#include <fstream>
#include <iostream>

using namespace qpcr;

static void writeExpressionResult(const QString& tag, const ExpressionResult& res) {
    QDir().mkpath("validation/results");
    QString path = QString("validation/results/cpp_%1.csv").arg(tag);
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

        ExpressionResult res = ExpressionCalculator::calculateByDeltaDeltaCt(p, "t.test");
        writeExpressionResult("deltadeltaCt", res);
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

        ExpressionResult res = ExpressionCalculator::calculateByStandardCurve(p, "t.test");
        writeExpressionResult("rqpcr", res);
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

        std::ofstream ofs("validation/results/cpp_standardcurve.csv");
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
        std::cout << "[cpp] standard curve -> validation/results/cpp_standardcurve.csv"
                  << std::endl;
    }

    std::cout << "[cpp] done." << std::endl;
    return 0;
}
