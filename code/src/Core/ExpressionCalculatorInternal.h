// ExpressionCalculator 的内部共享辅助函数（实现细节，非公开 API）。
// 仅被 ExpressionCalculator*.cpp 包含。置于 namespace qpcr，
// 故各 calculate* 方法中的非限定调用（computeMean 等）无需修改即可找到。
#pragma once

#include "Core/ExpressionCalculator.h"  // StatisticalResult, DataFrame, TestResult

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

namespace qpcr {

QList<QString> sortedUnique(const QVector<QString>& values);
QList<QString> sortedUniqueExclude(const QVector<QString>& values, const QString& exclude);
double computeMean(const QVector<double>& v);
double computeStdDev(const QVector<double>& v);

// 对每个基因，将 refGroup 与其余各组做两两统计检验。
void runPairwiseTests(
    const QHash<QString, QHash<QString, QVector<double>>>& allData,
    const QList<QString>& sortedGenes,
    const QList<QString>& sortedGroups,
    const QString& refGroup,
    const QString& statMethod,
    QVector<StatisticalResult>& statistics);

// 对每个基因做 ANOVA + Tukey HSD。
void runAnovaTests(
    const QHash<QString, QHash<QString, QVector<double>>>& allData,
    const QList<QString>& sortedGenes,
    const QList<QString>& sortedGroups,
    QVector<StatisticalResult>& statistics);

// 合并均值/SD 与 statistics 中的 p 值，构造结果 DataFrame。
DataFrame buildResultTableWithPValues(
    const QVector<QVariant>& genes,
    const QVector<QVariant>& groups,
    const QVector<QVariant>& means,
    const QVector<QVariant>& stdDevs,
    const QVector<StatisticalResult>& statistics);

}  // namespace qpcr
