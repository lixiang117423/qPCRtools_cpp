#include "Core/ExpressionCalculator.h"
#include "Core/StatisticalTest.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

namespace qpcr {

//=============================================================================
// ΔCt Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByDeltaCt(
    const DeltaCtParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "2^-ΔCt";

    qDebug() << "=== Starting ΔCt calculation ===";
    qDebug() << "Cq table rows:" << params.cqTable.rowCount();
    qDebug() << "Design table rows:" << params.designTable.rowCount();
    qDebug() << "Reference gene:" << params.referenceGene;

    // Merge tables
    DataFrame merged = params.cqTable.join(params.designTable, "Position");

    // Check required columns
    if (!merged.hasColumn("Gene") || !merged.hasColumn("Cq") ||
        !merged.hasColumn("Group") || !merged.hasColumn("BioRep")) {
        qWarning() << "Missing required columns";
        return result;
    }

    // Get unique groups and genes
    auto groups = merged.getStringColumn("Group");
    QSet<QString> groupSet;
    for (const auto& g : groups) {
        groupSet.insert(g);
    }

    auto allGenes = merged.getStringColumn("Gene");
    QSet<QString> geneSet;
    for (const auto& gene : allGenes) {
        if (gene != params.referenceGene) {
            geneSet.insert(gene);
        }
    }

    qDebug() << "Groups:" << groupSet;
    qDebug() << "Target genes:" << geneSet;

    // Storage for all expression values for statistical testing
    // Structure: allData[group][gene] = QVector<double> expressions
    QHash<QString, QHash<QString, QVector<double>>> allData;

    // Step 1: Calculate mean Cq for reference gene in each (group, biorep) combination
    // Key: "Group_BioRep", Value: mean Cq
    QHash<QString, double> refGeneMeanCq;

    for (const QString& group : groupSet) {
        // Get all unique bioReps in this group
        DataFrame groupData = merged.filter([group](const Row& row) {
            return row.value("Group").toString() == group;
        });

        auto bioReps = groupData.getStringColumn("BioRep");
        QSet<QString> bioRepSet;
        for (const auto& rep : bioReps) {
            bioRepSet.insert(rep);
        }

        // For each bioRep, calculate mean Cq of reference gene
        for (const QString& bioRep : bioRepSet) {
            DataFrame refData = groupData.filter([bioRep, &params](const Row& row) {
                return row.value("BioRep").toString() == bioRep &&
                       row.value("Gene").toString() == params.referenceGene;
            });

            if (refData.rowCount() > 0) {
                auto cqValues = refData.getNumericColumn("Cq");
                double sum = 0.0;
                for (double cq : cqValues) {
                    sum += cq;
                }
                double meanCq = sum / cqValues.size();

                QString key = group + "_" + bioRep;
                refGeneMeanCq[key] = meanCq;

                qDebug() << "Ref gene mean Cq for" << key << ":" << meanCq;
            }
        }
    }

    // Step 2: Calculate expression for each target gene
    // For each (group, biorep, gene) combination: expression = 2^(mean.ref.cq - target.cq)
    QVector<QVariant> finalGroups, finalGenes, finalMeans, finalStdDevs;

    for (const QString& group : groupSet) {
        for (const QString& gene : geneSet) {
            // Get all expression values for this (group, gene) combination
            QVector<double> expressions;

            // Get all bioReps for this group
            DataFrame groupData = merged.filter([group](const Row& row) {
                return row.value("Group").toString() == group;
            });

            auto bioReps = groupData.getStringColumn("BioRep");
            QSet<QString> bioRepSet;
            for (const auto& rep : bioReps) {
                bioRepSet.insert(rep);
            }

            // For each bioRep, get target gene Cq and calculate expression
            for (const QString& bioRep : bioRepSet) {
                QString refKey = group + "_" + bioRep;
                if (!refGeneMeanCq.contains(refKey)) {
                    qWarning() << "No reference gene data for" << refKey;
                    continue;
                }

                double meanRefCq = refGeneMeanCq[refKey];

                // Get target gene Cq values for this bioRep
                DataFrame targetData = groupData.filter([bioRep, gene](const Row& row) {
                    return row.value("BioRep").toString() == bioRep &&
                           row.value("Gene").toString() == gene;
                });

                if (targetData.rowCount() > 0) {
                    auto targetCqValues = targetData.getNumericColumn("Cq");
                    for (double targetCq : targetCqValues) {
                        // Calculate expression: 2^(mean.ref.cq - target.cq)
                        double deltaCt = meanRefCq - targetCq;
                        double expression = std::pow(2.0, deltaCt);
                        expressions.append(expression);
                    }
                }
            }

            if (expressions.isEmpty()) {
                qWarning() << "No expression data for" << group << gene;
                continue;
            }

            // Store all expression values for statistical testing
            allData[group][gene] = expressions;

            // Calculate statistics for this (group, gene) combination
            double mean = std::accumulate(expressions.begin(), expressions.end(), 0.0) / expressions.size();

            double sumSquaredDiff = 0.0;
            for (double val : expressions) {
                double diff = val - mean;
                sumSquaredDiff += diff * diff;
            }
            double stdDev = (expressions.size() > 1) ?
                std::sqrt(sumSquaredDiff / (expressions.size() - 1)) : 0.0;

            finalGroups.append(group);
            finalGenes.append(gene);
            finalMeans.append(mean);
            finalStdDevs.append(stdDev);

            qDebug() << "DeltaCt result:" << group << gene << "n=" << expressions.size()
                     << "mean=" << mean << "sd=" << stdDev;
        }
    }

    // Build result table
    DataFrame resultTable;
    resultTable.addColumn("Gene", finalGenes);
    resultTable.addColumn("Group", finalGroups);
    resultTable.addColumn("Mean", finalMeans);
    resultTable.addColumn("StdDev", finalStdDevs);

    qDebug() << "Final result table rows:" << resultTable.rowCount();
    qDebug() << "Final result table columns:" << resultTable.columns();

    // Perform statistical tests
    // For ΔCt, we compare between groups (use controlGroup as reference)
    QString refGroup = params.controlGroup.isEmpty() ? (groupSet.isEmpty() ? "" : *groupSet.begin()) : params.controlGroup;

    qDebug() << "Using reference group:" << refGroup << "for statistical tests";

    if (statMethod == "t.test") {
        for (const QString& gene : geneSet) {
            // Test all non-reference groups
            for (const QString& testGroup : groupSet) {
                if (testGroup == refGroup) continue;

                // Get expression values from allData
                QVector<double> refValues = allData.value(refGroup).value(gene);
                QVector<double> testValues = allData.value(testGroup).value(gene);

                // Perform t-test
                StatisticalResult statResult = performTTest(refValues, testValues, gene, refGroup, testGroup);
                result.statistics.append(statResult);
            }
        }
    } else if (statMethod == "wilcox.test") {
        for (const QString& gene : geneSet) {
            // Test all non-reference groups
            for (const QString& testGroup : groupSet) {
                if (testGroup == refGroup) continue;

                // Get expression values from allData
                QVector<double> refValues = allData.value(refGroup).value(gene);
                QVector<double> testValues = allData.value(testGroup).value(gene);

                StatisticalResult statResult = performWilcoxonTest(refValues, testValues, gene, refGroup, testGroup);
                result.statistics.append(statResult);
            }
        }
    } else if (statMethod == "anova") {
        // ANOVA not yet supported for ΔCt method
        qWarning() << "ANOVA not yet supported for ΔCt method, using t.test instead";
        // Fall through to t-test
        for (const QString& gene : geneSet) {
            // Test all non-reference groups
            for (const QString& testGroup : groupSet) {
                if (testGroup == refGroup) continue;

                QVector<double> refValues = allData.value(refGroup).value(gene);
                QVector<double> testValues = allData.value(testGroup).value(gene);

                StatisticalResult statResult = performTTest(refValues, testValues, gene, refGroup, testGroup);
                result.statistics.append(statResult);
            }
        }
    }

    // Add p-values and significance to table
    QVector<QVariant> finalPValues, finalSignifs;

    for (int i = 0; i < resultTable.rowCount(); ++i) {
        QString group = resultTable.get(i, "Group").toString();
        QString gene = resultTable.get(i, "Gene").toString();

        // Find p-value for this group-gene combination
        bool foundStat = false;
        for (const auto& stat : result.statistics) {
            if (stat.gene == gene && stat.group2 == group) {
                finalPValues.append(stat.pValue);
                finalSignifs.append(stat.significance);
                foundStat = true;
                break;
            }
        }

        if (!foundStat) {
            finalPValues.append(QVariant()); // No p-value (reference group)
            finalSignifs.append(""); // No significance
        }
    }

    // Rebuild table with all columns
    resultTable = DataFrame();
    resultTable.addColumn("Gene", finalGenes);
    resultTable.addColumn("Group", finalGroups);
    resultTable.addColumn("Mean", finalMeans);
    resultTable.addColumn("StdDev", finalStdDevs);
    resultTable.addColumn("PValue", finalPValues);
    resultTable.addColumn("Significance", finalSignifs);

    result.table = resultTable;
    return result;
}

//=============================================================================
// ΔΔCt Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByDeltaDeltaCt(
    const DeltaDeltaCtParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "2^-ΔΔCt";

    qDebug() << "[ExpressionCalculator] Input Cq rows:" << params.cqTable.rowCount()
             << "columns:" << params.cqTable.columns();
    qDebug() << "[ExpressionCalculator] Input Design rows:" << params.designTable.rowCount()
             << "columns:" << params.designTable.columns();

    // Merge tables
    DataFrame merged = params.cqTable.join(params.designTable, "Position");

    qDebug() << "[ExpressionCalculator] After join - Merged rows:" << merged.rowCount()
             << "columns:" << merged.columns();

    // Debug: Show first few rows of merged data
    qDebug() << "First 6 rows of merged data:";
    for (int i = 0; i < qMin(6, merged.rowCount()); ++i) {
        qDebug() << "  Row" << i << ":"
                 << "Position=" << merged.get(i, "Position").toString()
                 << "Gene=" << merged.get(i, "Gene").toString()
                 << "Cq=" << merged.get(i, "Cq").toDouble()
                 << "Group=" << merged.get(i, "Group").toString()
                 << "BioRep=" << merged.get(i, "BioRep").toString();
    }

    // Check required columns
    if (!merged.hasColumn("Gene") || !merged.hasColumn("Cq") ||
        !merged.hasColumn("Group") || !merged.hasColumn("BioRep")) {
        qWarning() << "Missing required columns. Has Gene:" << merged.hasColumn("Gene")
                   << "Cq:" << merged.hasColumn("Cq")
                   << "Group:" << merged.hasColumn("Group")
                   << "BioRep:" << merged.hasColumn("BioRep");
        return result;
    }

    // Get target genes (exclude reference)
    auto allGenes = merged.getStringColumn("Gene");
    QSet<QString> geneSet;
    for (const auto& gene : allGenes) {
        if (gene != params.referenceGene) {
            geneSet.insert(gene);
        }
    }

    qDebug() << "All genes in data:" << allGenes;
    qDebug() << "Reference gene:" << params.referenceGene;
    qDebug() << "Target genes:" << geneSet.values();
    qDebug() << "Target gene count:" << geneSet.size();

    // Get all groups and sort them
    auto allGroups = merged.getStringColumn("Group");
    QSet<QString> groupSet;
    for (const auto& group : allGroups) {
        groupSet.insert(group);
    }

    // Sort genes and groups for consistent output
    QList<QString> sortedGenes = geneSet.values();
    std::sort(sortedGenes.begin(), sortedGenes.end());

    QList<QString> sortedGroups = groupSet.values();
    std::sort(sortedGroups.begin(), sortedGroups.end());

    // Build result table structure
    QVector<QVariant> groups, genes, bioreps, expressions, signifs;
    QVector<double> expressionValues;

    // Calculate mean ΔCt for control group
    QHash<QString, double> controlDeltaCtByGene;

    for (const QString& gene : sortedGenes) {
        qDebug() << "Processing gene:" << gene << "for control group";

        // Filter for control group
        DataFrame controlData = merged.filter(
            [params, gene](const Row& row) {
                return row.value("Group").toString() == params.controlGroup &&
                       (row.value("Gene").toString() == gene ||
                        row.value("Gene").toString() == params.referenceGene);
            });

        qDebug() << "  Control data rows:" << controlData.rowCount();

        // Get mean Cq values
        QVector<double> targetCqs, refCqs;

        for (int i = 0; i < controlData.rowCount(); ++i) {
            QString g = controlData.get(i, "Gene").toString();
            double cq = controlData.get(i, "Cq").toDouble();

            if (g == gene) {
                targetCqs.append(cq);
            } else if (g == params.referenceGene) {
                refCqs.append(cq);
            }
        }

        if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
            double meanTarget = std::accumulate(targetCqs.begin(), targetCqs.end(), 0.0) / targetCqs.size();
            double meanRef = std::accumulate(refCqs.begin(), refCqs.end(), 0.0) / refCqs.size();
            controlDeltaCtByGene[gene] = meanTarget - meanRef;
        }
    }

    // Process each group (sorted order)
    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            qDebug() << "Processing group:" << group << "gene:" << gene;

            // Filter for this group
            DataFrame groupData = merged.filter(
                [group, gene, params](const Row& row) {
                    return row.value("Group").toString() == group &&
                           (row.value("Gene").toString() == gene ||
                            row.value("Gene").toString() == params.referenceGene);
                });

            qDebug() << "  Group data rows:" << groupData.rowCount();

            // Group by BioRep and sort them
            auto bioReps = groupData.getStringColumn("BioRep");
            QSet<QString> bioRepSet;
            for (const auto& rep : bioReps) {
                bioRepSet.insert(rep);
            }

            // Sort BioReps for consistent output
            QList<QString> sortedBioReps = bioRepSet.values();
            std::sort(sortedBioReps.begin(), sortedBioReps.end());

            qDebug() << "    BioReps in this group:" << bioReps << "Count:" << bioRepSet.size();

            // Calculate expression for each biological replicate
            QHash<QString, double> repExpression;

            for (const QString& bioRep : sortedBioReps) {
                qDebug() << "    Processing BioRep:" << bioRep;

                DataFrame repData = groupData.filter(
                    [bioRep](const Row& row) {
                        return row.value("BioRep").toString() == bioRep;
                    });

                qDebug() << "      RepData rows:" << repData.rowCount();

                QVector<double> targetCqs, refCqs;

                for (int i = 0; i < repData.rowCount(); ++i) {
                    QString g = repData.get(i, "Gene").toString();
                    double cq = repData.get(i, "Cq").toDouble();
                    qDebug() << "        Gene:" << g << "Cq:" << cq;

                    if (g == gene) {
                        targetCqs.append(cq);
                    } else if (g == params.referenceGene) {
                        refCqs.append(cq);
                    }
                }

                qDebug() << "      targetCqs:" << targetCqs << "refCqs:" << refCqs;

                if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
                    // Calculate mean Cq for technical replicates
                    double meanTarget = std::accumulate(targetCqs.begin(), targetCqs.end(), 0.0) / targetCqs.size();
                    double meanRef = std::accumulate(refCqs.begin(), refCqs.end(), 0.0) / refCqs.size();

                    double deltaCt = meanTarget - meanRef;
                    double deltaDeltaCt = deltaCt - controlDeltaCtByGene.value(gene, 0.0);

                    double expression = calculateExpressionFromDeltaDeltaCt(deltaDeltaCt, 0.0);
                    repExpression[bioRep] = expression;
                    expressionValues.append(expression);
                    qDebug() << "      Expression:" << expression;
                } else {
                    qDebug() << "      Missing target or ref Cqs, skipping";
                }
            }

            // Remove outliers if requested
            QVector<double> finalExpressions;

            qDebug() << "    repExpression has" << repExpression.size() << "entries";

            for (auto it = repExpression.begin(); it != repExpression.end(); ++it) {
                groups.append(group);
                genes.append(gene);
                bioreps.append(it.key());
                expressions.append(it.value());
                finalExpressions.append(it.value());
                qDebug() << "    Added to result: Group=" << group << "Gene=" << gene << "BioRep=" << it.key() << "Expression=" << it.value();
            }

            if (params.removeOutliers) {
                finalExpressions = removeOutliers(finalExpressions);
            }
        }
    }

    // Build summary result table with mean, std dev, and p-values
    DataFrame resultTable;

    qDebug() << "Building summary result table:";

    // Prepare vectors for summary table
    QVector<QVariant> summaryGroups;
    QVector<QVariant> summaryGenes;
    QVector<QVariant> summaryMeans;
    QVector<QVariant> summaryStdDevs;
    QVector<QVariant> summaryPValues;
    QVector<QVariant> summarySignificance;

    // First, collect all expressions for each group-gene combination
    QHash<QString, QVector<double>> groupGeneExpressions; // Key: "Group_Gene"

    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            QString key = group + "_" + gene;
            groupGeneExpressions[key] = QVector<double>();
        }
    }

    // Populate expressions from repExpression
    // Note: We need to recalculate since repExpression is inside the loop
    QHash<QString, QHash<QString, QVector<double>>> allData; // group -> gene -> values

    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            // Filter for this group and gene
            DataFrame groupData = merged.filter(
                [group, gene, params](const Row& row) {
                    return row.value("Group").toString() == group &&
                           (row.value("Gene").toString() == gene ||
                            row.value("Gene").toString() == params.referenceGene);
                });

            // Group by BioRep
            auto bioReps = groupData.getStringColumn("BioRep");
            QSet<QString> bioRepSet;
            for (const auto& rep : bioReps) {
                bioRepSet.insert(rep);
            }

            // Calculate expression for each biological replicate
            for (const QString& bioRep : bioRepSet) {
                DataFrame repData = groupData.filter(
                    [bioRep](const Row& row) {
                        return row.value("BioRep").toString() == bioRep;
                    });

                QVector<double> targetCqs, refCqs;

                for (int i = 0; i < repData.rowCount(); ++i) {
                    QString g = repData.get(i, "Gene").toString();
                    double cq = repData.get(i, "Cq").toDouble();

                    if (g == gene) {
                        targetCqs.append(cq);
                    } else if (g == params.referenceGene) {
                        refCqs.append(cq);
                    }
                }

                if (!targetCqs.isEmpty() && !refCqs.isEmpty()) {
                    double meanTarget = std::accumulate(targetCqs.begin(), targetCqs.end(), 0.0) / targetCqs.size();
                    double meanRef = std::accumulate(refCqs.begin(), refCqs.end(), 0.0) / refCqs.size();

                    double deltaCt = meanTarget - meanRef;
                    double deltaDeltaCt = deltaCt - controlDeltaCtByGene.value(gene, 0.0);

                    double expression = calculateExpressionFromDeltaDeltaCt(deltaDeltaCt, 0.0);
                    allData[group][gene].append(expression);
                }
            }
        }
    }

    // Calculate statistics for each group-gene combination (sorted order)
    for (const QString& group : sortedGroups) {
        for (const QString& gene : sortedGenes) {
            QVector<double> values = allData[group][gene];

            if (values.isEmpty()) {
                continue;
            }

            // Calculate mean
            double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();

            // Calculate standard deviation
            double sumSquaredDiff = 0.0;
            for (double val : values) {
                double diff = val - mean;
                sumSquaredDiff += diff * diff;
            }
            double stdDev = (values.size() > 1) ? std::sqrt(sumSquaredDiff / (values.size() - 1)) : 0.0;

            summaryGroups.append(group);
            summaryGenes.append(gene);
            summaryMeans.append(mean);
            summaryStdDevs.append(stdDev);

            qDebug() << "Summary:" << group << gene << "n=" << values.size() << "mean=" << mean << "std=" << stdDev;
        }
    }

    // Add columns to result table
    resultTable.addColumn("Group", summaryGroups);
    resultTable.addColumn("Gene", summaryGenes);
    resultTable.addColumn("Mean", summaryMeans);
    resultTable.addColumn("StdDev", summaryStdDevs);

    qDebug() << "Summary table rows:" << resultTable.rowCount();

    // Perform statistical tests and add p-values to the table
    if (statMethod == "t.test") {
        // t-test for each gene comparing control to all other groups
        for (const QString& gene : sortedGenes) {
            QVector<double> controlValues;

            // Get control group values
            if (allData.contains(params.controlGroup) && allData[params.controlGroup].contains(gene)) {
                controlValues = allData[params.controlGroup][gene];
            }

            // Test all non-control groups (sorted order)
            for (const QString& grp : sortedGroups) {
                if (grp == params.controlGroup) continue;

                QVector<double> treatedValues;
                if (allData.contains(grp) && allData[grp].contains(gene)) {
                    treatedValues = allData[grp][gene];
                }

                if (!controlValues.isEmpty() && !treatedValues.isEmpty()) {
                    StatisticalResult stat = performTTest(
                        controlValues,
                        treatedValues,
                        gene,
                        params.controlGroup,
                        grp
                    );
                    result.statistics.append(stat);
                }
            }
        }
    } else if (statMethod == "wilcox.test") {
        // Wilcoxon test for each gene comparing control to all other groups
        for (const QString& gene : sortedGenes) {
            QVector<double> controlValues;

            // Get control group values
            if (allData.contains(params.controlGroup) && allData[params.controlGroup].contains(gene)) {
                controlValues = allData[params.controlGroup][gene];
            }

            // Test all non-control groups (sorted order)
            for (const QString& grp : sortedGroups) {
                if (grp == params.controlGroup) continue;

                QVector<double> treatedValues;
                if (allData.contains(grp) && allData[grp].contains(gene)) {
                    treatedValues = allData[grp][gene];
                }

                if (!controlValues.isEmpty() && !treatedValues.isEmpty()) {
                    StatisticalResult stat = performWilcoxonTest(
                        controlValues,
                        treatedValues,
                        gene,
                        params.controlGroup,
                        grp
                    );
                    result.statistics.append(stat);
                }
            }
        }
    } else if (statMethod == "anova") {
        qDebug() << "=== Performing ANOVA analysis ===";

        // For each gene, perform ANOVA and Tukey HSD (sorted order)
        for (const QString& gene : sortedGenes) {
            qDebug() << "Processing gene:" << gene;

            // Collect all group data for this gene
            QVector<QVector<double>> groupDataList;
            QStringList groupNamesList;
            QHash<QString, double> geneGroupMeans;

            for (const QString& grp : sortedGroups) {
                if (allData.contains(grp) && allData[grp].contains(gene)) {
                    QVector<double> values = allData[grp][gene];
                    if (!values.isEmpty()) {
                        groupDataList.append(values);
                        groupNamesList.append(grp);

                        // Calculate mean for this group
                        double sum = std::accumulate(values.begin(), values.end(), 0.0);
                        double mean = sum / values.size();
                        geneGroupMeans[grp] = mean;

                        qDebug() << "  Group" << grp << ": n =" << values.size() << "mean =" << mean;
                    }
                }
            }

            if (groupDataList.size() < 2) {
                qWarning() << "Not enough groups for ANOVA on gene" << gene;
                continue;
            }

            // Perform ANOVA
            TestResult anovaResult = StatisticalTest::anova(groupDataList, 0.05);
            qDebug() << "  ANOVA F =" << anovaResult.statistic << "p =" << anovaResult.pValue;

            // Perform Tukey HSD post-hoc test
            QVector<TestResult> tukeyResults = StatisticalTest::tukeyHSD(groupDataList, groupNamesList, 0.05);
            qDebug() << "  Tukey HSD comparisons:" << tukeyResults.size();
            for (const auto& result : tukeyResults) {
                qDebug() << "    " << result.testName << "p =" << result.pValue << "significant =" << result.isSignificant;
            }

            // Generate letter groups
            QHash<QString, QString> letterGroups = generateLetterGroups(geneGroupMeans, tukeyResults, 0.05);
            qDebug() << "  Letter groups:" << letterGroups;

            // Store results for each group (sorted order)
            for (const QString& grp : sortedGroups) {
                if (!allData.contains(grp) || !allData[grp].contains(gene)) continue;

                StatisticalResult stat;
                stat.gene = gene;
                stat.group = grp;
                stat.group1 = "All";
                stat.group2 = grp;
                stat.pValue = anovaResult.pValue;
                stat.fStatistic = anovaResult.statistic;
                stat.significance = letterGroups.value(grp, "");
                stat.letterGroup = letterGroups.value(grp, "");

                result.statistics.append(stat);
            }
        }
    }

    // Create enhanced table with p-values included
    // We need to rebuild the table to include p-values
    QVector<QVariant> finalGroups, finalGenes, finalMeans, finalStdDevs, finalPValues, finalSignifs;

    for (int i = 0; i < resultTable.rowCount(); ++i) {
        QString group = resultTable.get(i, "Group").toString();
        QString gene = resultTable.get(i, "Gene").toString();
        double mean = resultTable.get(i, "Mean").toDouble();
        double stdDev = resultTable.get(i, "StdDev").toDouble();

        finalGroups.append(group);
        finalGenes.append(gene);
        finalMeans.append(mean);
        finalStdDevs.append(stdDev);

        // Find p-value for this group-gene combination
        bool foundStat = false;
        for (const auto& stat : result.statistics) {
            if (stat.gene == gene && stat.group2 == group) {
                finalPValues.append(stat.pValue);
                finalSignifs.append(stat.significance);
                foundStat = true;
                break;
            }
        }

        if (!foundStat) {
            finalPValues.append(QVariant()); // No p-value (control group)
            finalSignifs.append(""); // No significance
        }
    }

    // Rebuild table with all columns
    resultTable = DataFrame();
    resultTable.addColumn("Gene", finalGenes);
    resultTable.addColumn("Group", finalGroups);
    resultTable.addColumn("Mean", finalMeans);
    resultTable.addColumn("StdDev", finalStdDevs);
    resultTable.addColumn("PValue", finalPValues);
    resultTable.addColumn("Significance", finalSignifs);

    qDebug() << "Final result table rows:" << resultTable.rowCount();
    qDebug() << "Final result table columns:" << resultTable.columns();

    result.table = resultTable;

    // Build raw data table (BioRep level data) in sorted order
    qDebug() << "Building raw data table...";

    // First, organize all data into a nested structure for easy access
    // Structure: group -> gene -> bioRep -> expression
    QHash<QString, QHash<QString, QHash<QString, double>>> organizedData;
    QHash<QString, double> groupGeneMeans;
    QHash<QString, double> groupGeneSDs;
    QHash<QString, int> groupGeneCounts;

    for (int i = 0; i < groups.size(); ++i) {
        QString group = groups[i].toString();
        QString gene = genes[i].toString();
        QString bioRep = bioreps[i].toString();
        double expr = expressions[i].toDouble();
        QString key = group + "_" + gene;

        // Store expression in organized structure
        organizedData[group][gene][bioRep] = expr;

        // Accumulate for mean calculation
        if (!groupGeneMeans.contains(key)) {
            groupGeneMeans[key] = 0.0;
            groupGeneSDs[key] = 0.0;
            groupGeneCounts[key] = 0;
        }

        groupGeneMeans[key] += expr;
        groupGeneCounts[key]++;
    }

    // Calculate mean
    for (auto it = groupGeneMeans.begin(); it != groupGeneMeans.end(); ++it) {
        QString key = it.key();
        groupGeneMeans[key] = groupGeneCounts[key] > 0 ? groupGeneMeans[key] / groupGeneCounts[key] : 0.0;
    }

    // Calculate SD
    for (int i = 0; i < groups.size(); ++i) {
        QString group = groups[i].toString();
        QString gene = genes[i].toString();
        double expr = expressions[i].toDouble();
        QString key = group + "_" + gene;
        double mean = groupGeneMeans[key];

        double diff = expr - mean;
        groupGeneSDs[key] += diff * diff;
    }

    for (auto it = groupGeneSDs.begin(); it != groupGeneSDs.end(); ++it) {
        QString key = it.key();
        int n = groupGeneCounts[key];
        groupGeneSDs[key] = n > 1 ? std::sqrt(groupGeneSDs[key] / (n - 1)) : 0.0;
    }

    // Create raw data table in sorted order: Gene -> Group -> BioRep
    DataFrame rawData;
    QVector<QVariant> rawGenes, rawGroups, rawBioReps, rawExpressions, rawMeans, rawSDs;

    for (const QString& gene : sortedGenes) {
        for (const QString& group : sortedGroups) {
            // Get all BioReps for this group-gene combination and sort them
            if (!organizedData.contains(group) || !organizedData[group].contains(gene)) {
                continue;
            }

            QList<QString> bioRepsInGroup = organizedData[group][gene].keys();
            std::sort(bioRepsInGroup.begin(), bioRepsInGroup.end());

            QString key = group + "_" + gene;
            double mean = groupGeneMeans[key];
            double sd = groupGeneSDs[key];

            for (const QString& bioRep : bioRepsInGroup) {
                double expr = organizedData[group][gene][bioRep];

                rawGenes.append(gene);
                rawGroups.append(group);
                rawBioReps.append(bioRep);
                rawExpressions.append(expr);
                rawMeans.append(mean);
                rawSDs.append(sd);

                qDebug() << "Raw data (sorted):" << group << gene << bioRep << "expr=" << expr << "mean=" << mean << "sd=" << sd;
            }
        }
    }

    rawData.addColumn("Gene", rawGenes);
    rawData.addColumn("Group", rawGroups);
    rawData.addColumn("BioRep", rawBioReps);
    rawData.addColumn("Expression", rawExpressions);
    rawData.addColumn("Mean", rawMeans);
    rawData.addColumn("SD", rawSDs);

    qDebug() << "Raw data table rows:" << rawData.rowCount();

    result.rawData = rawData;
    return result;
}

//=============================================================================
// Standard Curve Method
//=============================================================================

ExpressionResult ExpressionCalculator::calculateByStandardCurve(
    const StandardCurveParams& params,
    const QString& statMethod)
{
    ExpressionResult result;
    result.method = "Standard Curve";

    // Implementation similar to ΔΔCt but using standard curve
    // to calculate absolute quantities

    Q_UNUSED(statMethod);

    return result;
}

//=============================================================================
// Helper Functions
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
    double targetDeltaCt,
    double controlDeltaCt)
{
    double deltaDeltaCt = targetDeltaCt - controlDeltaCt;
    return std::pow(2.0, -deltaDeltaCt);
}

QVector<double> ExpressionCalculator::removeOutliers(const QVector<double>& values)
{
    if (values.size() < 4) {
        return values;
    }

    // Calculate IQR
    QVector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());

    int n = sorted.size();
    double q1 = sorted[n / 4];
    double q3 = sorted[3 * n / 4];
    double iqr = q3 - q1;

    double lowerBound = q1 - 1.5 * iqr;
    double upperBound = q3 + 1.5 * iqr;

    // Filter outliers
    QVector<double> filtered;
    for (double val : values) {
        if (val >= lowerBound && val <= upperBound) {
            filtered.append(val);
        }
    }

    return filtered;
}

QString ExpressionCalculator::formatSignificance(double pValue)
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

//=============================================================================
// Statistical Tests
//=============================================================================

StatisticalResult ExpressionCalculator::performTTest(
    const QVector<double>& group1,
    const QVector<double>& group2,
    const QString& gene,
    const QString& group1Name,
    const QString& group2Name)
{
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    // Calculate means
    double mean1 = std::accumulate(group1.begin(), group1.end(), 0.0) / group1.size();
    double mean2 = std::accumulate(group2.begin(), group2.end(), 0.0) / group2.size();

    // Calculate standard deviations
    double var1 = 0, var2 = 0;
    for (double val : group1) {
        var1 += (val - mean1) * (val - mean1);
    }
    for (double val : group2) {
        var2 += (val - mean2) * (val - mean2);
    }
    var1 /= (group1.size() - 1);
    var2 /= (group2.size() - 1);

    // Pooled standard error
    double se = std::sqrt(var1 / group1.size() + var2 / group2.size());

    // t-statistic
    if (se > 0) {
        result.tStatistic = (mean1 - mean2) / se;

        // Approximate p-value (two-tailed)
        double absT = std::abs(result.tStatistic);
        if (absT < 1.96) {
            result.pValue = 0.05;
        } else if (absT < 2.58) {
            result.pValue = 0.01;
        } else if (absT < 3.29) {
            result.pValue = 0.001;
        } else {
            result.pValue = 0.0001;
        }
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
    const QVector<double>& group1,
    const QVector<double>& group2,
    const QString& gene,
    const QString& group1Name,
    const QString& group2Name)
{
    StatisticalResult result;
    result.gene = gene;
    result.group1 = group1Name;
    result.group2 = group2Name;

    // Simplified Wilcoxon rank-sum test
    // Combine and rank all values
    QVector<double> combined = group1 + group2;
    QVector<int> ranks;

    for (size_t i = 0; i < combined.size(); ++i) {
        int rank = 1;
        for (size_t j = 0; j < combined.size(); ++j) {
            if (combined[j] < combined[i] ||
                (combined[j] == combined[i] && j < i)) {
                rank++;
            }
        }
        ranks.append(rank);
    }

    // Sum of ranks for group1
    double sumRanks1 = 0;
    for (int i = 0; i < group1.size(); ++i) {
        sumRanks1 += ranks[i];
    }

    result.wilcoxV = sumRanks1;

    // Approximate p-value (simplified)
    // For accurate results, use statistical library
    result.pValue = 0.05; // Placeholder
    result.significance = formatSignificance(result.pValue);

    return result;
}

QVector<StatisticalResult> ExpressionCalculator::performANOVA(
    const DataFrame& data,
    const QString& geneCol,
    const QString& groupCol,
    const QString& valueCol)
{
    QVector<StatisticalResult> results;

    // Get unique genes and groups
    auto genes = data.getStringColumn(geneCol);
    auto groups = data.getStringColumn(groupCol);

    QSet<QString> geneSet, groupSet;
    for (const auto& g : genes) geneSet.insert(g);
    for (const auto& g : groups) groupSet.insert(g);

    // For each gene, perform ANOVA
    for (const QString& gene : geneSet) {
        // Get data for this gene
        DataFrame geneData = data.filter(
            [gene, geneCol](const Row& row) {
                return row.value(geneCol).toString() == gene;
            });

        // Calculate overall mean
        auto values = geneData.getNumericColumn(valueCol);
        double overallMean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();

        // Calculate between-group and within-group variance
        double ssBetween = 0, ssWithin = 0;
        int nTotal = values.size();
        int nGroups = groupSet.size();

        for (const QString& group : groupSet) {
            DataFrame groupData = geneData.filter(
                [group, groupCol](const Row& row) {
                    return row.value(groupCol).toString() == group;
                });

            auto groupValues = groupData.getNumericColumn(valueCol);
            double groupMean = std::accumulate(groupValues.begin(), groupValues.end(), 0.0) / groupValues.size();

            ssBetween += groupValues.size() * (groupMean - overallMean) * (groupMean - overallMean);

            for (double val : groupValues) {
                ssWithin += (val - groupMean) * (val - groupMean);
            }
        }

        // F-statistic
        double dfBetween = nGroups - 1;
        double dfWithin = nTotal - nGroups;

        double msBetween = ssBetween / dfBetween;
        double msWithin = ssWithin / dfWithin;

        double fStat = msWithin > 0 ? msBetween / msWithin : qQNaN();

        // Approximate p-value
        double pValue = 0.05; // Placeholder - use F-distribution for accuracy

        StatisticalResult result;
        result.gene = gene;
        result.fStatistic = fStat;
        result.pValue = pValue;
        result.significance = formatSignificance(pValue);

        results.append(result);
    }

    return results;
}

/**
 * @brief 生成ANOVA字母标记
 *
 * 根据Tukey HSD结果为各组分配字母标记
 * 相同字母表示无显著差异，不同字母表示有显著差异
 *
 * @param groupMeans 各组均值 (group -> mean)
 * @param tukeyResults Tukey HSD检验结果
 * @param alpha 显著性水平
 * @return 各组对应的字母标记 (group -> letters)
 */
QHash<QString, QString> ExpressionCalculator::generateLetterGroups(
    const QHash<QString, double>& groupMeans,
    const QVector<TestResult>& tukeyResults,
    double alpha)
{
    QHash<QString, QString> letterGroups;

    if (groupMeans.isEmpty()) return letterGroups;

    // 将组按均值排序（从高到低）
    QList<QPair<QString, double>> sortedGroups;
    for (auto it = groupMeans.begin(); it != groupMeans.end(); ++it) {
        sortedGroups.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sortedGroups.begin(), sortedGroups.end(),
        [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
            return a.second > b.second; // 降序排列
        });

    // 初始化：所有组都没有字母
    QStringList groups;
    for (const auto& pair : sortedGroups) {
        groups.append(pair.first);
        letterGroups[pair.first] = "";
    }

    // 构建显著性矩阵：sigMatrix[i][j] = true 表示组i和组j有显著差异
    QHash<QString, QHash<QString, bool>> sigMatrix;
    for (const QString& g1 : groups) {
        for (const QString& g2 : groups) {
            sigMatrix[g1][g2] = false;
        }
    }

    // 从Tukey HSD结果填充显著性矩阵
    for (const TestResult& result : tukeyResults) {
        // result.testName格式: "Tukey HSD: group1 vs group2"
        QString testName = result.testName;
        if (testName.contains(" vs ")) {
            int vsPos = testName.indexOf(" vs ");
            // "Tukey HSD: " 长度为10，跳过空格后是位置11
            QString group1 = testName.mid(11, vsPos - 11); // 跳过"Tukey HSD: "后的空格
            QString group2 = testName.mid(vsPos + 4); // " vs " 之后

            // 去除可能的前后空格
            group1 = group1.trimmed();
            group2 = group2.trimmed();

            bool isSignificant = result.pValue < alpha;
            qDebug() << "    SigMatrix: group1=" << group1 << "group2=" << group2
                     << "isSignificant=" << isSignificant;
            if (sigMatrix.contains(group1) && sigMatrix.contains(group2)) {
                sigMatrix[group1][group2] = isSignificant;
                sigMatrix[group2][group1] = isSignificant;
            }
        }
    }

    qDebug() << "  Groups sorted by mean:" << groups;
    qDebug() << "  Assigning letters...";

    // 分配字母
    int letterIndex = 0;
    QChar currentLetter = 'a';

    for (int i = 0; i < groups.size(); ++i) {
        QString currentGroup = groups[i];

        qDebug() << "  Processing group" << currentGroup << "at index" << i;

        // 如果这个组已经有字母了，说明之前的某个组已经分配给它了字母
        // 我们需要检查是否需要添加新的字母
        bool hasExistingLetter = !letterGroups[currentGroup].isEmpty();

        if (!hasExistingLetter) {
            // 分配新字母给当前组
            letterGroups[currentGroup] = QString(currentLetter);
            qDebug() << "    Assigned letter" << currentLetter << "to" << currentGroup;
        } else {
            qDebug() << "    Group" << currentGroup << "already has letter" << letterGroups[currentGroup];
        }

        // 找出所有与当前组无显著差异的组，分配相同字母
        for (int j = i + 1; j < groups.size(); ++j) {
            QString otherGroup = groups[j];

            bool isSig = sigMatrix[currentGroup][otherGroup];
            qDebug() << "    Checking" << currentGroup << "vs" << otherGroup
                     << ": significant=" << isSig;

            // 如果与当前组无显著差异，添加相同字母
            if (!sigMatrix[currentGroup][otherGroup]) {
                // 给otherGroup添加字母
                if (letterGroups[otherGroup].isEmpty()) {
                    letterGroups[otherGroup] = QString(currentLetter);
                    qDebug() << "      Also assigned" << currentLetter << "to" << otherGroup;
                } else if (!letterGroups[otherGroup].contains(currentLetter)) {
                    letterGroups[otherGroup] += currentLetter;
                    qDebug() << "      Added" << currentLetter << "to" << otherGroup
                             << "->" << letterGroups[otherGroup];
                }

                // 如果当前组原本就有字母（从之前的组继承来的），也需要给自己添加
                if (hasExistingLetter && !letterGroups[currentGroup].contains(currentLetter)) {
                    letterGroups[currentGroup] += currentLetter;
                    qDebug() << "      Added" << currentLetter << "to" << currentGroup
                             << "->" << letterGroups[currentGroup];
                }
            }
        }

        // 只有当前组原本没有字母时，才移动到下一个字母
        if (!hasExistingLetter) {
            letterIndex++;
            currentLetter = QChar('a' + letterIndex);
        }
    }

    return letterGroups;
}

} // namespace qpcr
