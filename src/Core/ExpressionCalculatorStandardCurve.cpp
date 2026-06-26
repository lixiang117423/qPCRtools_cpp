// 基于标准曲线的表达量计算（对应 R qPCRtools::CalExpCurve）。
// 从 ExpressionCalculator.cpp 按方法族拆分而来，行为不变。
#include "Core/ExpressionCalculator.h"
#include "ExpressionCalculatorInternal.h"

#include <QMap>
#include <QSet>
#include <QVariant>
#include <cmath>
#include <limits>

namespace qpcr {

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

}  // namespace qpcr
