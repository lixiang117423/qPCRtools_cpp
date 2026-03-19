#ifndef EXPRESSIONCALCULATOR_H
#define EXPRESSIONCALCULATOR_H

#include "Data/DataFrame.h"
#include "Core/StandardCurve.h"
#include "Core/StatisticalTest.h"
#include <QString>
#include <QVector>

namespace qpcr {

/**
 * @brief 统计检验结果
 */
struct StatisticalResult {
    QString gene;              // 基因名称
    QString group;             // 组别
    QString group1;            // 比较组1
    QString group2;            // 比较组2
    double pValue;             // P值
    QString significance;      // 显著性标记: "***", "**", "*", "NS" 或字母标记 "a", "b", "ab", etc.
    QString letterGroup;       // ANOVA字母标记分组 (仅ANOVA使用)

    // 统计量
    double tStatistic;         // t值 (t检验)
    double wilcoxV;            // V值 (Wilcoxon)
    double fStatistic;         // F值 (ANOVA)
};

/**
 * @brief 表达量计算结果
 */
struct ExpressionResult {
    DataFrame table;           // 完整结果表
    QVector<StatisticalResult> statistics;  // 统计检验结果
    QString method;            // 使用的方法
};

/**
 * @brief ΔCt 计算参数
 */
struct DeltaCtParams {
    DataFrame cqTable;         // Cq 值表
    DataFrame designTable;     // 设计表
    QString referenceGene;     // 内参基因
    QString controlGroup;      // 对照组
};

/**
 * @brief ΔΔCt 计算参数
 */
struct DeltaDeltaCtParams {
    DataFrame cqTable;         // Cq 值表
    DataFrame designTable;     // 设计表
    QString referenceGene;     // 内参基因
    QString controlGroup;      // 对照组
    bool removeOutliers;       // 是否移除异常值
};

/**
 * @brief 标准曲线计算参数
 */
struct StandardCurveParams {
    DataFrame cqTable;         // Cq 值表
    DataFrame curveTable;      // 标准曲线表
    DataFrame designTable;     // 设计表
    QString referenceGene;     // 内参基因
    QString controlGroup;      // 对照组
};

/**
 * @brief 表达量计算器
 *
 * 提供 ΔCt、ΔΔCt 和基于标准曲线的表达量计算
 * 对应 R 函数: CalExp2dCt(), CalExp2ddCt(), CalExpCurve()
 */
class ExpressionCalculator
{
public:
    /**
     * @brief 使用 2^-ΔCt 方法计算表达量
     *
     * ΔCt = Ct(target) - Ct(reference)
     * Relative Expression = 2^-ΔCt
     *
     * @param params 计算参数
     * @param statMethod 统计方法: "t.test", "wilcox.test", "anova"
     * @return 表达量结果
     */
    static ExpressionResult calculateByDeltaCt(
        const DeltaCtParams& params,
        const QString& statMethod = "t.test"
    );

    /**
     * @brief 使用 2^-ΔΔCt 方法计算表达量
     *
     * ΔCt = Ct(target) - Ct(reference)
     * ΔΔCt = ΔCt(treated) - ΔCt(control)
     * Relative Expression = 2^-ΔΔCt
     *
     * @param params 计算参数
     * @param statMethod 统计方法: "t.test", "wilcox.test", "anova"
     * @return 表达量结果
     */
    static ExpressionResult calculateByDeltaDeltaCt(
        const DeltaDeltaCtParams& params,
        const QString& statMethod = "t.test"
    );

    /**
     * @brief 使用标准曲线计算表达量
     *
     * @param params 计算参数
     * @param statMethod 统计方法
     * @return 表达量结果
     */
    static ExpressionResult calculateByStandardCurve(
        const StandardCurveParams& params,
        const QString& statMethod = "t.test"
    );

    /**
     * @brief 计算单个样本的 ΔCt
     * @param targetCq 目标基因 Cq 值
     * @param refCq 内参基因 Cq 值
     * @return ΔCt 值
     */
    static double calculateDeltaCt(double targetCq, double refCq);

    /**
     * @brief 计算相对表达量 (2^-ΔCt)
     * @param deltaCt ΔCt 值
     * @return 相对表达量
     */
    static double calculateExpressionFromDeltaCt(double deltaCt);

    /**
     * @brief 计算相对表达量 (2^-ΔΔCt)
     * @param targetDeltaCt 处理组 ΔCt
     * @param controlDeltaCt 对照组 ΔCt
     * @return 相对表达量
     */
    static double calculateExpressionFromDeltaDeltaCt(
        double targetDeltaCt,
        double controlDeltaCt
    );

    /**
     * @brief 检测并移除异常值 (IQR 方法)
     * @param values 数值向量
     * @return 无异常值的向量
     */
    static QVector<double> removeOutliers(const QVector<double>& values);

    /**
     * @brief 格式化显著性标记
     * @param pValue P值
     * @return 显著性标记: "***", "**", "*", "NS"
     */
    static QString formatSignificance(double pValue);

private:
    /**
     * @brief 执行 t检验
     */
    static StatisticalResult performTTest(
        const QVector<double>& group1,
        const QVector<double>& group2,
        const QString& gene,
        const QString& group1Name,
        const QString& group2Name
    );

    /**
     * @brief 执行 Wilcoxon 检验
     */
    static StatisticalResult performWilcoxonTest(
        const QVector<double>& group1,
        const QVector<double>& group2,
        const QString& gene,
        const QString& group1Name,
        const QString& group2Name
    );

    /**
     * @brief 执行 ANOVA + Tukey HSD
     */
    static QVector<StatisticalResult> performANOVA(
        const DataFrame& data,
        const QString& geneCol,
        const QString& groupCol,
        const QString& valueCol
    );

    /**
     * @brief 生成ANOVA字母标记
     */
    static QHash<QString, QString> generateLetterGroups(
        const QHash<QString, double>& groupMeans,
        const QVector<TestResult>& tukeyResults,
        double alpha = 0.05
    );
};

} // namespace qpcr

#endif // EXPRESSIONCALCULATOR_H
