#include "Core/StandardCurve.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace qpcr {

//=============================================================================
// Static public methods
//=============================================================================

QVector<StandardCurveResult> StandardCurve::calculate(
    const DataFrame& cqTable,
    const DataFrame& concenTable,
    double lowestConcen,
    double highestConcen,
    double dilution,
    bool byMean)
{
    QVector<StandardCurveResult> results;

    // Merge tables on Position
    DataFrame merged = cqTable.join(concenTable, "Position");

    // Check required columns
    if (!merged.hasColumn("Gene") ||
        !merged.hasColumn("Conc") ||
        !merged.hasColumn("Cq")) {
        qWarning() << "Missing required columns after merge";
        return results;
    }

    // Get unique genes
    QSet<QString> geneSet;
    auto genes = merged.getStringColumn("Gene");
    for (const auto& gene : genes) {
        geneSet.insert(gene);
    }

    // Process each gene
    for (const QString& gene : geneSet) {
        // Filter for this gene
        DataFrame geneData = merged.filter([gene](const Row& row) {
            return row.value("Gene").toString() == gene;
        });

        // Filter by concentration range
        auto concs = geneData.getNumericColumn("Conc");
        QVector<int> validRows;

        for (int i = 0; i < concs.size(); ++i) {
            if (concs[i] >= lowestConcen && concs[i] <= highestConcen) {
                validRows.append(i);
            }
        }

        if (validRows.isEmpty()) {
            qWarning() << "No valid data points for gene" << gene
                       << "in concentration range";
            continue;
        }

        // Extract valid data
        QVector<double> validConcs, validCqs;

        for (int row : validRows) {
            validConcs.append(geneData.get(row, "Conc").toDouble());
            validCqs.append(geneData.get(row, "Cq").toDouble());
        }

        // Calculate standard curve
        StandardCurveResult result;

        if (byMean) {
            // Group by concentration and average
            QHash<double, QVector<double>> concToCqs;

            for (int i = 0; i < validConcs.size(); ++i) {
                concToCqs[validConcs[i]].append(validCqs[i]);
            }

            QVector<double> uniqueConcs = concToCqs.keys().toVector();
            QVector<double> meanCqs;

            for (double conc : uniqueConcs) {
                const auto& cqs = concToCqs[conc];
                double sum = std::accumulate(cqs.begin(), cqs.end(), 0.0);
                meanCqs.append(sum / cqs.size());
            }

            result = calculateSingle(meanCqs, uniqueConcs, gene, dilution);
        } else {
            result = calculateSingle(validCqs, validConcs, gene, dilution);
        }

        results.append(result);
    }

    return results;
}

StandardCurveResult StandardCurve::calculateSingle(
    const QVector<double>& cqValues,
    const QVector<double>& concentrations,
    const QString& geneName,
    double dilution)
{
    StandardCurveResult result;
    result.gene = geneName;

    if (cqValues.size() != concentrations.size() || cqValues.isEmpty()) {
        qWarning() << "Invalid input data for standard curve calculation";
        result.slope = qQNaN();
        result.intercept = qQNaN();
        result.rSquared = qQNaN();
        result.pValue = qQNaN();
        result.efficiency = qQNaN();
        return result;
    }

    // Calculate log concentrations
    result.logConcentrations = logConcentrations(concentrations, dilution);
    result.cqValues = cqValues;

    // Find min/max Cq
    auto [minIt, maxIt] = std::minmax_element(cqValues.begin(), cqValues.end());
    result.minCq = *minIt;
    result.maxCq = *maxIt;

    // Perform linear regression: Cq ~ log(Conc)
    linearRegression(
        result.logConcentrations,
        result.cqValues,
        result.slope,
        result.intercept,
        result.rSquared,
        result.pValue
    );

    // Calculate efficiency
    result.efficiency = calculateEfficiency(result.slope, dilution);

    // Format formula
    result.formula = formatFormula(result.slope, result.intercept);

    // Calculate predicted values
    result.predictedCq.clear();
    for (double logConc : result.logConcentrations) {
        result.predictedCq.append(result.slope * logConc + result.intercept);
    }

    return result;
}

double StandardCurve::calculateEfficiency(double slope, double dilution) {
    // E = dilution^(-1/slope) - 1
    return std::pow(dilution, -1.0 / slope) - 1.0;
}

QString StandardCurve::formatFormula(double slope, double intercept) {
    return QString("y = %1*x + %2")
        .arg(slope, 0, 'f', 2)
        .arg(intercept, 0, 'f', 2);
}

//=============================================================================
// Private methods
//=============================================================================

void StandardCurve::linearRegression(
    const QVector<double>& x,
    const QVector<double>& y,
    double& slope,
    double& intercept,
    double& rSquared,
    double& pValue)
{
    int n = x.size();

    if (n < 2) {
        slope = qQNaN();
        intercept = qQNaN();
        rSquared = qQNaN();
        pValue = qQNaN();
        return;
    }

    // Calculate means
    double sumX = std::accumulate(x.begin(), x.end(), 0.0);
    double sumY = std::accumulate(y.begin(), y.end(), 0.0);

    double meanX = sumX / n;
    double meanY = sumY / n;

    // Calculate slope and intercept using least squares
    // slope = Σ((xi - x̄)(yi - ȳ)) / Σ((xi - x̄)²)
    double sumXY = 0.0;
    double sumXX = 0.0;
    double sumYY = 0.0;

    for (int i = 0; i < n; ++i) {
        double dx = x[i] - meanX;
        double dy = y[i] - meanY;
        sumXY += dx * dy;
        sumXX += dx * dx;
        sumYY += dy * dy;
    }

    slope = sumXY / sumXX;
    intercept = meanY - slope * meanX;

    // Calculate R²
    // R² = 1 - SSres / SStot
    double ssRes = 0.0;  // Residual sum of squares
    double ssTot = 0.0;  // Total sum of squares

    for (int i = 0; i < n; ++i) {
        double predicted = slope * x[i] + intercept;
        double residual = y[i] - predicted;
        double dy = y[i] - meanY;

        ssRes += residual * residual;
        ssTot += dy * dy;
    }

    if (ssTot > 0) {
        rSquared = 1.0 - (ssRes / ssTot);
    } else {
        rSquared = qQNaN();
    }

    // Calculate t-statistic and p-value for slope
    // t = slope / SE(slope)
    // SE(slope) = sqrt(SSres / (n-2) / sumXX)
    if (n > 2 && sumXX > 0) {
        double seSlope = std::sqrt(ssRes / (n - 2) / sumXX);

        if (seSlope > 0) {
            double tStat = slope / seSlope;

            // Approximate p-value using t-distribution (df = n-2)
            // For large n, this approximates normal distribution
            // Using two-tailed test
            int df = n - 2;

            // Simplified p-value calculation
            // For a more accurate result, use GSL or similar
            double absT = std::abs(tStat);

            // Approximate p-value
            if (absT < 1.96) {
                pValue = 0.05;  // Not significant
            } else if (absT < 2.58) {
                pValue = 0.01;  // Significant at 0.01 level
            } else if (absT < 3.29) {
                pValue = 0.001; // Significant at 0.001 level
            } else {
                pValue = 0.0001; // Highly significant
            }

            // Adjust for two-tailed
            pValue *= 2.0;
            if (pValue > 1.0) pValue = 1.0;
        } else {
            pValue = qQNaN();
        }
    } else {
        pValue = qQNaN();
    }
}

QVector<double> StandardCurve::logConcentrations(
    const QVector<double>& concentrations,
    double dilution)
{
    QVector<double> logConcs;
    logConcs.reserve(concentrations.size());

    for (double conc : concentrations) {
        // log with base = dilution
        // log_dilution(x) = ln(x) / ln(dilution)
        double logConc = std::log(conc) / std::log(dilution);
        logConcs.append(logConc);
    }

    return logConcs;
}

} // namespace qpcr
