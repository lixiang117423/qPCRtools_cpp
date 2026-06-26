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

    // Compute mean DeltaCt for control group per gene
    QHash<QString, double> controlDeltaCtByGene;
    for (const QString& gene : sortedGenes) {
        DataFrame controlData = merged.filter(
            [&params, &gene](const Row& row) {
                return row.value("Group").toString() == params.controlGroup &&
                       (row.value("Gene").toString() == gene ||
                        row.value("Gene").toString() == params.referenceGene);
            });

        QVector<double> targetCqs, refCqs;
        for (int i = 0; i < controlData.rowCount(); ++i) {
            QString g = controlData.get(i, "Gene").toString();
            double cq = controlData.get(i, "Cq").toDouble();
            if (g == gene) targetCqs.append(cq);
            else if (g == params.referenceGene) refCqs.append(cq);
        }

        if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
            controlDeltaCtByGene[gene] = computeMean(targetCqs) - computeMean(refCqs);
        }
    }

    // Compute expression for all (group, gene, biorep) in one pass
    // allData: group -> gene -> expressions (per biorep)
    QHash<QString, QHash<QString, QVector<double>>> allData;
    // For raw data table: organized by gene -> group -> biorep -> expression
    QHash<QString, QHash<QString, QHash<QString, double>>> organizedData;

    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            DataFrame groupData = merged.filter(
                [&group, &gene, &params](const Row& row) {
                    return row.value("Group").toString() == group &&
                           (row.value("Gene").toString() == gene ||
                            row.value("Gene").toString() == params.referenceGene);
                });

            QList<QString> bioReps = sortedUnique(groupData.getStringColumn("BioRep"));

            for (const QString& bioRep : bioReps) {
                DataFrame repData = groupData.filter(
                    [&bioRep](const Row& row) {
                        return row.value("BioRep").toString() == bioRep;
                    });

                QVector<double> targetCqs, refCqs;
                for (int i = 0; i < repData.rowCount(); ++i) {
                    QString g = repData.get(i, "Gene").toString();
                    double cq = repData.get(i, "Cq").toDouble();
                    if (g == gene) targetCqs.append(cq);
                    else if (g == params.referenceGene) refCqs.append(cq);
                }

                if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
                    double deltaCt = computeMean(targetCqs) - computeMean(refCqs);
                    double deltaDeltaCt = deltaCt - controlDeltaCtByGene.value(gene, 0.0);
                    double expression = std::pow(2.0, -deltaDeltaCt);

                    allData[group][gene].append(expression);
                    organizedData[group][gene][bioRep] = expression;
                }
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
    if (statMethod == "anova") {
        runAnovaTests(allData, sortedGenes, sortedGroups, result.statistics);
    } else {
        runPairwiseTests(allData, sortedGenes, sortedGroups, params.controlGroup, statMethod, result.statistics);
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
