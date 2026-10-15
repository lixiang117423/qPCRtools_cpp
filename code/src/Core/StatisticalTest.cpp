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
// 学生化极差分布（studentized range）：Tukey HSD 的精确 p 值
//
//   P(Q > q; k, ν) = ∫_0^∞ f_{χ²_ν}(u) · J(q·√(u/ν)) du
//   J(t) = ∫_{-∞}^{+∞} φ(z) [1 − k·(Φ(z+t) − Φ(z))^{k−1}] dz
//
// 双重积分形式取自 Batista et al. (2017)，与 scipy.stats.studentized_range 的积分核
// 一致（SciPy 为 BSD-3-Clause；R 的 ptukey 是 GPL，本实现未使用其代码）。
// 两重积分均用 Gauss–Legendre 求积：
//   · 内层（z）：6 段 × 16 点覆盖 [-12, 12]（|z| > 12 处 φ < 2e-32，可忽略）；
//   · 外层：令 y = √(χ²/ν) 换元消去 u^{ν/2−1} 在 0 处的奇性，[0, y_max] 按 0.6 等宽
//     分段、每段 12 点。
// 实测（2026-10-15，与自适应高精度积分逐点对照）：p ≥ 1e-6 区间相对误差 ≤ 3e-13；
// 更小的尾部受双精度相消限制，绝对误差 ~1e-16（即"远小于 α"的结论不受影响）。
// 注：R 的 ptukey 在极小误差自由度处本身不准——df=2、q=12、k=2 时 R 给 0.0134441，
// 而 2×10⁷ 次蒙特卡洛得 0.013639 ± 0.000051、高精度积分得 0.0136061；本实现与后两者
// 一致（详见 code/tests/generate_reference.R 中的说明）。
//=============================================================================

// n 点 Gauss–Legendre 节点与权重（Newton 迭代求 Legendre 多项式零点）
static void gaussLegendreRule(int n, QVector<double>& nodes, QVector<double>& weights)
{
    const double pi = 3.14159265358979323846;
    nodes.resize(n);
    weights.resize(n);

    for (int i = 0; i < n; ++i) {
        double x = std::cos(pi * (i + 0.75) / (n + 0.5));
        double derivative = 1.0;
        for (int iter = 0; iter < 100; ++iter) {
            double p0 = 1.0;
            double p1 = x;
            for (int j = 2; j <= n; ++j) {
                const double p2 = ((2.0 * j - 1.0) * x * p1 - (j - 1.0) * p0) / j;
                p0 = p1;
                p1 = p2;
            }
            derivative = n * (x * p1 - p0) / (x * x - 1.0);
            const double dx = p1 / derivative;
            x -= dx;
            if (std::abs(dx) < 1e-15) break;
        }
        nodes[i] = x;
        weights[i] = 2.0 / ((1.0 - x * x) * derivative * derivative);
    }
}

// 内层（z 方向）固定求积规则：6 段 × 16 点，只算一次
static const QVector<QPair<double, double>>& normalRangeQuadrature()
{
    static const QVector<QPair<double, double>> rule = [] {
        static const double panelEdges[] = {-12.0, -5.0, -2.0, 0.0, 2.0, 5.0, 12.0};
        const int nodesPerPanel = 16;
        QVector<double> xn, wn;
        gaussLegendreRule(nodesPerPanel, xn, wn);

        QVector<QPair<double, double>> out;
        out.reserve(6 * nodesPerPanel);
        for (int p = 0; p < 6; ++p) {
            const double a = panelEdges[p];
            const double b = panelEdges[p + 1];
            const double half = 0.5 * (b - a);
            const double mid = 0.5 * (a + b);
            for (int i = 0; i < nodesPerPanel; ++i) {
                out.append(qMakePair(mid + half * xn[i], half * wn[i]));
            }
        }
        return out;
    }();
    return rule;
}

// 整数幂（比 std::pow 快，且不引入 libm 的额外误差）
static double intPower(double base, int exponent)
{
    double result = 1.0;
    double factor = base;
    int e = exponent;
    while (e > 0) {
        if (e & 1) result *= factor;
        factor *= factor;
        e >>= 1;
    }
    return result;
}

// P(z < X ≤ z + t)，X ~ N(0,1)。按区间位置分段用 erf/erfc，避免大数相减。
static double normalIntervalProbability(double z, double t)
{
    const double invSqrt2 = 0.7071067811865476;
    if (z >= 0.0) {
        // 两端都在右半轴：用上尾 erfc 相减
        return 0.5 * (std::erfc(z * invSqrt2) - std::erfc((z + t) * invSqrt2));
    }
    if (z + t <= 0.0) {
        // 两端都在左半轴：用左尾 erfc 相减
        return 0.5 * (std::erfc(-(z + t) * invSqrt2) - std::erfc(-z * invSqrt2));
    }
    // 跨过 0：拆成 [z,0] + [0,z+t]，两项都是小量
    return 0.5 * std::erf(-z * invSqrt2) + 0.5 * std::erf((z + t) * invSqrt2);
}

// J(t) = ∫φ(z)[1 − k·(Φ(z+t) − Φ(z))^{k−1}]dz（rangeUpperTail 的内层量）
static double rangeUpperTailInner(double t, int k)
{
    const double invSqrt2Pi = 0.3989422804014327;
    const QVector<QPair<double, double>>& rule = normalRangeQuadrature();
    const int exponent = k - 1;

    double total = 0.0;
    for (const auto& node : rule) {
        const double z = node.first;
        const double a = normalIntervalProbability(z, t);
        const double powered = (exponent == 0) ? 1.0 : intPower(a, exponent);
        total += node.second * (invSqrt2Pi * std::exp(-0.5 * z * z)) * (1.0 - k * powered);
    }
    return total;
}

// 学生化极差分布的上尾 P(Q > q)：k 组、误差自由度 df
static double studentizedRangeUpperTail(double q, int k, double df)
{
    if (k < 2 || !(q > 0.0)) return 1.0;   // P(Q > 0) = 1
    if (!(df > 0.0)) return qQNaN();

    // 自由度极大时用 df = ∞ 的极限形式（与 R / SciPy 的渐近切换一致）
    if (df >= 1.0e5) return std::min(1.0, std::max(0.0, rangeUpperTailInner(q, k)));

    const double m = 0.5 * df;
    const double logNorm = -std::lgamma(m);
    const double scale = std::sqrt(2.0 / df);           // t = q · scale · y
    const double yMax = std::sqrt(m + 20.0 * std::sqrt(m) + 60.0);

    const int nodesPerPanel = 12;
    const double panelWidth = 0.6;
    QVector<double> xn, wn;
    gaussLegendreRule(nodesPerPanel, xn, wn);

    const int panelCount = std::max(4, static_cast<int>(std::ceil(yMax / panelWidth)));

    double total = 0.0;
    for (int p = 0; p < panelCount; ++p) {
        const double a = yMax * p / panelCount;
        const double b = yMax * (p + 1) / panelCount;
        const double half = 0.5 * (b - a);
        const double mid = 0.5 * (a + b);
        for (int i = 0; i < nodesPerPanel; ++i) {
            const double y = mid + half * xn[i];
            if (!(y > 0.0)) continue;
            // y = √(χ²/ν) 的密度：2·y^{ν−1}·e^{−νy²/2}·(ν/2)^{ν/2}/Γ(ν/2)
            const double logDensity = (2.0 * m - 1.0) * std::log(y) - y * y + logNorm;
            const double density = 2.0 * std::exp(logDensity);
            if (!(density > 0.0)) continue;
            total += half * wn[i] * density * rangeUpperTailInner(q * scale * y, k);
        }
    }

    if (!(total > 0.0)) return 0.0;     // 尾部极小时可能被数值噪声压成负值
    return std::min(1.0, total);
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

// 平均秩（并列取平均）→ 返回每个观测的秩；同时给出 Σ(t³-t)。
// 与 rankSumWithTies 用同一套排名规则，供精确分布与正态近似共用。
QVector<double> averageRanks(const QVector<double>& combined, double& tieCorrection)
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
    return ranks;
}

// 精确秩和检验（双侧），用于小样本——正态近似在 n 很小时会明显高估显著性：
// 例如 3 vs 3 完全分开时，正态近似给 p ≈ 0.0495（假阳性），精确值为 0.1。
//
// 做法：把观测按「秩的多重集合」分组（并列值共享同一个平均秩、各带重数），
// 用背包式 DP 数出「任取 n1 个观测」时秩和等于 w 的组合数，再按 R 的规则
// （|w − 均值| ≥ |观测值 − 均值|）累加双侧概率。因为用的是秩的多重集合，
// 有并列值时同样给出精确 p（对应新版 R wilcox.test(exact = TRUE) 的行为；
// 本项目 3 vs 3 含并列的算例已用 R 4.6.1 核对）。
//
// 返回 NaN 表示无法计算（空样本），调用方应回退到正态近似。
// 秩一律折半成整数（平均秩只可能是整数或半整数）以做数组索引。
double exactWilcoxonTwoSided(const QVector<double>& combined, int n1, double observedSumRanks)
{
    const int n = combined.size();
    if (n == 0 || n1 <= 0 || n1 >= n) return qQNaN();

    double tieCorrection = 0.0;
    const QVector<double> ranks = averageRanks(combined, tieCorrection);

    // rank2 = 2 × 秩（整数），并按取值聚合重数
    QVector<int> rank2Values;
    QVector<int> rank2Counts;
    int total2 = 0;
    for (double r : ranks) {
        const int r2 = static_cast<int>(std::lround(2.0 * r));
        total2 += r2;
        int pos = rank2Values.indexOf(r2);
        if (pos < 0) {
            rank2Values.append(r2);
            rank2Counts.append(1);
        } else {
            rank2Counts[pos] += 1;
        }
    }

    // dp[j][w] = 选 j 个观测、秩和（半秩单位）= w 的组合数
    const int maxW = total2;
    QVector<QVector<double>> dp(n1 + 1, QVector<double>(maxW + 1, 0.0));
    dp[0][0] = 1.0;
    for (int g = 0; g < rank2Values.size(); ++g) {
        const int r2 = rank2Values[g];
        const int m = rank2Counts[g];
        for (int j = qMin(n1, n); j >= 1; --j) {
            for (int w = maxW; w >= 0; --w) {
                double acc = 0.0;
                double binom = 1.0;   // C(m, k)
                for (int k = 1; k <= qMin(m, j); ++k) {
                    binom = binom * (m - k + 1) / k;
                    const int prevW = w - k * r2;
                    if (prevW < 0) break;
                    const double prev = dp[j - k][prevW];
                    if (prev > 0.0) acc += prev * binom;
                }
                dp[j][w] += acc;
            }
        }
    }

    // 双侧：累加与观测值同样极端或更极端的秩和（R 的判定规则）
    double total = 0.0;
    for (int w = 0; w <= maxW; ++w) total += dp[n1][w];
    if (!(total > 0.0)) return qQNaN();

    const double meanSum = n1 * (n + 1) / 2.0;
    const double observed2 = 2.0 * observedSumRanks;
    const double deviation2 = std::fabs(observed2 - 2.0 * meanSum);
    const double tol = 1e-9;

    double extreme = 0.0;
    for (int w = 0; w <= maxW; ++w) {
        if (dp[n1][w] <= 0.0) continue;
        const double dev = std::fabs(static_cast<double>(w) - 2.0 * meanSum);
        if (dev >= deviation2 - tol) extreme += dp[n1][w];
    }
    return extreme / total;
}

// 对 combined = a + b 计算平均秩（并列取平均秩），返回 a 部分的秩和与全体秩的平局校正项。
// tieCorrection = Σ (t³ - t)，供 R 一致的正态近似方差校正使用。
double rankSumWithTies(const QVector<double>& combined, int n1, double& tieCorrection)
{
    const QVector<double> ranks = averageRanks(combined, tieCorrection);
    double sumRanks1 = 0.0;
    for (int k = 0; k < n1 && k < ranks.size(); ++k) sumRanks1 += ranks[k];
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

    // 小样本改用精确检验（R wilcox.test 默认同样在小样本走精确分布，且新版 R
    // 在存在并列值时也能给精确 p；见 exactWilcoxonTwoSided 的说明）。
    // 阈值取 n1,n2 ≤ 25（N ≤ 50）：覆盖面已包含 qPCR 常见的 3–10 个生物学重复，
    // 而更大样本下正态近似与精确值差异已可忽略。
    if (n1 <= 25 && n2 <= 25) {
        const double exactP = exactWilcoxonTwoSided(combined, n1, sumRanks1);
        if (!qIsNaN(exactP) && exactP >= 0.0) p = exactP;
    }

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
            result.degreesOfFreedom = dfWithin;

            // 真 Tukey（Tukey–Kramer）：p = P(学生化极差 > Q)，对应 R 的
            // ptukey(Q, k, dfWithin, lower.tail = FALSE)。
            // 旧实现是「t 分布 + Bonferroni」，p 值偏保守且与 Tukey 的
            // 学生化极差分布不是同一个量（2026-10-15 更正）。
            const double p = studentizedRangeUpperTail(Q, k, dfWithin);
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
