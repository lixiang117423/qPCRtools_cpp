// 基于扩增效率的表达量计算（对应 R qPCRtools::CalExpRqPCR，非 CalExpCurve 斜率截距法）。
// 从 ExpressionCalculator.cpp 按方法族拆分而来。
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

    // --- QCq per (biorep, group, gene), matching R qPCRtools::CalExpRqPCR ---
    //   mean.cq     = mean(Cq) per (biorep, group, gene)        [avg over techreps]
    //   min.mean.cq = min over groups, per (biorep, gene)
    //   QCq = Eff^(min.mean.cq - mean.cq);  SD_QCq = sd.cq * QCq * ln(Eff)
    // (Previously this used a single GLOBAL min Cq plus per-(gene,group) means
    //  averaged over bioreps — both diverged from R.)

    // Eff is a per-gene property (constant across rows); cache it.
    QMap<QString, double> effByGene;
    for (const QString& gene : genes) {
        double eff = 2.0;
        for (int i = 0; i < merged.rowCount(); ++i) {
            if (merged.get(i, "Gene").toString() == gene) {
                bool ok;
                double e = merged.get(i, "Eff").toString().toDouble(&ok);
                if (ok && e > 0) { eff = e; break; }
            }
        }
        effByGene[gene] = eff;
    }

    // Pass 1: mean.cq & sd.cq per (biorep, group, gene).
    struct RawCq { QString biorep, group, gene; double meanCq, sdCq; };
    QVector<RawCq> rawCqs;
    for (const QString& biorep : bioreps) {
        for (const QString& gene : genes) {
            for (const QString& group : groups) {
                QVector<double> cqValues = getFilteredValues(merged, "BioRep", biorep, "Gene", gene, "Group", group, "Cq");
                if (cqValues.isEmpty()) continue;
                RawCq r{biorep, group, gene, computeMean(cqValues), calculateStandardDeviation(cqValues)};
                rawCqs.append(r);
            }
        }
    }

    // Pass 2: min.mean.cq per (biorep, gene), then QCq.
    for (const RawCq& r : rawCqs) {
        double minMeanCq = std::numeric_limits<double>::max();
        for (const RawCq& r2 : rawCqs) {
            if (r2.biorep == r.biorep && r2.gene == r.gene && r2.meanCq < minMeanCq)
                minMeanCq = r2.meanCq;
        }
        double eff = effByGene.value(r.gene, 2.0);
        QCqData data;
        data.biorep = r.biorep;
        data.group = r.group;
        data.gene = r.gene;
        data.meanCq = r.meanCq;
        data.sdCq = r.sdCq;
        data.minMeanCq = minMeanCq;
        data.eff = eff;
        data.qCq = std::pow(eff, minMeanCq - r.meanCq);
        data.sdQCq = r.sdCq * data.qCq * std::log(eff);
        qcqDataList.append(data);
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

    // Normalize per gene by its MINIMUM group mean, matching R qPCRtools::CalExpRqPCR
    // (min.expression = min(mean.expression) within gene; mean.expression /= min.expression).
    // controlGroup is NOT used for expression normalization here — only for the stats
    // below — so, like R, the control group is not pinned to 1.
    //
    // R averages mean(unique(expression)) per (gene, group): bioreps whose expression
    // coincides exactly (e.g. the min group, where QCq = 1.0 exactly) are collapsed
    // before averaging. Replicate that, or genes with several pinned-to-1.0 bioreps
    // diverge. se divides by sqrt(unique-biorep count).
    auto uniqueDoubles = [](const QVector<double>& v) {
        QVector<double> out;
        for (double x : v) {
            bool seen = false;
            for (double y : out) if (y == x) { seen = true; break; }
            if (!seen) out.append(x);
        }
        return out;
    };

    QMap<QString, double> groupUniqueMean;   // key = gene + "_" + group
    QMap<QString, double> groupUniqueSd;
    QMap<QString, int> groupBiorepCount;
    for (const QString& gene : genes) {
        if (refGenes.contains(gene)) continue;
        for (const QString& group : groups) {
            QString key = gene + "_" + group;
            if (!groupExpressions.contains(key)) continue;
            QVector<double> uniq = uniqueDoubles(groupExpressions[key]);
            groupUniqueMean[key] = computeMean(uniq);
            groupUniqueSd[key]   = calculateStandardDeviation(uniq);
            groupBiorepCount[key] = groupExpressions[key].size();
        }
    }

    QMap<QString, ExpressionData> finalResults;
    for (const QString& gene : genes) {
        if (refGenes.contains(gene)) continue;

        double minMean = -1.0;
        for (const QString& group : groups) {
            QString key = gene + "_" + group;
            if (groupUniqueMean.contains(key)) {
                double m = groupUniqueMean[key];
                if (minMean < 0 || m < minMean) minMean = m;
            }
        }

        for (const QString& group : groups) {
            QString key = gene + "_" + group;
            if (!groupUniqueMean.contains(key)) continue;

            double mean = groupUniqueMean[key];
            double sd   = groupUniqueSd[key];
            double se   = sd / std::sqrt(static_cast<double>(groupBiorepCount[key]));

            ExpressionData expData;
            expData.group = group;
            expData.gene = gene;

            if (minMean > 0) {
                expData.meanExpression = mean / minMean;
                expData.sdExpression = sd / minMean;
                expData.seExpression = se / minMean;
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
                    // 命名约定与 runPairwiseTests 一致：group1=对照组, group2=处理组。
                    // （旧实现把 group2 写成 controlGroup，导致最后的 PValue 合并循环
                    //   永远匹配不上，结果表 PValue 列恒为空。）
                    if (statMethod == "wilcox.test") {
                        result.statistics.append(performWilcoxonTest(controlData, treatData, gene, params.controlGroup, group));
                    } else {
                        result.statistics.append(performTTest(controlData, treatData, gene, params.controlGroup, group));
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
