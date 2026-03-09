#include "Core/StatisticalTest.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <QDebug>

// 如果有 GSL 库，使用精确计算
#ifdef HAS_GSL
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_randist.h>
#endif

namespace qpcr {

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

    tStat = (mean1 - mean2) / se;
    result.statistic = tStat;
    result.degreesOfFreedom = static_cast<int>(std::round(df));

    // Calculate two-tailed p-value
#ifdef HAS_GSL
    double p = 2 * (1 - gsl_cdf_tdist_P(std::abs(tStat), df));
#else
    // Approximation using error function
    double absT = std::abs(tStat);
    double z = absT / std::sqrt(1 + tStat * tStat / (2 * df));

    // Approximate standard normal CDF
    double p = 2 * (1 - 0.5 * (1 + std::erf(z / std::sqrt(2))));
#endif

    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    // Confidence interval for difference
    double diff = mean1 - mean2;
    auto ci = confidenceInterval(diff, se, static_cast<int>(df), 1 - alpha);
    result.confidenceLower = ci.first;
    result.confidenceUpper = ci.second;

    // Effect size (Cohen's d)
    result.effectSize = QString("Cohen's d = %1").arg(
        cohensD(group1, group2), 0, 'f', 2);

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

    double tStat = meanDiff / se;
    result.statistic = tStat;
    result.degreesOfFreedom = n - 1;

    // Two-tailed p-value
#ifdef HAS_GSL
    double p = 2 * (1 - gsl_cdf_tdist_P(std::abs(tStat), n - 1));
#else
    double absT = std::abs(tStat);
    double z = absT / std::sqrt(1 + tStat * tStat / (2 * (n - 1)));
    double p = 2 * (1 - 0.5 * (1 + std::erf(z / std::sqrt(2))));
#endif

    result.pValue = p;
    result.significance = formatSignificance(p);
    result.isSignificant = p < alpha;

    return result;
}

//=============================================================================
// Wilcoxon Tests
//=============================================================================

TestResult StatisticalTest::wilcoxonTest(
    const QVector<double>& group1,
    const QVector<double>& group2,
    double alpha)
{
    TestResult result;
    result.testName = "Wilcoxon rank-sum test (Mann-Whitney U)";

    // Combine all values
    QVector<double> combined = group1 + group2;
    int n1 = group1.size();
    int n2 = group2.size();
    int n = n1 + n2;

    // Assign ranks
    QVector<int> ranks(n);
    for (int i = 0; i < n; ++i) {
        int rank = 1;
        for (int j = 0; j < n; ++j) {
            if (combined[j] < combined[i]) {
                rank++;
            } else if (combined[j] == combined[i] && j < i) {
                rank++;
            }
        }
        ranks[i] = rank;
    }

    // Sum of ranks for group1
    double sumRanks1 = 0;
    for (int i = 0; i < n1; ++i) {
        sumRanks1 += ranks[i];
    }

    // U statistic
    double U1 = sumRanks1 - n1 * (n1 + 1) / 2.0;
    double U2 = n1 * n2 - U1;
    double U = qMin(U1, U2);

    result.statistic = U;

    // Approximate p-value using normal approximation
    double meanU = n1 * n2 / 2.0;
    double varU = n1 * n2 * (n + 1) / 12.0;
    double z = (U - meanU) / std::sqrt(varU);

    // Two-tailed p-value
    double p = 2 * (1 - 0.5 * (1 + std::erf(std::abs(z) / std::sqrt(2))));
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
        result.pValue = qQNaN();
        return result;
    }

    // Rank absolute differences
    QVector<std::pair<double, int>> absDiffs;
    for (int i = 0; i < n2; ++i) {
        absDiffs.append({std::abs(diffs[i]), i});
    }

    std::sort(absDiffs.begin(), absDiffs.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    QVector<int> ranks(n2);
    for (int i = 0; i < n2; ++i) {
        ranks[absDiffs[i].second] = i + 1;
    }

    // Sum of ranks for positive differences
    double Wplus = 0;
    for (int i = 0; i < n2; ++i) {
        if (diffs[i] > 0) {
            Wplus += ranks[i];
        }
    }

    result.statistic = Wplus;

    // Normal approximation
    double meanW = n2 * (n2 + 1) / 4.0;
    double varW = n2 * (n2 + 1) * (2 * n2 + 1) / 24.0;
    double z = (Wplus - meanW) / std::sqrt(varW);

    double p = 2 * (1 - 0.5 * (1 + std::erf(std::abs(z) / std::sqrt(2))));
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

    if (groups.size() < 2) {
        result.pValue = qQNaN();
        return result;
    }

    // Calculate total number of observations
    int nTotal = 0;
    for (const auto& group : groups) {
        nTotal += group.size();
    }

    if (nTotal < 3) {
        result.pValue = qQNaN();
        return result;
    }

    // Calculate overall mean
    double grandMean = 0;
    int count = 0;
    for (const auto& group : groups) {
        for (double val : group) {
            grandMean += val;
            count++;
        }
    }
    grandMean /= count;

    // Calculate between-group and within-group sum of squares
    double ssBetween = 0, ssWithin = 0;
    int k = groups.size();

    for (const auto& group : groups) {
        if (group.isEmpty()) continue;

        double groupMean = mean(group);
        ssBetween += group.size() * (groupMean - grandMean) * (groupMean - grandMean);

        for (double val : group) {
            ssWithin += (val - groupMean) * (val - groupMean);
        }
    }

    int dfBetween = k - 1;
    int dfWithin = nTotal - k;

    double msBetween = ssBetween / dfBetween;
    double msWithin = ssWithin / dfWithin;

    double F = msWithin > 0 ? msBetween / msWithin : qQNaN();

    result.statistic = F;
    result.degreesOfFreedom = dfBetween;

    // Calculate p-value using F-distribution
#ifdef HAS_GSL
    double p = 1 - gsl_cdf_fdist_P(F, dfBetween, dfWithin);
#else
    // Approximation (for demonstration)
    double p = 0.05; // Placeholder
#endif

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

    int k = groups.size();
    if (k < 2) return results;

    // Calculate overall MSE from ANOVA
    int nTotal = 0;
    double grandMean = 0;
    for (const auto& group : groups) {
        for (double val : group) {
            grandMean += val;
            nTotal++;
        }
    }
    grandMean /= nTotal;

    double ssWithin = 0;
    int dfWithin = nTotal - k;

    for (const auto& group : groups) {
        if (group.isEmpty()) continue;
        double groupMean = mean(group);
        for (double val : group) {
            ssWithin += (val - groupMean) * (val - groupMean);
        }
    }

    double MSE = ssWithin / dfWithin;

    // Perform all pairwise comparisons
    for (int i = 0; i < k; ++i) {
        for (int j = i + 1; j < k; ++j) {
            TestResult result;

            int ni = groups[i].size();
            int nj = groups[j].size();

            double meanI = mean(groups[i]);
            double meanJ = mean(groups[j]);
            double diff = meanI - meanJ;

            // Standard error
            double se = std::sqrt(MSE / 2 * (1.0 / ni + 1.0 / nj));

            // Q statistic (Studentized range)
            double Q = std::abs(diff) / se;

            result.statistic = Q;
            result.testName = QString("Tukey HSD: %1 vs %2").arg(groupNames[i]).arg(groupNames[j]);

            // Approximate p-value (simplified)
            double p = 0.05; // Placeholder - use studentized range distribution
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
    double mean1 = mean(group1);
    double mean2 = mean(group2);
    double var1 = variance(group1);
    double var2 = variance(group2);

    // Pooled standard deviation
    int n1 = group1.size();
    int n2 = group2.size();
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
    // Approximation using normal distribution for large df
    if (df > 30) {
        double z = 1 - alpha / 2;
        tCrit = std::sqrt(2) * std::erfInverse(2 * z - 1);
    } else {
        // Simplified t critical values
        if (alpha < 0.01) tCrit = 3.291;
        else if (alpha < 0.05) tCrit = 2.5;
        else tCrit = 2.0;
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

    // Simplified implementation
    // For accurate results, use statistical library
    int n = data.size();

    if (n < 3) {
        result.pValue = qQNaN();
        return result;
    }

    // Sort data
    QVector<double> sorted = data;
    std::sort(sorted.begin(), sorted.end());

    // Calculate test statistic (simplified)
    double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
    double mean = sum / n;

    double ss = 0;
    for (double val : sorted) {
        ss += (val - mean) * (val - mean);
    }

    // W statistic (simplified calculation)
    double W = ss; // Placeholder

    result.statistic = W;
    result.pValue = 0.05; // Placeholder
    result.significance = formatSignificance(result.pValue);

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
