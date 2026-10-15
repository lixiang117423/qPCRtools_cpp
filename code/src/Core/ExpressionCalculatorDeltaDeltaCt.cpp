// ΔΔCt 表达量计算（对应 R qPCRtools::CalExp2ddCt）。
// 从 ExpressionCalculator.cpp 按方法族拆分而来，行为不变。
#include "Core/ExpressionCalculator.h"
#include "ExpressionCalculatorInternal.h"

#include <QDebug>
#include <QHash>
#include <QList>
#include <QVariant>
#include <algorithm>
#include <cmath>

namespace qpcr {

//=============================================================================
// DeltaDeltaCt Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByDeltaDeltaCt(
    const DeltaDeltaCtParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "2^-DeltaDeltaCt";

    DataFrame merged = params.cqTable.join(params.designTable, "Position");

    if (!merged.hasColumn("Gene") || !merged.hasColumn("Cq") ||
        !merged.hasColumn("Group") || !merged.hasColumn("BioRep")) {
        qWarning() << "Missing required columns";
        return result;
    }

    QList<QString> sortedGenes = sortedUniqueExclude(merged.getStringColumn("Gene"), params.referenceGene);
    QList<QString> sortedGroups = sortedUnique(merged.getStringColumn("Group"));

    // Compute the GLOBAL mean DeltaCt across ALL groups per gene — this is the
    // normalization baseline R qPCRtools::CalExp2ddCt actually uses: it averages
    // the target and reference gene over every group (see CalExp2ddCt.R lines
    // 102-111), NOT just the control group. Pinning to the control group instead
    // would force the control group toward 1.0 and diverge from R by a constant
    // factor. (controlGroup is still consumed below for the statistical tests.)
    QHash<QString, double> globalDeltaCtByGene;
    for (const QString& gene : sortedGenes) {
        DataFrame geneData = merged.filter(
            [&params, &gene](const Row& row) {
                return row.value("Gene").toString() == gene ||
                       row.value("Gene").toString() == params.referenceGene;
            });

        QVector<double> targetCqs, refCqs;
        for (int i = 0; i < geneData.rowCount(); ++i) {
            QString g = geneData.get(i, "Gene").toString();
            double cq = geneData.get(i, "Cq").toDouble();
            if (g == gene) targetCqs.append(cq);
            else if (g == params.referenceGene) refCqs.append(cq);
        }

        if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
            globalDeltaCtByGene[gene] = computeMean(targetCqs) - computeMean(refCqs);
        }
    }

    // 一次遍历建立 (group, gene, biorep) -> Cq 值索引，取代旧实现中每个
    // (group, gene, biorep) 组合都全表 filter 一遍的 O(n·g·r) 扫描。
    // 桶内按行序追加，与 filter 的行序一致 → 数值结果不变。
    const QString REF = params.referenceGene;
    QHash<QString, QVector<double>> cqIndex;             // key: group\x1fgene\x1fbiorep
    QHash<QString, QSet<QString>> biorepsByGroupGene;    // key: group\x1fgene
    for (int i = 0; i < merged.rowCount(); ++i) {
        const QString g = merged.get(i, "Gene").toString();
        const QString grp = merged.get(i, "Group").toString();
        const QString rep = merged.get(i, "BioRep").toString();
        bool ok = false;
        const double cq = merged.get(i, "Cq").toDouble(&ok);
        if (!ok) continue;
        const QString key = grp + QChar(0x1f) + g + QChar(0x1f) + rep;
        cqIndex[key].append(cq);
        biorepsByGroupGene[grp + QChar(0x1f) + g].insert(rep);
    }

    // Compute expression for all (group, gene, biorep) in one pass
    // allData: group -> gene -> expressions (per biorep)
    QHash<QString, QHash<QString, QVector<double>>> allData;
    // For raw data table: organized by group -> gene -> biorep -> expression
    QHash<QString, QHash<QString, QHash<QString, double>>> organizedData;

    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            // 需要同时有目标基因与参考基因的生物学重复（与旧实现语义一致）
            QSet<QString> candidateReps = biorepsByGroupGene[group + QChar(0x1f) + gene]
                                        + biorepsByGroupGene[group + QChar(0x1f) + REF];
            QList<QString> bioReps = candidateReps.values();
            std::sort(bioReps.begin(), bioReps.end());

            // 按 bioRep 顺序收集 (bioRep, expression)，便于异常值过滤后保留身份
            QVector<QPair<QString, double>> repExpressions;
            repExpressions.reserve(bioReps.size());

            for (const QString& bioRep : bioReps) {
                const QVector<double> targetCqs =
                    cqIndex.value(group + QChar(0x1f) + gene + QChar(0x1f) + bioRep);
                const QVector<double> refCqs =
                    cqIndex.value(group + QChar(0x1f) + REF + QChar(0x1f) + bioRep);

                if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
                    double deltaCt = computeMean(targetCqs) - computeMean(refCqs);
                    double deltaDeltaCt = deltaCt - globalDeltaCtByGene.value(gene, 0.0);
                    double expression = std::pow(2.0, -deltaDeltaCt);
                    repExpressions.append({bioRep, expression});
                }
            }

            if (repExpressions.isEmpty()) continue;

            // 异常值过滤（R CalExp2ddCt 语义：按 (group, gene) 对 expression 做 IQR 过滤，
            // 移除对应 bioRep）。默认 false，行为与未开启时完全一致。
            QVector<double> values;
            for (const auto& rep : repExpressions) values.append(rep.second);
            QVector<double> keepValues =
                params.removeOutliers ? removeOutliers(values) : values;

            if (keepValues.size() < values.size()) {
                QVector<QPair<QString, double>> survivors;
                QVector<double> remaining = keepValues;
                for (const auto& rep : repExpressions) {
                    const int idx = remaining.indexOf(rep.second);
                    if (idx >= 0) {
                        survivors.append(rep);
                        remaining.removeAt(idx);
                    }
                }
                repExpressions = survivors;
            }

            for (const auto& rep : repExpressions) {
                allData[group][gene].append(rep.second);
                organizedData[group][gene][rep.first] = rep.second;
            }
        }
    }

    // Build summary table
    QVector<QVariant> summaryGroups, summaryGenes, summaryMeans, summaryStdDevs;

    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            QVector<double> values = allData[group][gene];
            if (values.isEmpty()) continue;

            summaryGroups.append(group);
            summaryGenes.append(gene);
            summaryMeans.append(computeMean(values));
            summaryStdDevs.append(computeStdDev(values));
        }
    }

    // Statistical tests
    // 对照组只用于统计检验；为空时回退到第一组（与 ΔCt 行为一致，避免统计结果静默为空）
    const QString refGroup = params.controlGroup.isEmpty()
        ? (sortedGroups.isEmpty() ? QString() : sortedGroups.first())
        : params.controlGroup;

    if (statMethod == "anova") {
        runAnovaTests(allData, sortedGenes, sortedGroups, result.statistics);
    } else {
        runPairwiseTests(allData, sortedGenes, sortedGroups, refGroup, statMethod, result.statistics);
    }

    result.table = buildResultTableWithPValues(summaryGenes, summaryGroups, summaryMeans, summaryStdDevs, result.statistics);

    // Build raw data table (sorted: Gene -> Group -> BioRep)
    QVector<QVariant> rawGenes, rawGroups, rawBioReps, rawExpressions, rawMeans, rawSDs;

    for (const QString& gene : sortedGenes) {
        for (const QString& group : sortedGroups) {
            if (!organizedData.contains(group) || !organizedData[group].contains(gene)) continue;

            QList<QString> bioRepsInGroup = organizedData[group][gene].keys();
            std::sort(bioRepsInGroup.begin(), bioRepsInGroup.end());

            QString key = group + "_" + gene;
            QVector<double> vals = allData[group][gene];
            double mean = computeMean(vals);
            double sd = computeStdDev(vals);

            for (const QString& bioRep : bioRepsInGroup) {
                rawGenes.append(gene);
                rawGroups.append(group);
                rawBioReps.append(bioRep);
                rawExpressions.append(organizedData[group][gene][bioRep]);
                rawMeans.append(mean);
                rawSDs.append(sd);
            }
        }
    }

    DataFrame rawData;
    rawData.addColumn("Gene", rawGenes);
    rawData.addColumn("Group", rawGroups);
    rawData.addColumn("BioRep", rawBioReps);
    rawData.addColumn("Expression", rawExpressions);
    rawData.addColumn("Mean", rawMeans);
    rawData.addColumn("SD", rawSDs);
    result.rawData = rawData;

    return result;
}

}  // namespace qpcr
