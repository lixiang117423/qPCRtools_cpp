#include "Core/ExpressionCalculator.h"
#include "Core/StatisticalTest.h"
#include "ExpressionCalculatorInternal.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

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

//=============================================================================
// Standard Curve Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByStandardCurve(
    const StandardCurveParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "Standard Curve Expression";

    DataFrame merged = params.cqTable;
    if (params.designTable.columnCount() > 0) {
        merged = mergeByPosition(params.cqTable, params.designTable);
    }

    QSet<QString> genesSet = getUniqueValues(merged, "Gene");
    QSet<QString> groupsSet = getUniqueValues(merged, "Group");
    QSet<QString> biorepsSet = getUniqueValues(merged, "BioRep");

    QVector<QString> genes = genesSet.values();
    QVector<QString> groups = groupsSet.values();
    QVector<QString> bioreps = biorepsSet.values();

    struct QCqData {
        QString biorep, group, gene;
        double meanCq, sdCq, minMeanCq, eff, qCq, sdQCq;
    };

    QVector<QCqData> qcqDataList;

    double globalMinMeanCq = std::numeric_limits<double>::max();
    QMap<QString, QMap<QString, double>> allGroupMeanCq;

    for (const QString& gene : genes) {
        for (const QString& group : groups) {
            QVector<double> cqValues = getFilteredValues(merged, "Gene", gene, "Group", group, "", "", "Cq");
            if (!cqValues.isEmpty()) {
                double mean = computeMean(cqValues);
                allGroupMeanCq[gene][group] = mean;
                if (mean < globalMinMeanCq) globalMinMeanCq = mean;
            }
        }
    }

    for (const QString& biorep : bioreps) {
        for (const QString& gene : genes) {
            double eff = 2.0;
            for (int i = 0; i < merged.rowCount(); ++i) {
                if (merged.get(i, "Gene").toString() == gene) {
                    bool ok;
                    double e = merged.get(i, "Eff").toString().toDouble(&ok);
                    if (ok && e > 0) { eff = e; break; }
                }
            }

            for (const QString& group : groups) {
                QVector<double> cqValues = getFilteredValues(merged, "BioRep", biorep, "Gene", gene, "Group", group, "Cq");
                if (!cqValues.isEmpty()) {
                    double meanCq = allGroupMeanCq[gene][group];
                    QCqData data;
                    data.biorep = biorep;
                    data.group = group;
                    data.gene = gene;
                    data.meanCq = meanCq;
                    data.sdCq = calculateStandardDeviation(cqValues);
                    data.minMeanCq = globalMinMeanCq;
                    data.eff = eff;
                    data.qCq = std::pow(eff, globalMinMeanCq - meanCq);
                    data.sdQCq = data.sdCq * data.qCq * std::log(eff);
                    qcqDataList.append(data);
                }
            }
        }
    }

    QVector<QString> refGenes;
    if (!params.referenceGene.isEmpty()) {
        refGenes = params.referenceGene.split(',', Qt::SkipEmptyParts);
        for (QString& gene : refGenes) gene = gene.trimmed();
    } else {
        refGenes = selectReferenceGenesByGeNorm(merged);
    }

    struct FactorData {
        QString biorep, group;
        double factor, sdFactor;
    };
    QMap<QString, FactorData> factorMap;

    for (const QString& biorep : bioreps) {
        for (const QString& group : groups) {
            QVector<double> refQCqs;
            double sumVariance = 0.0;

            for (const QString& refGene : refGenes) {
                for (const QCqData& data : qcqDataList) {
                    if (data.biorep == biorep && data.group == group && data.gene == refGene) {
                        refQCqs.append(data.qCq);
                        sumVariance += std::pow(data.sdQCq / (refGenes.size() * data.qCq), 2);
                        break;
                    }
                }
            }

            if (!refQCqs.isEmpty()) {
                double product = 1.0;
                for (double v : refQCqs) product *= v;
                double geoMean = std::pow(product, 1.0 / refQCqs.size());

                FactorData fd;
                fd.biorep = biorep;
                fd.group = group;
                fd.factor = geoMean;
                fd.sdFactor = std::sqrt(sumVariance) * geoMean;
                factorMap[biorep + "_" + group] = fd;
            }
        }
    }

    struct ExpressionData {
        QString group, gene, biorep;
        double bioRepExpression, sd1, meanExpression, sdExpression, seExpression;
    };

    QVector<ExpressionData> expressionList;
    QMap<QString, QVector<double>> groupExpressions;

    for (const QString& gene : genes) {
        if (refGenes.contains(gene)) continue;

        for (const QString& group : groups) {
            QVector<double> bioRepExpressions;

            for (const QString& biorep : bioreps) {
                double qCq = 0, sdQCq = 0;
                bool found = false;
                for (const QCqData& data : qcqDataList) {
                    if (data.biorep == biorep && data.group == group && data.gene == gene) {
                        qCq = data.qCq;
                        sdQCq = data.sdQCq;
                        found = true;
                        break;
                    }
                }

                if (found) {
                    QString key = biorep + "_" + group;
                    if (factorMap.contains(key)) {
                        const FactorData& factor = factorMap[key];
                        double expression = qCq / factor.factor;
                        double sd1 = expression * std::sqrt(std::pow(sdQCq / qCq, 2) + std::pow(factor.sdFactor / factor.factor, 2));

                        bioRepExpressions.append(expression);
                        ExpressionData expData;
                        expData.group = group;
                        expData.gene = gene;
                        expData.biorep = biorep;
                        expData.bioRepExpression = expression;
                        expData.sd1 = sd1;
                        expressionList.append(expData);
                    }
                }
            }

            if (!bioRepExpressions.isEmpty()) {
                groupExpressions[gene + "_" + group] = bioRepExpressions;
            }
        }
    }

    // Normalize to control group and compute stats
    QMap<QString, ExpressionData> finalResults;

    for (const QString& gene : genes) {
        if (refGenes.contains(gene)) continue;

        double controlGroupMean = std::numeric_limits<double>::quiet_NaN();
        QString controlKey = gene + "_" + params.controlGroup;
        if (groupExpressions.contains(controlKey)) {
            controlGroupMean = computeMean(groupExpressions[controlKey]);
        }

        for (const QString& group : groups) {
            QString key = gene + "_" + group;
            if (!groupExpressions.contains(key)) continue;

            const QVector<double>& exps = groupExpressions[key];
            double mean = computeMean(exps);
            double sd = calculateStandardDeviation(exps);
            double se = sd / std::sqrt(exps.size());

            ExpressionData expData;
            expData.group = group;
            expData.gene = gene;

            if (!std::isnan(controlGroupMean) && controlGroupMean > 0) {
                expData.meanExpression = mean / controlGroupMean;
                expData.sdExpression = sd / controlGroupMean;
                expData.seExpression = se / controlGroupMean;
            } else {
                expData.meanExpression = mean;
                expData.sdExpression = sd;
                expData.seExpression = se;
            }
            finalResults[key] = expData;
        }
    }

    // Build result table
    QVector<QVariant> finalGroups, finalGenes, finalMeans, finalStdDevs, finalSE, finalPValues, finalSig;
    for (const ExpressionData& expData : finalResults) {
        finalGroups.append(expData.group);
        finalGenes.append(expData.gene);
        finalMeans.append(expData.meanExpression);
        finalStdDevs.append(expData.sdExpression);
        finalSE.append(expData.seExpression);
        finalPValues.append(QVariant());
        finalSig.append("");
    }

    result.table.addColumn("Group", finalGroups);
    result.table.addColumn("Gene", finalGenes);
    result.table.addColumn("Mean", finalMeans);
    result.table.addColumn("StdDev", finalStdDevs);
    result.table.addColumn("SE", finalSE);
    result.table.addColumn("PValue", finalPValues);
    result.table.addColumn("Significance", finalSig);

    // Build raw data table
    QVector<QVariant> rawGroups, rawGenes, rawBioReps, rawExpr, rawMeans, rawSDs;
    for (const ExpressionData& expData : expressionList) {
        rawGroups.append(expData.group);
        rawGenes.append(expData.gene);
        rawBioReps.append(expData.biorep);
        rawExpr.append(expData.bioRepExpression);

        QString key = expData.gene + "_" + expData.group;
        if (finalResults.contains(key)) {
            rawMeans.append(finalResults[key].meanExpression);
            rawSDs.append(finalResults[key].sdExpression);
        } else {
            rawMeans.append(QVariant());
            rawSDs.append(QVariant());
        }
    }

    result.rawData.addColumn("Group", rawGroups);
    result.rawData.addColumn("Gene", rawGenes);
    result.rawData.addColumn("BioRep", rawBioReps);
    result.rawData.addColumn("Expression", rawExpr);
    result.rawData.addColumn("Mean", rawMeans);
    result.rawData.addColumn("SD", rawSDs);

    // Statistical tests
    if (statMethod == "t.test" || statMethod == "wilcox.test") {
        for (const QString& gene : genes) {
            if (refGenes.contains(gene)) continue;

            for (const QString& group : groups) {
                if (group == params.controlGroup) continue;

                QVector<double> controlData, treatData;
                for (const ExpressionData& expData : expressionList) {
                    if (expData.gene == gene) {
                        if (expData.group == params.controlGroup) controlData.append(expData.bioRepExpression);
                        else if (expData.group == group) treatData.append(expData.bioRepExpression);
                    }
                }

                if (!controlData.isEmpty() && !treatData.isEmpty()) {
                    if (statMethod == "wilcox.test") {
                        result.statistics.append(performWilcoxonTest(treatData, controlData, gene, group, params.controlGroup));
                    } else {
                        result.statistics.append(performTTest(treatData, controlData, gene, group, params.controlGroup));
                    }
                }
            }
        }
    } else if (statMethod == "anova") {
        for (const QString& gene : genes) {
            if (refGenes.contains(gene)) continue;

            QVector<QVariant> anovaGenes, anovaGroups, anovaExpr;
            for (const ExpressionData& expData : expressionList) {
                if (expData.gene == gene) {
                    anovaGenes.append(expData.gene);
                    anovaGroups.append(expData.group);
                    anovaExpr.append(expData.bioRepExpression);
                }
            }

            if (!anovaGenes.isEmpty()) {
                DataFrame anovaData;
                anovaData.addColumn("Gene", anovaGenes);
                anovaData.addColumn("Group", anovaGroups);
                anovaData.addColumn("Expression", anovaExpr);
                QVector<StatisticalResult> anovaResults = performANOVA(anovaData, "Gene", "Group", "Expression");
                for (const StatisticalResult& res : anovaResults) result.statistics.append(res);
            }
        }
    }

    // Merge p-values into result table
    for (int i = 0; i < result.table.rowCount(); ++i) {
        QString group = result.table.get(i, "Group").toString();
        QString gene = result.table.get(i, "Gene").toString();

        if (group == params.controlGroup) {
            result.table.set(i, "PValue", QVariant());
            result.table.set(i, "Significance", "");
            continue;
        }

        for (const StatisticalResult& stat : result.statistics) {
            if (stat.gene == gene && stat.group2 == group) {
                result.table.set(i, "PValue", stat.pValue);
                result.table.set(i, "Significance", stat.significance);
                break;
            }
        }
    }

    return result;
}

//=============================================================================
// Public Helper Functions
//=============================================================================

double ExpressionCalculator::calculateDeltaCt(double targetCq, double refCq)
{
    return targetCq - refCq;
}

double ExpressionCalculator::calculateExpressionFromDeltaCt(double deltaCt)
{
    return std::pow(2.0, -deltaCt);
}

double ExpressionCalculator::calculateExpressionFromDeltaDeltaCt(
    double targetDeltaCt, double controlDeltaCt)
{
    return std::pow(2.0, -(targetDeltaCt - controlDeltaCt));
}

QVector<double> ExpressionCalculator::removeOutliers(const QVector<double>& values)
{
    if (values.size() < 4) return values;

    QVector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());

    int n = sorted.size();
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;

    double lowerBound = q1 - 1.5 * iqr;
    double upperBound = q3 + 1.5 * iqr;

    QVector<double> filtered;
    for (double val : values) {
        if (val >= lowerBound && val <= upperBound) filtered.append(val);
    }
    return filtered;
}

QString ExpressionCalculator::formatSignificance(double pValue)
{
    if (std::isnan(pValue)) return "NA";
    if (pValue < 0.001) return "***";
    if (pValue < 0.01) return "**";
    if (pValue < 0.05) return "*";
    return "NS";
}

//=============================================================================
// Statistical Tests
//=============================================================================

StatisticalResult ExpressionCalculator::performTTest(
    const QVector<double>& group1, const QVector<double>& group2,
    const QString& gene, const QString& group1Name, const QString& group2Name)
{
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    double mean1 = computeMean(group1);
    double mean2 = computeMean(group2);

    double var1 = 0, var2 = 0;
    for (double val : group1) var1 += (val - mean1) * (val - mean1);
    for (double val : group2) var2 += (val - mean2) * (val - mean2);
    var1 /= (group1.size() - 1);
    var2 /= (group2.size() - 1);

    double se = std::sqrt(var1 / group1.size() + var2 / group2.size());

    if (se > 0) {
        result.tStatistic = (mean1 - mean2) / se;
        double absT = std::abs(result.tStatistic);
        if (absT < 1.96) result.pValue = 0.05;
        else if (absT < 2.58) result.pValue = 0.01;
        else if (absT < 3.29) result.pValue = 0.001;
        else result.pValue = 0.0001;
        result.pValue *= 2.0;
        if (result.pValue > 1.0) result.pValue = 1.0;
    } else {
        result.pValue = 1.0;
        result.tStatistic = qQNaN();
    }

    result.significance = formatSignificance(result.pValue);
    return result;
}

StatisticalResult ExpressionCalculator::performWilcoxonTest(
    const QVector<double>& group1, const QVector<double>& group2,
    const QString& gene, const QString& group1Name, const QString& group2Name)
{
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    QVector<double> combined = group1 + group2;
    QVector<int> ranks;

    for (size_t i = 0; i < combined.size(); ++i) {
        int rank = 1;
        for (size_t j = 0; j < combined.size(); ++j) {
            if (combined[j] < combined[i] || (combined[j] == combined[i] && j < i)) rank++;
        }
        ranks.append(rank);
    }

    double sumRanks1 = 0;
    for (int i = 0; i < group1.size(); ++i) sumRanks1 += ranks[i];
    result.wilcoxV = sumRanks1;
    result.pValue = 0.05; // Placeholder
    result.significance = formatSignificance(result.pValue);
    return result;
}

QVector<StatisticalResult> ExpressionCalculator::performANOVA(
    const DataFrame& data, const QString& geneCol, const QString& groupCol, const QString& valueCol)
{
    QVector<StatisticalResult> results;

    auto genes = data.getStringColumn(geneCol);
    auto groups = data.getStringColumn(groupCol);
    QSet<QString> geneSet, groupSet;
    for (const auto& g : genes) geneSet.insert(g);
    for (const auto& g : groups) groupSet.insert(g);

    for (const QString& gene : geneSet) {
        DataFrame geneData = data.filter(
            [&gene, &geneCol](const Row& row) { return row.value(geneCol).toString() == gene; });

        auto values = geneData.getNumericColumn(valueCol);
        double overallMean = computeMean(values);
        double ssBetween = 0, ssWithin = 0;
        int nTotal = values.size();

        for (const QString& group : groupSet) {
            DataFrame groupData = geneData.filter(
                [&group, &groupCol](const Row& row) { return row.value(groupCol).toString() == group; });
            auto groupValues = groupData.getNumericColumn(valueCol);
            double groupMean = computeMean(groupValues);
            ssBetween += groupValues.size() * (groupMean - overallMean) * (groupMean - overallMean);
            for (double val : groupValues) ssWithin += (val - groupMean) * (val - groupMean);
        }

        double dfBetween = groupSet.size() - 1;
        double dfWithin = nTotal - groupSet.size();
        double msBetween = ssBetween / dfBetween;
        double msWithin = ssWithin / dfWithin;
        double fStat = msWithin > 0 ? msBetween / msWithin : qQNaN();

        StatisticalResult result;
        result.gene = gene;
        result.fStatistic = fStat;
        result.pValue = 0.05; // Placeholder
        result.significance = formatSignificance(result.pValue);
        results.append(result);
    }

    return results;
}

QHash<QString, QString> ExpressionCalculator::generateLetterGroups(
    const QHash<QString, double>& groupMeans,
    const QVector<TestResult>& tukeyResults,
    double alpha)
{
    QHash<QString, QString> letterGroups;
    if (groupMeans.isEmpty()) return letterGroups;

    QList<QPair<QString, double>> sortedGroups;
    for (auto it = groupMeans.begin(); it != groupMeans.end(); ++it)
        sortedGroups.append(qMakePair(it.key(), it.value()));
    std::sort(sortedGroups.begin(), sortedGroups.end(),
        [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
            return a.second > b.second;
        });

    QStringList groups;
    for (const auto& pair : sortedGroups) {
        groups.append(pair.first);
        letterGroups[pair.first] = "";
    }

    QHash<QString, QHash<QString, bool>> sigMatrix;
    for (const QString& g1 : groups)
        for (const QString& g2 : groups)
            sigMatrix[g1][g2] = false;

    for (const TestResult& result : tukeyResults) {
        QString testName = result.testName;
        if (testName.contains(" vs ")) {
            int vsPos = testName.indexOf(" vs ");
            QString group1 = testName.mid(11, vsPos - 11).trimmed();
            QString group2 = testName.mid(vsPos + 4).trimmed();

            bool isSignificant = result.pValue < alpha;
            if (sigMatrix.contains(group1) && sigMatrix.contains(group2)) {
                sigMatrix[group1][group2] = isSignificant;
                sigMatrix[group2][group1] = isSignificant;
            }
        }
    }

    int letterIndex = 0;
    QChar currentLetter = 'a';

    for (int i = 0; i < groups.size(); ++i) {
        QString currentGroup = groups[i];
        bool hasExistingLetter = !letterGroups[currentGroup].isEmpty();

        if (!hasExistingLetter) {
            letterGroups[currentGroup] = QString(currentLetter);
        }

        for (int j = i + 1; j < groups.size(); ++j) {
            QString otherGroup = groups[j];

            if (!sigMatrix[currentGroup][otherGroup]) {
                if (letterGroups[otherGroup].isEmpty()) {
                    letterGroups[otherGroup] = QString(currentLetter);
                } else if (!letterGroups[otherGroup].contains(currentLetter)) {
                    letterGroups[otherGroup] += currentLetter;
                }

                if (hasExistingLetter && !letterGroups[currentGroup].contains(currentLetter)) {
                    letterGroups[currentGroup] += currentLetter;
                }
            }
        }

        if (!hasExistingLetter) {
            letterIndex++;
            currentLetter = QChar('a' + letterIndex);
        }
    }

    return letterGroups;
}

//=============================================================================
// DataFrame Helper Functions
//=============================================================================

DataFrame ExpressionCalculator::mergeByPosition(const DataFrame& cqTable, const DataFrame& designTable)
{
    DataFrame result;
    QStringList cqCols = cqTable.columns();
    QStringList designCols = designTable.columns();

    QSet<QString> allCols;
    for (const QString& col : cqCols) allCols.insert(col);
    for (const QString& col : designCols) allCols.insert(col);
    allCols.remove("Position");

    QStringList finalCols = allCols.values();
    finalCols.prepend("Position");

    QMap<QString, QVector<QVariant>> columnData;
    for (const QString& col : finalCols) columnData[col] = QVector<QVariant>();

    for (int i = 0; i < cqTable.rowCount(); ++i) {
        QVariant posVar = cqTable.get(i, "Position");
        QString pos = posVar.toString();

        QMap<QString, QVariant> row;
        row["Position"] = posVar;
        for (const QString& col : cqCols) {
            if (col != "Position") row[col] = cqTable.get(i, col);
        }

        for (int j = 0; j < designTable.rowCount(); ++j) {
            if (designTable.get(j, "Position").toString() == pos) {
                for (const QString& col : designCols) {
                    if (col != "Position" && !row.contains(col)) row[col] = designTable.get(j, col);
                }
                break;
            }
        }

        for (const QString& col : finalCols) columnData[col].append(row[col]);
    }

    for (const QString& col : finalCols) result.addColumn(col, columnData[col]);
    return result;
}

QSet<QString> ExpressionCalculator::getUniqueValues(const DataFrame& df, const QString& columnName)
{
    QSet<QString> uniqueValues;
    if (!df.hasColumn(columnName)) return uniqueValues;
    for (int i = 0; i < df.rowCount(); ++i)
        uniqueValues.insert(df.get(i, columnName).toString());
    return uniqueValues;
}

QVector<double> ExpressionCalculator::getFilteredValues(
    const DataFrame& df,
    const QString& col1Name, const QString& col1Value,
    const QString& col2Name, const QString& col2Value,
    const QString& col3Name, const QString& col3Value,
    const QString& targetColName)
{
    QVector<double> result;
    for (int i = 0; i < df.rowCount(); ++i) {
        bool match = true;
        if (!col1Name.isEmpty() && df.get(i, col1Name).toString() != col1Value) match = false;
        if (!col2Name.isEmpty() && df.get(i, col2Name).toString() != col2Value) match = false;
        if (!col3Name.isEmpty() && df.get(i, col3Name).toString() != col3Value) match = false;

        if (match && df.hasColumn(targetColName)) {
            bool ok;
            double d = df.get(i, targetColName).toDouble(&ok);
            if (ok) result.append(d);
        }
    }
    return result;
}

QVector<QString> ExpressionCalculator::getFilteredValuesAsString(
    const DataFrame& df,
    const QString& col1Name, const QString& col1Value,
    const QString& col2Name, const QString& col2Value)
{
    QVector<QString> result;
    for (int i = 0; i < df.rowCount(); ++i) {
        bool match = true;
        if (!col1Name.isEmpty() && df.get(i, col1Name).toString() != col1Value) match = false;
        if (!col2Name.isEmpty() && df.get(i, col2Name).toString() != col2Value) match = false;

        if (match && df.hasColumn(col2Name)) result.append(df.get(i, col2Name).toString());
    }
    return result;
}

double ExpressionCalculator::calculateStandardDeviation(const QVector<double>& values)
{
    if (values.size() < 2) return 0.0;
    double mean = computeMean(values);
    double ss = 0.0;
    for (double val : values) ss += (val - mean) * (val - mean);
    return std::sqrt(ss / (values.size() - 1));
}

QVector<QString> ExpressionCalculator::selectReferenceGenesByGeNorm(const DataFrame& df)
{
    QSet<QString> genesSet = getUniqueValues(df, "Gene");
    QVector<QString> genes = genesSet.values();
    if (genes.size() < 2) return genes;

    QMap<QString, QMap<QString, double>> cqMatrix;
    QSet<QString> treatmentsSet = getUniqueValues(df, "Group");
    QSet<QString> biorepsSet = getUniqueValues(df, "BioRep");
    QSet<QString> techrepsSet = getUniqueValues(df, "TechRep");

    for (const QString& gene : genes) {
        for (const QString& treatment : treatmentsSet) {
            for (const QString& biorep : biorepsSet) {
                for (const QString& techrep : techrepsSet) {
                    QString key = treatment + biorep + techrep;
                    for (int i = 0; i < df.rowCount(); ++i) {
                        if (df.get(i, "Gene").toString() == gene &&
                            df.get(i, "Group").toString() == treatment &&
                            df.get(i, "BioRep").toString() == biorep &&
                            df.get(i, "TechRep").toString() == techrep) {
                            bool ok;
                            double cq = df.get(i, "Cq").toDouble(&ok);
                            if (ok) cqMatrix[gene][key] = cq;
                            break;
                        }
                    }
                }
            }
        }
    }

    QMap<QString, double> MValues;
    for (const QString& gene : genes) {
        double totalSD = 0.0;
        int comparisons = 0;

        for (const QString& otherGene : genes) {
            if (otherGene == gene) continue;
            QVector<double> log2Ratios;
            const QMap<QString, double>& geneCqs = cqMatrix[gene];
            const QMap<QString, double>& otherCqs = cqMatrix[otherGene];

            for (auto it = geneCqs.begin(); it != geneCqs.end(); ++it) {
                if (otherCqs.contains(it.key())) {
                    double ratio = it.value() / otherCqs[it.key()];
                    if (ratio > 0) log2Ratios.append(std::log2(ratio));
                }
            }

            if (log2Ratios.size() > 1) {
                double mean = computeMean(log2Ratios);
                double sumSqDiff = 0.0;
                for (double val : log2Ratios) sumSqDiff += (val - mean) * (val - mean);
                totalSD += std::sqrt(sumSqDiff / (log2Ratios.size() - 1));
                comparisons++;
            }
        }

        MValues[gene] = totalSD / comparisons;
    }

    QVector<QPair<QString, double>> geneMList;
    for (auto it = MValues.begin(); it != MValues.end(); ++it)
        geneMList.append(qMakePair(it.key(), it.value()));

    std::sort(geneMList.begin(), geneMList.end(),
        [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
            return a.second < b.second;
        });

    QVector<QString> refGenes;
    int count = qMin(2, geneMList.size());
    for (int i = 0; i < count; ++i) refGenes.append(geneMList[i].first);
    return refGenes;
}

} // namespace qpcr
