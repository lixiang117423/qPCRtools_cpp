#include "Core/ExpressionCalculator.h"
#include "Core/StatisticalTest.h"
#include "ExpressionCalculatorInternal.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

namespace qpcr {


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
