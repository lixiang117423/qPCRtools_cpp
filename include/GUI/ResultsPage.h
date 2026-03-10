#ifndef RESULTSPAGE_H
#define RESULTSPAGE_H

#include <QWizardPage>
#include <QTableWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "Data/DataFrame.h"
#include "Core/ExpressionCalculator.h"
#include "Core/StandardCurve.h"

namespace qpcr {

/**
 * @brief 结果展示向导页面
 *
 * 展示分析结果，包括数据表格和图表
 */
class ResultsPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ResultsPage(QWidget* parent = nullptr);
    ~ResultsPage();

    // Set results
    void setResultTable(const DataFrame& table);
    void setStatistics(const QVector<StatisticalResult>& stats);
    void setStandardCurveResults(const QVector<StandardCurveResult>& results);

    // QWizardPage interface
    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override;

private slots:
    void onExportCSV();
    void onExportExcel();
    void onExportPDF();
    void onExportImage();
    void onTabChanged(int index);

private:
    void setupUI();
    void createSummaryTab();
    void createDataTableTab();
    void createStatisticsTab();
    void createFiguresTab();
    void updateSummary();
    void populateDataTable();
    void populateStatisticsTable();
    void createFigures();

    QString formatPValue(double p);
    QString formatSignificance(const QString& sig);

    // UI Components
    QTabWidget* m_tabWidget;
    QPushButton* m_btnExportCSV;
    QPushButton* m_btnExportExcel;
    QPushButton* m_btnExportPDF;
    QPushButton* m_btnExportImage;

    // Summary tab
    QLabel* m_summaryLabel;
    QLabel* m_methodLabel;
    QLabel* m_sampleCountLabel;
    QLabel* m_geneCountLabel;

    // Data table tab
    QTableWidget* m_dataTable;

    // Statistics tab
    QTableWidget* m_statsTable;

    // Figures tab
    QWidget* m_figuresWidget;
    QVBoxLayout* m_figuresLayout;

    // Data
    DataFrame m_resultTable;
    QVector<StatisticalResult> m_statistics;
    QVector<StandardCurveResult> m_curveResults;

    // State
    bool m_hasResults;
};

} // namespace qpcr

#endif // RESULTSPAGE_H
