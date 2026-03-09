#ifndef STATISTICALTEST_H
#define STATISTICALTEST_H

#include <QVector>
#include <QString>

namespace qpcr {

/**
 * @brief 统计检验方法枚举
 */
enum class TestMethod {
    TTest,
    Wilcoxon,
    ANOVA
};

/**
 * @brief 统计检验结果
 */
struct TestResult {
    QString testName;         // 检验名称
    double statistic;         // 统计量 (t, F, W, etc.)
    double pValue;            // P值
    double confidenceLower;   // 置信区间下限
    double confidenceUpper;   // 置信区间上限
    QString significance;     // 显著性标记
    bool isSignificant;       // 是否显著 (alpha = 0.05)

    // 额外信息
    int degreesOfFreedom;     // 自由度
    QString effectSize;       // 效应量描述
};

/**
 * @brief 统计检验类
 *
 * 提供常用的统计检验方法
 * 使用 GSL 库进行精确计算
 */
class StatisticalTest
{
public:
    /**
     * @brief 执行独立样本 t检验
     *
     * H0: 两组均值相等
     * HA: 两组均值不相等
     *
     * @param group1 第一组数据
     * @param group2 第二组数据
     * @param equalVariance 是否假设方差相等 (Welch's t-test if false)
     * @param alpha 显著性水平，默认 0.05
     * @return 检验结果
     */
    static TestResult tTest(
        const QVector<double>& group1,
        const QVector<double>& group2,
        bool equalVariance = false,
        double alpha = 0.05
    );

    /**
     * @brief 执行配对样本 t检验
     *
     * @param before 处理前数据
     * @param after 处理后数据
     * @param alpha 显著性水平
     * @return 检验结果
     */
    static TestResult pairedTTest(
        const QVector<double>& before,
        const QVector<double>& after,
        double alpha = 0.05
    );

    /**
     * @brief Wilcoxon 秩和检验 (Mann-Whitney U test)
     *
     * 非参数检验，用于比较两组独立样本
     *
     * @param group1 第一组数据
     * @param group2 第二组数据
     * @param alpha 显著性水平
     * @return 检验结果
     */
    static TestResult wilcoxonTest(
        const QVector<double>& group1,
        const QVector<double>& group2,
        double alpha = 0.05
    );

    /**
     * @brief Wilcoxon 符号秩检验 (配对)
     *
     * @param before 处理前数据
     * @param after 处理后数据
     * @param alpha 显著性水平
     * @return 检验结果
     */
    static TestResult wilcoxonSignedRankTest(
        const QVector<double>& before,
        const QVector<double>& after,
        double alpha = 0.05
    );

    /**
     * @brief 单因素方差分析 (ANOVA)
     *
     * 比较多组均值
     *
     * @param groups 各组数据
     * @param alpha 显著性水平
     * @return 检验结果
     */
    static TestResult anova(
        const QVector<QVector<double>>& groups,
        double alpha = 0.05
    );

    /**
     * @brief Tukey's HSD 事后检验
     *
     * 在 ANOVA 显著后进行两两比较
     *
     * @param groups 各组数据
     * @param groupNames 组名
     * @param alpha 显著性水平
     * @return 所有两两比较的结果
     */
    static QVector<TestResult> tukeyHSD(
        const QVector<QVector<double>>& groups,
        const QStringList& groupNames,
        double alpha = 0.05
    );

    /**
     * @brief 计算效应量 (Cohen's d)
     *
     * @param group1 第一组数据
     * @param group2 第二组数据
     * @return Cohen's d 值
     */
    static double cohensD(
        const QVector<double>& group1,
        const QVector<double>& group2
    );

    /**
     * @brief 计算置信区间
     *
     * @param mean 均值
     * @param se 标准误
     * @param df 自由度
     * @param confidence 置信水平 (0-1)
     * @return QPair<下限, 上限>
     */
    static QPair<double, double> confidenceInterval(
        double mean,
        double se,
        int df,
        double confidence = 0.95
    );

    /**
     * @brief 计算 P值对应的显著性标记
     * @param pValue P值
     * @return 标记字符串
     */
    static QString formatSignificance(double pValue);

    /**
     * @brief 计算 Shapiro-Wilk 正态性检验
     *
     * @param data 数据向量
     * @return 检验结果
     */
    static TestResult shapiroWilkTest(const QVector<double>& data);

private:
    /**
     * @brief 计算均值
     */
    static double mean(const QVector<double>& data);

    /**
     * @brief 计算方差
     */
    static double variance(const QVector<double>& data);

    /**
     * @brief 计算 t分布的累积概率
     */
    static double tCDF(double t, int df);

    /**
     * @brief 计算 t分布的分位数
     */
    static double tQuantile(double p, int df);

    /**
     * @brief 计算 F分布的累积概率
     */
    static double fCDF(double f, int df1, int df2);
};

} // namespace qpcr

#endif // STATISTICALTEST_H
