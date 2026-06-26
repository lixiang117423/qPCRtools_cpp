// ΔCt 表达量计算（对应 R qPCRtools::CalExp2dCt）。
// 从 ExpressionCalculator.cpp 按方法族拆分而来，行为不变。
#include "Core/ExpressionCalculator.h"
#include "ExpressionCalculatorInternal.h"

#include <QDebug>
#include <QHash>
#include <QList>
#include <QVariant>
#include <cmath>

namespace qpcr {

//=============================================================================
// DeltaCt Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByDeltaCt(
    const DeltaCtParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "2^-DeltaCt";

    DataFrame merged = params.cqTable.join(params.designTable, "Position");

    if (!merged.hasColumn("Gene") || !merged.hasColumn("Cq") ||
        !merged.hasColumn("Group") || !merged.hasColumn("BioRep")) {
        qWarning() << "Missing required columns";
        return result;
    }

    QList<QString> sortedGroups = sortedUnique(merged.getStringColumn("Group"));
    QList<QString> sortedGenes = sortedUniqueExclude(merged.getStringColumn("Gene"), params.referenceGene);

    // Compute mean reference Cq per (group, biorep)
    QHash<QString, double> refGeneMeanCq;
    for (const QString& group : sortedGroups) {
        DataFrame groupData = merged.filter([group](const Row& row) {
            return row.value("Group").toString() == group;
        });
        QList<QString> bioReps = sortedUnique(groupData.getStringColumn("BioRep"));

        for (const QString& bioRep : bioReps) {
            DataFrame refData = groupData.filter([&params, &bioRep](const Row& row) {
                return row.value("BioRep").toString() == bioRep &&
                       row.value("Gene").toString() == params.referenceGene;
            });
            if (refData.rowCount() > 0) {
                auto cqValues = refData.getNumericColumn("Cq");
                refGeneMeanCq[group + "_" + bioRep] = computeMean(cqValues);
            }
        }
    }

    // Compute expression per (group, gene, biorep)
    QHash<QString, QHash<QString, QVector<double>>> allData; // group -> gene -> expressions
    QVector<QVariant> finalGroups, finalGenes, finalMeans, finalStdDevs;
    QVector<QVariant> rawGenes, rawGroups, rawBioReps, rawExpressions, rawMeans, rawSDs;

    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            QVector<double> expressions;

            DataFrame groupData = merged.filter([group](const Row& row) {
                return row.value("Group").toString() == group;
            });
            QList<QString> bioReps = sortedUnique(groupData.getStringColumn("BioRep"));

            for (const QString& bioRep : bioReps) {
                QString refKey = group + "_" + bioRep;
                if (!refGeneMeanCq.contains(refKey)) continue;

                double meanRefCq = refGeneMeanCq[refKey];
                DataFrame targetData = groupData.filter([&bioRep, &gene](const Row& row) {
                    return row.value("BioRep").toString() == bioRep &&
                           row.value("Gene").toString() == gene;
                });

                if (targetData.rowCount() > 0) {
                    for (double targetCq : targetData.getNumericColumn("Cq")) {
                        double expression = std::pow(2.0, meanRefCq - targetCq);
                        expressions.append(expression);
                        rawGenes.append(gene);
                        rawGroups.append(group);
                        rawBioReps.append(bioRep);
                        rawExpressions.append(expression);
                    }
                }
            }

            if (expressions.isEmpty()) continue;

            allData[group][gene] = expressions;
            double mean = computeMean(expressions);
            double sd = computeStdDev(expressions);

            finalGroups.append(group);
            finalGenes.append(gene);
            finalMeans.append(mean);
            finalStdDevs.append(sd);

            for (int i = 0; i < rawGenes.size(); ++i) {
                if (rawGenes[i].toString() == gene && rawGroups[i].toString() == group && rawMeans.size() <= i) {
                    rawMeans.append(mean);
                    rawSDs.append(sd);
                }
            }
        }
    }

    // Statistical tests
    QString refGroup = params.controlGroup.isEmpty() ? (sortedGroups.isEmpty() ? "" : sortedGroups[0]) : params.controlGroup;

    if (statMethod == "anova") {
        qWarning() << "ANOVA not yet supported for DeltaCt method, using t.test instead";
    }
    runPairwiseTests(allData, sortedGenes, sortedGroups, refGroup, statMethod, result.statistics);

    result.table = buildResultTableWithPValues(finalGenes, finalGroups, finalMeans, finalStdDevs, result.statistics);

    // Raw data table
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
