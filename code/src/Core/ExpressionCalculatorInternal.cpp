#include "ExpressionCalculatorInternal.h"

#include "Core/ExpressionCalculator.h"  // ExpressionCalculator::performTTest 等
#include "Core/StatisticalTest.h"       // StatisticalTest::anova / tukeyHSD

#include <algorithm>
#include <cmath>
#include <numeric>

namespace qpcr {

QList<QString> sortedUnique(const QVector<QString>& values) {
    QSet<QString> set;
    for (const auto& v : values) set.insert(v);
    QList<QString> result = set.values();
    std::sort(result.begin(), result.end());
    return result;
}

QList<QString> sortedUniqueExclude(const QVector<QString>& values, const QString& exclude) {
    QSet<QString> set;
    for (const auto& v : values) {
        if (v != exclude) set.insert(v);
    }
    QList<QString> result = set.values();
    std::sort(result.begin(), result.end());
    return result;
}

double computeMean(const QVector<double>& v) {
    if (v.isEmpty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double computeStdDev(const QVector<double>& v) {
    if (v.size() < 2) return 0.0;
    double mean = computeMean(v);
    double ss = 0.0;
    for (double val : v) ss += (val - mean) * (val - mean);
    return std::sqrt(ss / (v.size() - 1));
}

// Run pairwise statistical tests for each gene, comparing refGroup to all others
void runPairwiseTests(
    const QHash<QString, QHash<QString, QVector<double>>>& allData,
    const QList<QString>& sortedGenes,
    const QList<QString>& sortedGroups,
    const QString& refGroup,
    const QString& statMethod,
    QVector<StatisticalResult>& statistics)
{
    for (const QString& gene : sortedGenes) {
        QVector<double> refValues = allData.value(refGroup).value(gene);

        for (const QString& testGroup : sortedGroups) {
            if (testGroup == refGroup) continue;

            QVector<double> testValues = allData.value(testGroup).value(gene);
            if (refValues.isEmpty() || testValues.isEmpty()) continue;

            StatisticalResult statResult;
            if (statMethod == "wilcox.test") {
                statResult = ExpressionCalculator::performWilcoxonTest(
                    refValues, testValues, gene, refGroup, testGroup);
            } else {
                statResult = ExpressionCalculator::performTTest(
                    refValues, testValues, gene, refGroup, testGroup);
            }
            statistics.append(statResult);
        }
    }
}

// Run ANOVA + Tukey HSD for each gene
void runAnovaTests(
    const QHash<QString, QHash<QString, QVector<double>>>& allData,
    const QList<QString>& sortedGenes,
    const QList<QString>& sortedGroups,
    QVector<StatisticalResult>& statistics)
{
    for (const QString& gene : sortedGenes) {
        QVector<QVector<double>> groupDataList;
        QStringList groupNamesList;
        QHash<QString, double> geneGroupMeans;

        for (const QString& grp : sortedGroups) {
            QVector<double> values = allData.value(grp).value(gene);
            if (!values.isEmpty()) {
                groupDataList.append(values);
                groupNamesList.append(grp);
                geneGroupMeans[grp] = computeMean(values);
            }
        }

        if (groupDataList.size() < 2) continue;

        TestResult anovaResult = StatisticalTest::anova(groupDataList, 0.05);
        QVector<TestResult> tukeyResults = StatisticalTest::tukeyHSD(groupDataList, groupNamesList, 0.05);
        QHash<QString, QString> letterGroups = ExpressionCalculator::generateLetterGroups(geneGroupMeans, tukeyResults, 0.05);

        for (const QString& grp : sortedGroups) {
            if (!allData.contains(grp) || !allData[grp].contains(gene)) continue;
            StatisticalResult stat;
            stat.gene = gene;
            stat.group = grp;
            stat.group1 = "All";
            stat.group2 = grp;
            stat.pValue = anovaResult.pValue;
            stat.fStatistic = anovaResult.statistic;
            stat.significance = letterGroups.value(grp, "");
            stat.letterGroup = letterGroups.value(grp, "");
            statistics.append(stat);
        }
    }
}

// Build a result DataFrame with p-values merged from statistics
DataFrame buildResultTableWithPValues(
    const QVector<QVariant>& genes,
    const QVector<QVariant>& groups,
    const QVector<QVariant>& means,
    const QVector<QVariant>& stdDevs,
    const QVector<StatisticalResult>& statistics)
{
    QVector<QVariant> pValues, signifs;
    for (int i = 0; i < genes.size(); ++i) {
        QString group = groups[i].toString();
        QString gene = genes[i].toString();
        bool found = false;
        for (const auto& stat : statistics) {
            if (stat.gene == gene && stat.group2 == group) {
                pValues.append(stat.pValue);
                signifs.append(stat.significance);
                found = true;
                break;
            }
        }
        if (!found) {
            pValues.append(QVariant());
            signifs.append("");
        }
    }

    DataFrame table;
    table.addColumn("Gene", genes);
    table.addColumn("Group", groups);
    table.addColumn("Mean", means);
    table.addColumn("StdDev", stdDevs);
    table.addColumn("PValue", pValues);
    table.addColumn("Significance", signifs);
    return table;
}

}  // namespace qpcr
