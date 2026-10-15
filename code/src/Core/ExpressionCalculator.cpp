#include "Core/ExpressionCalculator.h"
#include "Core/StatisticalTest.h"
#include "ExpressionCalculatorInternal.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

namespace qpcr {

namespace {

// R 默认 (type 7) 分位数：线性插值。用于 IQR 异常值检测，与 R stats::quantile 一致。
double quantileType7(const QVector<double>& sorted, double p)
{
    const int n = sorted.size();
    if (n == 0) return qQNaN();
    const double h = (n - 1) * p;
    const int lo = static_cast<int>(std::floor(h));
    const int hi = static_cast<int>(std::ceil(h));
    if (lo == hi) return sorted[lo];
    const double frac = h - lo;
    return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
}

} // namespace


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
    // 与 R qPCRtools::find_outlier 一致：Q1/Q3 用 type-7 分位数（线性插值），
    // 落在 [Q1 - 1.5*IQR, Q3 + 1.5*IQR] 之外的值被移除。
    if (values.size() < 4) return values;

    QVector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());

    const double q1 = quantileType7(sorted, 0.25);
    const double q3 = quantileType7(sorted, 0.75);
    const double iqr = q3 - q1;

    const double lowerBound = q1 - 1.5 * iqr;
    const double upperBound = q3 + 1.5 * iqr;

    QVector<double> filtered;
    filtered.reserve(values.size());
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
    // 委托给 StatisticalTest（Welch t 检验，与 R t.test 默认一致）。
    // 旧实现用 z 阈值阶梯近似 p 值且乘 2，结果错误。
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    TestResult tr = StatisticalTest::tTest(group1, group2, /*equalVariance=*/false);
    result.tStatistic = tr.statistic;
    result.pValue = tr.pValue;
    result.significance = formatSignificance(tr.pValue);
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

    // wilcoxV = group1 的秩和（R wilcox.test 的 W 统计量）
    QVector<double> combined = group1 + group2;
    const int n1 = group1.size();
    if (n1 == 0 || group2.isEmpty()) {
        result.pValue = qQNaN();
        result.significance = formatSignificance(result.pValue);
        return result;
    }

    QVector<std::pair<double, int>> indexed;
    indexed.reserve(combined.size());
    for (int i = 0; i < combined.size(); ++i) indexed.append({combined[i], i});
    std::sort(indexed.begin(), indexed.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    QVector<double> ranks(combined.size());
    int i = 0;
    while (i < indexed.size()) {
        int j = i;
        while (j + 1 < indexed.size() && indexed[j + 1].first == indexed[i].first) ++j;
        const double avgRank = (i + j) / 2.0 + 1.0;
        for (int k = i; k <= j; ++k) ranks[indexed[k].second] = avgRank;
        i = j + 1;
    }

    double sumRanks1 = 0.0;
    for (int k = 0; k < n1; ++k) sumRanks1 += ranks[k];
    result.wilcoxV = sumRanks1;

    // p 值委托给 StatisticalTest（平均秩 + R 一致的并列方差校正）
    TestResult tr = StatisticalTest::wilcoxonTest(group1, group2);
    result.pValue = tr.pValue;
    result.significance = formatSignificance(tr.pValue);
    return result;
}

QVector<StatisticalResult> ExpressionCalculator::performANOVA(
    const DataFrame& data, const QString& geneCol, const QString& groupCol, const QString& valueCol)
{
    // 与 runAnovaTests 相同的输出结构：每个 (gene, group) 一条结果，
    // pValue 为 ANOVA 整体 p 值，significance/letterGroup 为 Tukey 字母标记。
    // 旧实现 pValue 硬编码 0.05。
    QVector<StatisticalResult> results;

    const auto genes = data.getStringColumn(geneCol);
    const auto groups = data.getStringColumn(groupCol);
    QSet<QString> geneSet(genes.begin(), genes.end());
    QSet<QString> groupSet(groups.begin(), groups.end());

    for (const QString& gene : geneSet) {
        QVector<QVector<double>> groupDataList;
        QStringList groupNames;
        QHash<QString, double> geneGroupMeans;

        for (const QString& group : groupSet) {
            QVector<double> vals;
            for (int i = 0; i < data.rowCount(); ++i) {
                if (data.get(i, geneCol).toString() == gene &&
                    data.get(i, groupCol).toString() == group) {
                    bool ok = false;
                    double v = data.get(i, valueCol).toDouble(&ok);
                    if (ok) vals.append(v);
                }
            }
            if (!vals.isEmpty()) {
                groupDataList.append(vals);
                groupNames.append(group);
                geneGroupMeans[group] = computeMean(vals);
            }
        }

        if (groupDataList.size() < 2) continue;

        TestResult anovaResult = StatisticalTest::anova(groupDataList, 0.05);
        QVector<TestResult> tukeyResults =
            StatisticalTest::tukeyHSD(groupDataList, groupNames, 0.05);
        QHash<QString, QString> letterGroups =
            generateLetterGroups(geneGroupMeans, tukeyResults, 0.05);

        for (const QString& group : groupNames) {
            StatisticalResult stat;
            stat.gene = gene;
            stat.group = group;
            stat.group1 = "All";
            stat.group2 = group;
            stat.pValue = anovaResult.pValue;
            stat.fStatistic = anovaResult.statistic;
            stat.significance = letterGroups.value(group, "");
            stat.letterGroup = letterGroups.value(group, "");
            results.append(stat);
        }
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

    // 显著性矩阵：默认「显著」（只有 tukeyResults 里 p >= alpha 的对比标记为不显著）
    QHash<QString, QHash<QString, bool>> sigMatrix;
    for (const QString& g1 : groups)
        for (const QString& g2 : groups)
            sigMatrix[g1][g2] = (g1 == g2);

    for (const TestResult& result : tukeyResults) {
        QString group1 = result.group1Name;
        QString group2 = result.group2Name;

        // 兼容未携带组名字段的旧式调用（字符串解析仅在字段缺失时兜底）
        if (group1.isEmpty() || group2.isEmpty()) {
            const QString testName = result.testName;
            const int vsPos = testName.indexOf(" vs ");
            if (vsPos < 0) continue;
            const QString prefix = QStringLiteral("Tukey HSD: ");
            group1 = testName.mid(prefix.size(), vsPos - prefix.size()).trimmed();
            group2 = testName.mid(vsPos + 4).trimmed();
        }

        if (sigMatrix.contains(group1) && sigMatrix.contains(group2)) {
            const bool isSignificant = result.pValue < alpha;
            sigMatrix[group1][group2] = isSignificant;
            sigMatrix[group2][group1] = isSignificant;
        }
    }

    // 字母分配（Piepho 2004 风格）：
    // 按均值降序遍历；空字母的组发一个新字母；然后把该组的每个字母插入所有与它
    // 不显著的组，前提是目标组与持有该字母的所有组都不显著（否则会造成两个显著
    // 不同的组共享字母）。旧实现只在 j>i 方向用「当前字母」传播，且新字母计数在
    // 组已有字母时不会递增，导致非传递性比较下出现错误字母（如显著差异组共享字母）。
    QStringList groupLetters(groups.size());
    int nextLetter = 0;

    for (int i = 0; i < groups.size(); ++i) {
        if (groupLetters[i].isEmpty()) {
            groupLetters[i] = QChar('a' + nextLetter);
            ++nextLetter;
        }

        for (int j = 0; j < groups.size(); ++j) {
            if (j == i || sigMatrix[groups[i]][groups[j]]) continue;

            for (const QChar c : groupLetters[i]) {
                if (groupLetters[j].contains(c)) continue;

                bool conflict = false;
                for (int k = 0; k < groups.size() && !conflict; ++k) {
                    if (k != j && groupLetters[k].contains(c) && sigMatrix[groups[j]][groups[k]]) {
                        conflict = true;
                    }
                }
                if (!conflict) groupLetters[j].append(c);
            }
        }
    }

    for (int i = 0; i < groups.size(); ++i) {
        std::sort(groupLetters[i].begin(), groupLetters[i].end());
        letterGroups[groups[i]] = groupLetters[i];
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
    if (!df.hasColumn(col2Name)) return result;  // 提前返回，避免逐行重复检查

    for (int i = 0; i < df.rowCount(); ++i) {
        bool match = true;
        if (!col1Name.isEmpty() && df.get(i, col1Name).toString() != col1Value) match = false;
        if (!col2Name.isEmpty() && df.get(i, col2Name).toString() != col2Value) match = false;

        if (match) result.append(df.get(i, col2Name).toString());
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
