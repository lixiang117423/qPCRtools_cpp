// validation/run_cpp.cpp
//
// qPCRtools 等价性验证 driver：链接 qpcr_core 核心算法库，用 examples 真实数据
// 跑与 R 版相同的输入，输出 CSV 供逐项对比（见 validation/compare.py）。
//
// 构建：cmake -B build-val -DQPCRTOOLS_BUILD_VALIDATION=ON . \
//        && cmake --build build-val --target run_cpp
// 运行（仓库根目录）：./build-val/run_cpp
//
// 说明：本 driver 仅链接 qpcr_core（Qt6::Core），不依赖 GUI。
// 当前完整实现 ΔΔCt（examples 有数据）；RqPCR 与标准曲线因缺数据/需确认
// curveTable 数据流而标注 TODO，待数据齐备后启用（见注释）。

#include "Data/DataFrame.h"
#include "Core/ExpressionCalculator.h"
#include "Core/StandardCurve.h"

#include <QCoreApplication>
#include <QDir>
#include <iostream>

using namespace qpcr;

static void writeExpressionResult(const QString& tag, const ExpressionResult& res) {
    QDir().mkpath("validation/results");
    QString path = QString("validation/results/cpp_%1.csv").arg(tag);
    res.table.saveCSV(path);

    std::cout << "[cpp] " << tag.toStdString()
              << "  rows=" << res.table.rowCount()
              << "  -> " << path.toStdString() << std::endl;
    // 打印统计量供人工核对
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

    // ---- 方法 1: ΔΔCt（对应 R CalExp2ddCt）----
    // 数据：examples/cq.csv（Position,Gene,Cq）+ examples/design.csv（Position,Group,BioRep）
    {
        DataFrame cq     = DataFrame::fromCSV("examples/cq.csv");
        DataFrame design = DataFrame::fromCSV("examples/design.csv");

        DeltaDeltaCtParams p;
        p.cqTable        = cq;
        p.designTable    = design;
        p.referenceGene  = "Beta Actin";   // examples/README：内参基因
        p.controlGroup   = "0";            // design.csv 的 Group 取值：0 / 0.5 / 1
        p.removeOutliers = false;

        ExpressionResult res = ExpressionCalculator::calculateByDeltaDeltaCt(p, "t.test");
        writeExpressionResult("deltadeltaCt", res);
    }

    // ---- 方法 2: RqPCR（对应 R CalExpRqPCR）----
    // TODO(数据流): calculateByStandardCurve 需要 curveTable；rqpcr_design.csv 含 Eff 列。
    //   待确认 curveTable 的确切来源（由 StandardCurve::calculate 结果构造，还是直接以
    //   Eff 列驱动），补全后取消下面注释并输出 cpp_rqpcr.csv。
    // DataFrame rqCq     = DataFrame::fromCSV("examples/rqpcr_cq.csv");
    // DataFrame rqDesign = DataFrame::fromCSV("examples/rqpcr_design.csv");

    // ---- 方法 3: 标准曲线 CalCurve（对应 R CalCurve）----
    // TODO(数据): StandardCurve::calculate 需要稀释梯度浓度表（Position,Gene,Conc），
    //   examples 暂无该数据；待用户提供稀释梯度后启用，输出 cpp_standardcurve.csv。
    // DataFrame curveCq  = DataFrame::fromCSV("examples/curve_cq.csv");
    // DataFrame curveConc= DataFrame::fromCSV("examples/curve_conc.csv");
    // auto sc = StandardCurve::calculate(curveCq, curveConc, /*lowestConcen*/1,
    //                                    /*highestConcen*/10000, /*dilution*/10);

    std::cout << "[cpp] done." << std::endl;
    return 0;
}
