#include "Core/ExpressionCalculator.h"
#include "Core/StatisticalTest.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

namespace qpcr {

//=============================================================================
// ΔCt Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByDeltaCt(const DeltaCtParams& params)
{
    ExpressionResult result;
    result.method = "2^-ΔCt";

    // Merge tables
    DataFrame merged = params.cqTable.join(params.designTable, "Position");

    // Check required columns
    if (!merged.hasColumn("Gene") || !merged.hasColumn("Cq") ||
        !merged.hasColumn("Group") || !merged.hasColumn("BioRep")) {
        qWarning() << "Missing required columns";
        return result;
    }

    // Get target genes (exclude reference)
    auto allGenes = merged.getStringColumn("Gene");
    QSet<QString> geneSet;
    for (const auto& gene : allGenes) {
        if (gene != params.referenceGene) {
            geneSet.insert(gene);
        }
    }

    // Process each target gene
    for (const QString& gene : geneSet) {
        // Get data for this gene and reference gene
        DataFrame geneData = merged.filter([gene](const Row& row) {
            return row.value("Gene").toString() == gene ||
                   row.value("Gene").toString() == params.referenceGene;
        });

        // Group by BioRep and calculate mean Cq for technical replicates
        // ... implementation details
    }

    return result;
}

//=============================================================================
// ΔΔCt Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByDeltaDeltaCt(
    const DeltaDeltaCtParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "2^-ΔΔCt";

    // Merge tables
    DataFrame merged = params.cqTable.join(params.designTable, "Position");

    // Check required columns
    if (!merged.hasColumn("Gene") || !merged.hasColumn("Cq") ||
        !merged.hasColumn("Group") || !merged.hasColumn("BioRep")) {
        qWarning() << "Missing required columns";
        return result;
    }

    // Get target genes (exclude reference)
    auto allGenes = merged.getStringColumn("Gene");
    QSet<QString> geneSet;
    for (const auto& gene : allGenes) {
        if (gene != params.referenceGene) {
            geneSet.insert(gene);
        }
    }

    // Get all groups
    auto allGroups = merged.getStringColumn("Group");
    QSet<QString> groupSet;
    for (const auto& group : allGroups) {
        groupSet.insert(group);
    }

    // Build result table structure
    QVector<QVariant> groups, genes, bioreps, expressions, signifs;
    QVector<double> expressionValues;

    // Calculate mean ΔCt for control group
    QHash<QString, double> controlDeltaCtByGene;

    for (const QString& gene : geneSet) {
        // Filter for control group
        DataFrame controlData = merged.filter(
            [params, gene](const Row& row) {
                return row.value("Group").toString() == params.controlGroup &&
                       (row.value("Gene").toString() == gene ||
                        row.value("Gene").toString() == params.referenceGene);
            });

        // Get mean Cq values
        QVector<double> targetCqs, refCqs;

        for (int i = 0; i < controlData.rowCount(); ++i) {
            QString g = controlData.get(i, "Gene").toString();
            double cq = controlData.get(i, "Cq").toDouble();

            if (g == gene) {
                targetCqs.append(cq);
            } else if (g == params.referenceGene) {
                refCqs.append(cq);
            }
        }

        if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
            double meanTarget = std::accumulate(targetCqs.begin(), targetCqs.end(), 0.0) / targetCqs.size();
            double meanRef = std::accumulate(refCqs.begin(), refCqs.end(), 0.0) / refCqs.size();
            controlDeltaCtByGene[gene] = meanTarget - meanRef;
        }
    }

    // Process each group
    for (const QString& group : groupSet) {
        for (const QString& gene : geneSet) {
            // Filter for this group
            DataFrame groupData = merged.filter(
                [group, gene, params](const Row& row) {
                    return row.value("Group").toString() == group &&
                           (row.value("Gene").toString() == gene ||
                            row.value("Gene").toString() == params.referenceGene);
                });

            // Group by BioRep
            auto bioReps = groupData.getStringColumn("BioRep");
            QSet<QString> bioRepSet;
            for (const auto& rep : bioReps) {
                bioRepSet.insert(rep);
            }

            // Calculate expression for each biological replicate
            QHash<QString, double> repExpression;

            for (const QString& bioRep : bioRepSet) {
                DataFrame repData = groupData.filter(
                    [bioRep](const Row& row) {
                        return row.value("BioRep").toString() == bioRep;
                    });

                QVector<double> targetCqs, refCqs;

                for (int i = 0; i < repData.rowCount(); ++i) {
                    QString g = repData.get(i, "Gene").toString();
                    double cq = repData.get(i, "Cq").toDouble();

                    if (g == gene) {
                        targetCqs.append(cq);
                    } else if (g == params.referenceGene) {
                        refCqs.append(cq);
                    }
                }

                if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
                    // Calculate mean Cq for technical replicates
                    double meanTarget = std::accumulate(targetCqs.begin(), targetCqs.end(), 0.0) / targetCqs.size();
                    double meanRef = std::accumulate(refCqs.begin(), refCqs.end(), 0.0) / refCqs.size();

                    double deltaCt = meanTarget - meanRef;
                    double deltaDeltaCt = deltaCt - controlDeltaCtByGene.value(gene, 0.0);

                    double expression = calculateExpressionFromDeltaDeltaCt(deltaDeltaCt, 0.0);
                    repExpression[bioRep] = expression;
                    expressionValues.append(expression);
                }
            }

            // Remove outliers if requested
            QVector<double> finalExpressions;
            for (auto it = repExpression.begin(); it != repExpression.end(); ++it) {
                groups.append(group);
                genes.append(gene);
                bioreps.append(it.key());
                expressions.append(it.value());
                finalExpressions.append(it.value());
            }

            if (params.removeOutliers) {
                finalExpressions = removeOutliers(finalExpressions);
            }
        }
    }

    // Build result table
    DataFrame resultTable;
    resultTable.addColumn("Group", groups);
    resultTable.addColumn("Gene", genes);
    resultTable.addColumn("BioRep", bioreps);
    resultTable.addColumn("Expression", expressions);

    // Perform statistical tests
    if (statMethod == "t.test") {
        // t-test for each gene comparing to control
        for (const QString& gene : geneSet) {
            QVector<double> controlValues, treatedValues;

            for (int i = 0; i < resultTable.rowCount(); ++i) {
                if (resultTable.get(i, "Gene").toString() == gene) {
                    double expr = resultTable.get(i, "Expression").toDouble();
                    QString group = resultTable.get(i, "Group").toString();

                    if (group == params.controlGroup) {
                        controlValues.append(expr);
                    } else {
                        treatedValues.append(expr);
                    }
                }
            }

            if (!controlValues.isEmpty() && !treatedValues.isEmpty()) {
                StatisticalResult stat = performTTest(
                    controlValues,
                    treatedValues,
                    gene,
                    params.controlGroup,
                    "Treated"
                );
                result.statistics.append(stat);
            }
        }
    }

    result.table = resultTable;
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
    result.method = "Standard Curve";

    // Implementation similar to ΔΔCt but using standard curve
    // to calculate absolute quantities

    Q_UNUSED(statMethod);

    return result;
}

//=============================================================================
// Helper Functions
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
    double targetDeltaCt,
    double controlDeltaCt)
{
    double deltaDeltaCt = targetDeltaCt - controlDeltaCt;
    return std::pow(2.0, -deltaDeltaCt);
}

QVector<double> ExpressionCalculator::removeOutliers(const QVector<double>& values)
{
    if (values.size() < 4) {
        return values;
    }

    // Calculate IQR
    QVector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());

    int n = sorted.size();
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;

    double lowerBound = q1 - 1.5 * iqr;
    double upperBound = q3 + 1.5 * iqr;

    // Filter outliers
    QVector<double> filtered;
    for (double val : values) {
        if (val >= lowerBound && val <= upperBound) {
            filtered.append(val);
        }
    }

    return filtered;
}

QString ExpressionCalculator::formatSignificance(double pValue)
{
    if (std::isnan(pValue)) {
        return "NA";
    } else if (pValue < 0.001) {
        return "***";
    } else if (pValue < 0.01) {
        return "**";
    } else if (pValue < 0.05) {
        return "*";
    } else {
        return "NS";
    }
}

//=============================================================================
// Statistical Tests
//=============================================================================

StatisticalResult ExpressionCalculator::performTTest(
    const QVector<double>& group1,
    const QVector<double>& group2,
    const QString& gene,
    const QString& group1Name,
    const QString& group2Name)
{
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    // Calculate means
    double mean1 = std::accumulate(group1.begin(), group1.end(), 0.0) / group1.size();
    double mean2 = std::accumulate(group2.begin(), group2.end(), 0.0) / group2.size();

    // Calculate standard deviations
    double var1 = 0, var2 = 0;
    for (double val : group1) {
        var1 += (val - mean1) * (val - mean1);
    }
    for (double val : group2) {
        var2 += (val - mean2) * (val - mean2);
    }
    var1 /= (group1.size() - 1);
    var2 /= (group2.size() - 1);

    // Pooled standard error
    double se = std::sqrt(var1 / group1.size() + var2 / group2.size());

    // t-statistic
    if (se > 0) {
        result.tStatistic = (mean1 - mean2) / se;

        // Approximate p-value (two-tailed)
        double absT = std::abs(result.tStatistic);
        if (absT < 1.96) {
            result.pValue = 0.05;
        } else if (absT < 2.58) {
            result.pValue = 0.01;
        } else if (absT < 3.29) {
            result.pValue = 0.001;
        } else {
            result.pValue = 0.0001;
        }
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
    const QVector<double>& group1,
    const QVector<double>& group2,
    const QString& gene,
    const QString& group1Name,
    const QString& group2Name)
{
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    // Simplified Wilcoxon rank-sum test
    // Combine and rank all values
    QVector<double> combined = group1 + group2;
    QVector<int> ranks;

    for (size_t i = 0; i < combined.size(); ++i) {
        int rank = 1;
        for (size_t j = 0; j < combined.size(); ++j) {
            if (combined[j] < combined[i] ||
                (combined[j] == combined[i] && j < i)) {
                rank++;
            }
        }
        ranks.append(rank);
    }

    // Sum of ranks for group1
    double sumRanks1 = 0;
    for (int i = 0; i < group1.size(); ++i) {
        sumRanks1 += ranks[i];
    }

    result.wilcoxV = sumRanks1;

    // Approximate p-value (simplified)
    // For accurate results, use statistical library
    result.pValue = 0.05; // Placeholder
    result.significance = formatSignificance(result.pValue);

    return result;
}

QVector<StatisticalResult> ExpressionCalculator::performANOVA(
    const DataFrame& data,
    const QString& geneCol,
    const QString& groupCol,
    const QString& valueCol)
{
    QVector<StatisticalResult> results;

    // Get unique genes and groups
    auto genes = data.getStringColumn(geneCol);
    auto groups = data.getStringColumn(groupCol);

    QSet<QString> geneSet, groupSet;
    for (const auto& g : genes) geneSet.insert(g);
    for (const auto& g : groups) groupSet.insert(g);

    // For each gene, perform ANOVA
    for (const QString& gene : geneSet) {
        // Get data for this gene
        DataFrame geneData = data.filter(
            [gene, geneCol](const Row& row) {
                return row.value(geneCol).toString() == gene;
            });

        // Calculate overall mean
        auto values = geneData.getNumericColumn(valueCol);
        double overallMean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();

        // Calculate between-group and within-group variance
        double ssBetween = 0, ssWithin = 0;
        int nTotal = values.size();
        int nGroups = groupSet.size();

        for (const QString& group : groupSet) {
            DataFrame groupData = geneData.filter(
                [group, groupCol](const Row& row) {
                    return row.value(groupCol).toString() == group;
                });

            auto groupValues = groupData.getNumericColumn(valueCol);
            double groupMean = std::accumulate(groupValues.begin(), groupValues.end(), 0.0) / groupValues.size();

            ssBetween += groupValues.size() * (groupMean - overallMean) * (groupMean - overallMean);

            for (double val : groupValues) {
                ssWithin += (val - groupMean) * (val - groupMean);
            }
        }

        // F-statistic
        double dfBetween = nGroups - 1;
        double dfWithin = nTotal - nGroups;

        double msBetween = ssBetween / dfBetween;
        double msWithin = ssWithin / dfWithin;

        double fStat = msWithin > 0 ? msBetween / msWithin : qQNaN();

        // Approximate p-value
        double pValue = 0.05; // Placeholder - use F-distribution for accuracy

        StatisticalResult result;
        result.gene = gene;
        result.fStatistic = fStat;
        result.pValue = pValue;
        result.significance = formatSignificance(pValue);

        results.append(result);
    }

    return results;
}

} // namespace qpcr
