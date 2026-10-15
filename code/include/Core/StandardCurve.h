#ifndef STANDARDCURVE_H
#define STANDARDCURVE_H

#include "Data/DataFrame.h"
#include <QVector>
#include <QString>

namespace qpcr {

/**
 * @brief 标准曲线计算结果
 */
struct StandardCurveResult {
    QString gene;              // 基因名称
    double slope;              // 斜率
    double intercept;          // 截距
    double rSquared;           // R² (决定系数)
    double pValue;             // P值
    double efficiency;         // 扩增效率
    double maxCq;              // 最大 Cq 值
    double minCq;              // 最小 Cq 值
    QString formula;           // 回归公式

    // 原始数据
    QVector<double> logConcentrations;  // 对数浓度
    QVector<double> cqValues;            // Cq 值
    QVector<double> predictedCq;         // 预测 Cq 值
};

/**
 * @brief 标准曲线计算器
 *
 * 计算标准曲线和扩增效率
 * 对应 R 函数: CalCurve()
 */
class StandardCurve
{
public:
    /**
     * @brief 计算标准曲线
     *
     * @param cqTable Cq 值表格，必须包含列: Position, Cq
     * @param concenTable 浓度表格，必须包含列: Position, Gene, Conc
     * @param lowestConcen 用于计算标准曲线的最低浓度
     * @param highestConcen 用于计算标准曲线的最高浓度
     * @param dilution 稀释倍数，默认 4
     * @param byMean 是否使用平均 Cq 值计算，默认 true
     * @return 标准曲线结果列表 (每个基因一个结果)
     */
    static QVector<StandardCurveResult> calculate(
        const DataFrame& cqTable,
        const DataFrame& concenTable,
        double lowestConcen,
        double highestConcen,
        double dilution = 4.0,
        bool byMean = true
    );

    /**
     * @brief 计算单个基因的标准曲线
     *
     * @param cqValues Cq 值向量
     * @param concentrations 浓度向量
     * @param geneName 基因名称
     * @param dilution 稀释倍数
     * @return 标准曲线结果
     */
    static StandardCurveResult calculateSingle(
        const QVector<double>& cqValues,
        const QVector<double>& concentrations,
        const QString& geneName,
        double dilution = 4.0
    );

    /**
     * @brief 计算扩增效率
     *
     * E = dilution^(-1/slope) - 1
     *
     * @param slope 回归斜率
     * @param dilution 稀释倍数
     * @return 扩增效率
     */
    static double calculateEfficiency(double slope, double dilution = 4.0);

    /**
     * @brief 格式化回归公式字符串
     * @param slope 斜率
     * @param intercept 截距
     * @return 公式字符串 (如 "y = -3.32*x + 25.6")
     */
    static QString formatFormula(double slope, double intercept);

private:
    /**
     * @brief 简单线性回归
     * @param x 自变量
     * @param y 因变量
     * @param slope 输出斜率
     * @param intercept 输出截距
     * @param rSquared 输出 R²
     * @param pValue 输出 P值
     */
    static void linearRegression(
        const QVector<double>& x,
        const QVector<double>& y,
        double& slope,
        double& intercept,
        double& rSquared,
        double& pValue
    );

    /**
     * @brief 计算对数浓度
     * @param concentrations 浓度向量
     * @param dilution 稀释倍数
     * @return 对数浓度向量
     */
    static QVector<double> logConcentrations(
        const QVector<double>& concentrations,
        double dilution
    );
};

} // namespace qpcr

#endif // STANDARDCURVE_H
