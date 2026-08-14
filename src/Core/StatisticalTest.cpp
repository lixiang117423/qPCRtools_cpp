#include "Core/StatisticalTest.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <limits>
#include <QDebug>

// 如果有 GSL 库，使用精确计算
#ifdef HAS_GSL
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_randist.h>
#endif

namespace qpcr {

//=============================================================================
// 数值分布函数
//
// 无 GSL 时用正则化不完全 Beta 函数（连分数 + 对数空间）给出与 GSL/R 同精度
// 的 t/F p 值与分位数，取代旧的「正态近似 + 硬编码临界值」fallback。
//=============================================================================

// 不完全 Beta 的连分数（Lentz 算法，Numerical Recipes betacf）
static double betaContinuedFraction(double a, double b, double x)
{
    const int maxIter = 200;
    const double eps = 3.0e-12;
    const double fpMin = 1.0e-300;

    const double qab = a + b;
    const double qap = a + 1.0;
    const double qam = a - 1.0;
    double c = 1.0;
    double d = 1.0 - qab * x / qap;
    if (std::abs(d) < fpMin) d = fpMin;
    d = 1.0 / d;
    double h = d;

    for (int m = 1; m <= maxIter; ++m) {
        const int m2 = 2 * m;
        double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d;
        if (std::abs(d) < fpMin) d = fpMin;
        c = 1.0 + aa / c;
        if (std::abs(c) < fpMin) c = fpMin;
        d = 1.0 / d;
        h *= d * c;

        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d;
        if (std::abs(d) < fpMin) d = fpMin;
        c = 1.0 + aa / c;
        if (std::abs(c) < fpMin) c = fpMin;
        d = 1.0 / d;
        const double del = d * c;
        h *= del;
        if (std::abs(del - 1.0) < eps) break;
    }
    return h;
}

// 正则化不完全 Beta 函数 I_x(a, b)
static double incompleteBeta(double a, double b, double x)
{
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;

    const double bt = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b)
                               + a * std::log(x) + b * std::log1p(-x));
    if (x < (a + 1.0) / (a + b + 2.0)) {
        return bt * betaContinuedFraction(a, b, x) / a;
    }
    return 1.0 - bt * betaContinuedFraction(b, a, 1.0 - x) / b;
}

// 正则化不完全 Beta 的逆（二分法；统计用途精度足够）
static double incompleteBetaInverse(double p, double a, double b)
{
    if (p <= 0.0) return 0.0;
    if (p >= 1.0) return 1.0;

    double lo = 0.0, hi = 1.0;
    for (int i = 0; i < 200; ++i) {
        const double mid = 0.5 * (lo + hi);
        if (incompleteBeta(a, b, mid) < p) lo = mid;
        else hi = mid;
    }
    return 0.5 * (lo + hi);
}

// t 分布双侧 p 值：p = I_{df/(df+t²)}(df/2, 1/2)
static double tTwoSidedP(double t, double df)
{
    if (!(df > 0.0)) return qQNaN();
    const double x = df / (df + t * t);
    return incompleteBeta(df / 2.0, 0.5, x);
}

// t 分布双侧临界值：给定双侧水平 alpha，返回 t_{1-alpha/2, df}
static double tTwoSidedCritical(double alpha, double df)
{
    if (!(df > 0.0)) return qQNaN();
    const double x = incompleteBetaInverse(alpha, df / 2.0, 0.5);
    return std::sqrt(df * (1.0 / x - 1.0));
}

// F 分布上尾概率：P(F > f) = I_{df2/(df2+df1*f)}(df2/2, df1/2)
static double fUpperTailP(double f, double df1, double df2)
{
    if (!(f >= 0.0) || !(df1 > 0.0) || !(df2 > 0.0)) return qQNaN();
    return incompleteBeta(df2 / 2.0, df1 / 2.0, df2 / (df2 + df1 * f));
}

// Acklam 算法：标准正态分布的逆 CDF（无外部依赖，精度约 1e-9）。
static double inverseNormalCDF(double p) {
    static const double a[] = {
        -3.969683028665376e+01,  2.209460984245205e+02,
        -2.759285104469687e+02,  1.383577518672690e+02,
        -3.066479806614716e+01,  2.506628277459239e+00
    };
    static const double b[] = {
        -5.447609879822406e+01,  1.615858368580409e+02,
        -1.556989798598866e+02,  6.680131188771972e+01,
        -1.328068155288572e+01
    };
    static const double c[] = {
        -7.784894002430293e-03, -3.223964580411365e-01,
        -2.400758277161838e+00, -2.549732539343734e+00,
         4.374664141464968e+00,  2.938163982698783e+00
    };
    static const double d[] = {
         7.784695709041462e-03,  3.224671290700398e-01,
         2.445134137142996e+00,  3.754408661907416e+00
    };
    const double pLow = 0.02425;
    const double pHigh = 1.0 - pLow;

    if (p < pLow) {
        double q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0]*q + c[1])*q + c[2])*q + c[3])*q + c[4])*q + c[5]) /
               ((((d[0]*q + d[1])*q + d[2])*q + d[3])*q + 1.0);
    } else if (p <= pHigh) {
        double q = p - 0.5;
        double r = q * q;
        return (((((a[0]*r + a[1])*r + a[2])*r + a[3])*r + a[4])*r + a[5])*q /
               (((((b[0]*r + b[1])*r + b[2])*r + b[3])*r + b[4])*r + 1.0);
    } else {
        double q = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c[0]*q + c[1])*q + c[2])*q + c[3])*q + c[4])*q + c[5]) /
                ((((d[0]*q + d[1])*q + d[2])*q + d[3])*q + 1.0);
    }
}

static double normalCDF(double z)
{
    return 0.5 * (1.0 + std::erf(z / std::sqrt(2.0)));
}

//=============================================================================
// t-Test
//=============================================================================

TestResult StatisticalTest::tTest(
    const QVector<double>& group1,
    const QVector<double>& group2,
    bool equalVariance,
    double alpha)
{
    TestResult result;
    result.testName = equalVariance ? "Student's t-test" : "Welch's t-test";

    int n1 = group1.size();
    int n2 = group2.size();

    if (n1 < 2 || n2 < 2) {
        result.pValue = qQNaN();
        return result;
    }

    // Calculate means
    double mean1 = mean(group1);
    double mean2 = mean(group2);

    // Calculate variances
    double var1 = variance(group1);
    double var2 = variance(group2);

    // Calculate t-statistic
    double tStat, se, df;

    if (equalVariance) {
        // Pooled variance
        double pooledVar = ((n1 - 1) * var1 + (n2 - 1) * var2) / (n1 + n2 - 2);
        se = std::sqrt(pooledVar * (1.0 / n1 + 1.0 / n2));
        df = n1 + n2 - 2;
    } else {
        // Welch's t-test
        se = std::sqrt(var1 / n1 + var2 / n2);

        // Welch-Satterthwaite equation for degrees of freedom
        double num = var1 / n1 + var2 / n2;
        double denom1 = (var1 / n1) * (var1 / n1) / (n1 - 1);
        double denom2 = (var2 / n2) * (var2 / n2) / (n2 - 1);
        df = num * num / (denom1 + denom2);
    }

    const double diff = mean1 - mean2;

    if (se == 0.0) {
        // 两组方差均为 0：均值相等则 t=0, p=1；否则差异无穷显著。
        const bool sameMean = (diff == 0.0);
        result.statistic = sameMean ? 0.0
                                    : (diff > 0.0 ? std::numeric_limits<double>::infinity()
                                                  : -std::numeric_limits<double>::infinity());
        result.degreesOfFreedom = static_cast<int>(std::round(df));
        result.pValue = sameMean ? 1.0 : 0.0;
        result.significance = formatSignificance(result.pValue);
        result.isSignificant = result.pValue < alpha;
        result.confidenceLower = diff;
        result.confidenceUpper = diff;
        result.effectSize = "Cohen's d = NA";
        return result;
    }

    tStat = diff / se;
    result.statistic = tStat;
    result.degreesOfFreedom = static_cast<int>(std::round(df));

    // Calculate two-tailed p-value
#ifdef HAS_GSL
    double p = 2 * (1 - gsl_cdf_tdist_P(std::abs(tStat), df));
#else
    double p = tTwoSidedP(std::abs(tStat), df);
#endif

    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    // Confidence interval for difference
    auto ci = confidenceInterval(diff, se, static_cast<int>(df), 1 - alpha);
    result.confidenceLower = ci.first;
    result.confidenceUpper = ci.second;

    // Effect size (Cohen's d)
    double d = cohensD(group1, group2);
    result.effectSize = std::isnan(d)
        ? QStringLiteral("Cohen's d = NA")
        : QStringLiteral("Cohen's d = %1").arg(d, 0, 'f', 2);

    return result;
}

TestResult StatisticalTest::pairedTTest(
    const QVector<double>& before,
    const QVector<double>& after,
    double alpha)
{
    TestResult result;
    result.testName = "Paired t-test";

    int n = qMin(before.size(), after.size());
    if (n < 2) {
        result.pValue = qQNaN();
        return result;
    }

    // Calculate differences
    QVector<double> diffs;
    for (int i = 0; i < n; ++i) {
        diffs.append(after[i] - before[i]);
    }

    // Test if mean difference is zero
    double meanDiff = mean(diffs);
    double varDiff = variance(diffs);
    double se = std::sqrt(varDiff / n);

    if (se == 0.0) {
        const bool sameMean = (meanDiff == 0.0);
        result.statistic = sameMean ? 0.0
                                    : (meanDiff > 0.0 ? std::numeric_limits<double>::infinity()
                                                      : -std::numeric_limits<double>::infinity());
        result.degreesOfFreedom = n - 1;
        result.pValue = sameMean ? 1.0 : 0.0;
        result.significance = formatSignificance(result.pValue);
        result.isSignificant = result.pValue < alpha;
        return result;
    }

    double tStat = meanDiff / se;
    result.statistic = tStat;
    result.degreesOfFreedom = n - 1;

    // Two-tailed p-value
#ifdef HAS_GSL
    double p = 2 * (1 - gsl_cdf_tdist_P(std::abs(tStat), n - 1));
#else
    double p = tTwoSidedP(std::abs(tStat), n - 1);
#endif

    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    return result;
}

//=============================================================================
// Wilcoxon Tests
//=============================================================================

namespace {

// 对 combined = a + b 计算平均秩（并列取平均秩），返回 a 部分的秩和与全体秩的平局校正项。
// tieCorrection = Σ (t³ - t)，供 R 一致的正态近似方差校正使用。
double rankSumWithTies(const QVector<double>& combined, int n1, double& tieCorrection)
{
    const int n = combined.size();
    QVector<std::pair<double, int>> indexed;
    indexed.reserve(n);
    for (int i = 0; i < n; ++i) indexed.append({combined[i], i});
    std::sort(indexed.begin(), indexed.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    QVector<double> ranks(n);
    tieCorrection = 0.0;
    int i = 0;
    while (i < n) {
        int j = i;
        while (j + 1 < n && indexed[j + 1].first == indexed[i].first) ++j;
        const double avgRank = (i + j) / 2.0 + 1.0;  // 1-based 平均秩
        for (int k = i; k <= j; ++k) ranks[indexed[k].second] = avgRank;
        const double t = static_cast<double>(j - i + 1);
        tieCorrection += t * t * t - t;
        i = j + 1;
    }

    double sumRanks1 = 0.0;
    for (int k = 0; k < n1; ++k) sumRanks1 += ranks[k];
    return sumRanks1;
}

} // namespace

TestResult StatisticalTest::wilcoxonTest(
    const QVector<double>& group1,
    const QVector<double>& group2,
    double alpha)
{
    TestResult result;
    result.testName = "Wilcoxon rank-sum test (Mann-Whitney U)";

    int n1 = group1.size();
    int n2 = group2.size();
    if (n1 == 0 || n2 == 0) {
        result.pValue = qQNaN();
        return result;
    }

    // Combine all values and assign average ranks (ties → average rank)
    QVector<double> combined = group1 + group2;
    double tieCorrection = 0.0;
    double sumRanks1 = rankSumWithTies(combined, n1, tieCorrection);

    // U statistic
    double U1 = sumRanks1 - n1 * (n1 + 1) / 2.0;
    double U2 = n1 * n2 - U1;
    double U = qMin(U1, U2);

    // R wilcox.test 显示的 statistic 名为 W，实际等于 U1（秩和减去 n1(n1+1)/2）
    result.statistic = U1;

    // 正态近似（无连续性校正，对应 R wilcox.test(exact=FALSE, correct=FALSE)；
    // 并列用 R 一致的方差校正）。
    const int n = n1 + n2;
    double meanU = n1 * n2 / 2.0;
    double varU = (n1 * n2 / 12.0) * ((n + 1.0) - tieCorrection / (n * (n - 1.0)));
    double z = (U - meanU) / std::sqrt(varU);

    double p = 2 * (1 - normalCDF(std::abs(z)));
    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    return result;
}

TestResult StatisticalTest::wilcoxonSignedRankTest(
    const QVector<double>& before,
    const QVector<double>& after,
    double alpha)
{
    TestResult result;
    result.testName = "Wilcoxon signed-rank test";

    int n = qMin(before.size(), after.size());

    // Calculate differences and exclude zeros
    QVector<double> diffs;
    for (int i = 0; i < n; ++i) {
        double d = after[i] - before[i];
        if (std::abs(d) > 1e-10) {
            diffs.append(d);
        }
    }

    int n2 = diffs.size();
    if (n2 < 2) {
        result.statistic = qQNaN();
        result.pValue = qQNaN();
        return result;
    }

    // Rank absolute differences（并列取平均秩）
    QVector<std::pair<double, int>> absDiffs;
    absDiffs.reserve(n2);
    for (int i = 0; i < n2; ++i) {
        absDiffs.append({std::abs(diffs[i]), i});
    }
    std::sort(absDiffs.begin(), absDiffs.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    QVector<double> ranks(n2);
    double tieCorrection = 0.0;
    int i = 0;
    while (i < n2) {
        int j = i;
        while (j + 1 < n2 && absDiffs[j + 1].first == absDiffs[i].first) ++j;
        const double avgRank = (i + j) / 2.0 + 1.0;
        for (int k = i; k <= j; ++k) ranks[absDiffs[k].second] = avgRank;
        const double t = static_cast<double>(j - i + 1);
        tieCorrection += t * t * t - t;
        i = j + 1;
    }

    // R wilcox.test(paired=TRUE) 的 V = (before - after) 正差值的秩和。
    // 本函数 diffs = after - before，故取负差值（与 R 报告统计量一致）。
    double Wplus = 0;
    for (int k = 0; k < n2; ++k) {
        if (diffs[k] < 0) {
            Wplus += ranks[k];
        }
    }

    result.statistic = Wplus;

    // Normal approximation（R 一致：方差含并列校正，无连续性校正）
    double meanW = n2 * (n2 + 1) / 4.0;
    double varW = n2 * (n2 + 1) * (2 * n2 + 1) / 24.0 - tieCorrection / 48.0;
    double z = (Wplus - meanW) / std::sqrt(varW);

    double p = 2 * (1 - normalCDF(std::abs(z)));
    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    return result;
}

//=============================================================================
// ANOVA
//=============================================================================

TestResult StatisticalTest::anova(
    const QVector<QVector<double>>& groups,
    double alpha)
{
    TestResult result;
    result.testName = "One-way ANOVA";

    // 过滤空组：k 与 dfWithin 都应按有数据的组计算
    QVector<QVector<double>> nonEmpty;
    nonEmpty.reserve(groups.size());
    for (const auto& group : groups) {
        if (!group.isEmpty()) nonEmpty.append(group);
    }
    if (nonEmpty.size() < 2) {
        result.pValue = qQNaN();
        return result;
    }

    // Calculate total number of observations
    int nTotal = 0;
    for (const auto& group : nonEmpty) {
        nTotal += group.size();
    }

    if (nTotal < 3) {
        result.pValue = qQNaN();
        return result;
    }

    // Calculate overall mean
    double grandMean = 0;
    int count = 0;
    for (const auto& group : nonEmpty) {
        for (double val : group) {
            grandMean += val;
            count++;
        }
    }
    grandMean /= count;

    // Calculate between-group and within-group sum of squares
    double ssBetween = 0, ssWithin = 0;
    int k = nonEmpty.size();

    for (const auto& group : nonEmpty) {
        double groupMean = mean(group);
        ssBetween += group.size() * (groupMean - grandMean) * (groupMean - grandMean);

        for (double val : group) {
            ssWithin += (val - groupMean) * (val - groupMean);
        }
    }

    int dfBetween = k - 1;
    int dfWithin = nTotal - k;

    if (dfWithin <= 0) {
        // 每组只有一个观测：无法估计组内方差
        result.pValue = qQNaN();
        result.degreesOfFreedom = dfBetween;
        return result;
    }

    double msBetween = ssBetween / dfBetween;
    double msWithin = ssWithin / dfWithin;

    double F = msWithin > 0 ? msBetween / msWithin : qQNaN();
    result.statistic = F;
    result.degreesOfFreedom = dfBetween;

    double p;
    if (msWithin == 0.0) {
        // 组内无变异：均值相同 → p=1；均值不同 → p=0
        p = (msBetween == 0.0) ? 1.0 : 0.0;
    } else {
#ifdef HAS_GSL
        p = 1 - gsl_cdf_fdist_P(F, dfBetween, dfWithin);
#else
        p = fUpperTailP(F, dfBetween, dfWithin);
#endif
    }

    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    return result;
}

QVector<TestResult> StatisticalTest::tukeyHSD(
    const QVector<QVector<double>>& groups,
    const QStringList& groupNames,
    double alpha)
{
    QVector<TestResult> results;

    // 过滤空组（保持组名与数据一一对应）
    QVector<QVector<double>> nonEmpty;
    QStringList nonEmptyNames;
    for (int i = 0; i < groups.size(); ++i) {
        if (!groups[i].isEmpty()) {
            nonEmpty.append(groups[i]);
            nonEmptyNames.append(groupNames.value(i));
        }
    }

    int k = nonEmpty.size();
    if (k < 2) return results;

    // Calculate overall MSE from ANOVA
    int nTotal = 0;
    double grandMean = 0;
    for (const auto& group : nonEmpty) {
        for (double val : group) {
            grandMean += val;
            nTotal++;
        }
    }
    grandMean /= nTotal;

    double ssWithin = 0;
    int dfWithin = nTotal - k;

    for (const auto& group : nonEmpty) {
        double groupMean = mean(group);
        for (double val : group) {
            ssWithin += (val - groupMean) * (val - groupMean);
        }
    }

    if (dfWithin <= 0) {
        // 组内自由度不足，无法给出事后检验 p 值
        return results;
    }

    double MSE = ssWithin / dfWithin;

    // Perform all pairwise comparisons
    for (int i = 0; i < k; ++i) {
        for (int j = i + 1; j < k; ++j) {
            TestResult result;

            int ni = nonEmpty[i].size();
            int nj = nonEmpty[j].size();

            double meanI = mean(nonEmpty[i]);
            double meanJ = mean(nonEmpty[j]);
            double diff = meanI - meanJ;

            // Q statistic (Studentized range)
            double seQ = std::sqrt(MSE / 2 * (1.0 / ni + 1.0 / nj));
            double Q = std::abs(diff) / seQ;

            result.statistic = Q;
            result.testName = QStringLiteral("Tukey HSD: %1 vs %2")
                                  .arg(nonEmptyNames[i], nonEmptyNames[j]);
            result.group1Name = nonEmptyNames[i];
            result.group2Name = nonEmptyNames[j];

            // 用 t 分布 + Bonferroni 校正近似 Tukey HSD 的 p 值（保守但有效；
            // 与旧实现一致，但改用真实 t 分布而非数值积分旧版 incompleteBeta）。
            int numComparisons = k * (k - 1) / 2;
            double t = std::abs(diff) / std::sqrt(MSE * (1.0 / ni + 1.0 / nj));
            int df = dfWithin;

            double t_p;
#ifdef HAS_GSL
            t_p = 2 * (1 - gsl_cdf_tdist_P(t, df));
#else
            t_p = tTwoSidedP(t, df);
#endif

            // Apply Bonferroni correction
            double p = std::min(1.0, t_p * numComparisons);
            result.pValue = p;
            result.significance = formatSignificance(p);
            result.isSignificant = p < alpha;

            results.append(result);
        }
    }

    return results;
}

//=============================================================================
// Helper Functions
//=============================================================================

double StatisticalTest::cohensD(
    const QVector<double>& group1,
    const QVector<double>& group2)
{
    int n1 = group1.size();
    int n2 = group2.size();
    if (n1 < 2 || n2 < 2) return qQNaN();

    double mean1 = mean(group1);
    double mean2 = mean(group2);
    double var1 = variance(group1);
    double var2 = variance(group2);

    // Pooled standard deviation
    double pooledSD = std::sqrt(((n1 - 1) * var1 + (n2 - 1) * var2) / (n1 + n2 - 2));

    if (pooledSD == 0) return qQNaN();

    return (mean1 - mean2) / pooledSD;
}

QPair<double, double> StatisticalTest::confidenceInterval(
    double mean,
    double se,
    int df,
    double confidence)
{
    double alpha = 1 - confidence;
    double tCrit;

#ifdef HAS_GSL
    tCrit = gsl_cdf_tdist_Pinv(1 - alpha / 2, df);
#else
    if (df > 0) {
        // 用不完全 Beta 的逆求精确 t 临界值（旧实现用硬编码 2.0/2.5/3.291 近似）
        tCrit = tTwoSidedCritical(alpha, df);
    } else {
        // df ≤ 0 退化：正态近似
        tCrit = inverseNormalCDF(1 - alpha / 2);
    }
#endif

    double margin = tCrit * se;
    return {mean - margin, mean + margin};
}

QString StatisticalTest::formatSignificance(double pValue)
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

TestResult StatisticalTest::shapiroWilkTest(const QVector<double>& data)
{
    TestResult result;
    result.testName = "Shapiro-Wilk normality test";

    // 尚未实现：如实返回 NaN，而不是伪造 p 值。
    Q_UNUSED(data);
    result.statistic = qQNaN();
    result.pValue = qQNaN();
    result.significance = "NA";
    result.isSignificant = false;
    qWarning() << "StatisticalTest::shapiroWilkTest is not implemented; returning NA.";
    return result;
}

//=============================================================================
// Private Helpers
//=============================================================================

double StatisticalTest::mean(const QVector<double>& data)
{
    if (data.isEmpty()) return qQNaN();
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double StatisticalTest::variance(const QVector<double>& data)
{
    int n = data.size();
    if (n < 2) return qQNaN();

    double m = mean(data);
    double sum = 0;

    for (double val : data) {
        sum += (val - m) * (val - m);
    }

    return sum / (n - 1);
}

} // namespace qpcr
