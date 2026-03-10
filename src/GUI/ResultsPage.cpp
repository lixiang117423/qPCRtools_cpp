#include "GUI/ResultsPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QSplitter>

namespace qpcr {

//=============================================================================
// Constructor / Destructor
//=============================================================================

ResultsPage::ResultsPage(QWidget* parent)
    : QWizardPage(parent)
    , m_hasResults(false)
{
    setTitle(tr("Analysis Results"));
    setSubTitle(tr("View and export your analysis results"));
    setupUI();
}

ResultsPage::~ResultsPage()
{
}

//=============================================================================
// QWizardPage interface
//=============================================================================

void ResultsPage::initializePage()
{
    // Run analysis when page is initialized
    // TODO: Execute analysis based on previous pages' data

    m_hasResults = true;
    emit completeChanged();
}

bool ResultsPage::isComplete() const
{
    return m_hasResults;
}

int ResultsPage::nextId() const
{
    return -1; // Last page
}

//=============================================================================
// UI Setup
//=============================================================================

void ResultsPage::setupUI()
{
    // Main layout
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Export buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_btnExportCSV = new QPushButton(tr("Export CSV"), this);
    m_btnExportCSV->setToolTip(tr("Export results to CSV file"));
    connect(m_btnExportCSV, &QPushButton::clicked, this, &ResultsPage::onExportCSV);
    buttonLayout->addWidget(m_btnExportCSV);

    m_btnExportExcel = new QPushButton(tr("Export Excel"), this);
    m_btnExportExcel->setToolTip(tr("Export results to Excel file"));
    connect(m_btnExportExcel, &QPushButton::clicked, this, &ResultsPage::onExportExcel);
    buttonLayout->addWidget(m_btnExportExcel);

    m_btnExportPDF = new QPushButton(tr("Export PDF"), this);
    m_btnExportPDF->setToolTip(tr("Export report as PDF"));
    connect(m_btnExportPDF, &QPushButton::clicked, this, &ResultsPage::onExportPDF);
    buttonLayout->addWidget(m_btnExportPDF);

    m_btnExportImage = new QPushButton(tr("Export Chart"), this);
    m_btnExportImage->setToolTip(tr("Export chart as image"));
    connect(m_btnExportImage, &QPushButton::clicked, this, &ResultsPage::onExportImage);
    buttonLayout->addWidget(m_btnExportImage);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &ResultsPage::onTabChanged);
    mainLayout->addWidget(m_tabWidget);

    // Create tabs
    createSummaryTab();
    createDataTableTab();
    createStatisticsTab();
    createFiguresTab();
}

void ResultsPage::createSummaryTab()
{
    auto* summaryWidget = new QWidget();
    auto* layout = new QVBoxLayout(summaryWidget);
    layout->setSpacing(15);

    // Analysis method
    m_methodLabel = new QLabel(tr("Analysis Method: 2^-ΔΔCt"), summaryWidget);
    QFont font = m_methodLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    m_methodLabel->setFont(font);
    layout->addWidget(m_methodLabel);

    layout->addSpacing(10);

    // Summary information
    auto* infoGroup = new QGroupBox(tr("Summary"), summaryWidget);
    auto* infoLayout = new QFormLayout(infoGroup);

    m_sampleCountLabel = new QLabel(tr("N/A"), summaryWidget);
    infoLayout->addRow(tr("Total Samples:"), m_sampleCountLabel);

    m_geneCountLabel = new QLabel(tr("N/A"), summaryWidget);
    infoLayout->addRow(tr("Number of Genes:"), m_geneCountLabel);

    layout->addWidget(infoGroup);

    // Additional summary
    m_summaryLabel = new QLabel(summaryWidget);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setAlignment(Qt::AlignTop);
    layout->addWidget(m_summaryLabel, 1);

    m_tabWidget->addTab(summaryWidget, tr("Summary"));
}

void ResultsPage::createDataTableTab()
{
    auto* dataWidget = new QWidget();
    auto* layout = new QVBoxLayout(dataWidget);

    m_dataTable = new QTableWidget(dataWidget);
    m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dataTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_dataTable->horizontalHeader()->setStretchLastSection(true);
    m_dataTable->setAlternatingRowColors(true);

    layout->addWidget(m_dataTable);
    m_tabWidget->addTab(dataWidget, tr("Data Table"));
}

void ResultsPage::createStatisticsTab()
{
    auto* statsWidget = new QWidget();
    auto* layout = new QVBoxLayout(statsWidget);

    m_statsTable = new QTableWidget(statsWidget);
    m_statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_statsTable->horizontalHeader()->setStretchLastSection(true);
    m_statsTable->setAlternatingRowColors(true);

    // Set headers
    m_statsTable->setColumnCount(6);
    m_statsTable->setHorizontalHeaderLabels({
        tr("Gene"), tr("Group"), tr("Statistic"), tr("P-value"), tr("Significance"), tr("Note")
    });

    layout->addWidget(m_statsTable);
    m_tabWidget->addTab(statsWidget, tr("Statistics"));
}

void ResultsPage::createFiguresTab()
{
    m_figuresWidget = new QWidget();
    m_figuresLayout = new QVBoxLayout(m_figuresWidget);
    m_figuresLayout->setSpacing(20);

    // Placeholder for figures
    auto* placeholder = new QLabel(tr("Figures will be displayed here"), m_figuresWidget);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("color: #999; font-size: 14pt;");
    m_figuresLayout->addWidget(placeholder, 1);

    m_tabWidget->addTab(m_figuresWidget, tr("Figures"));
}

//=============================================================================
// Data Population
//=============================================================================

void ResultsPage::setResultTable(const DataFrame& table)
{
    m_resultTable = table;
    populateDataTable();
    updateSummary();
}

void ResultsPage::setStatistics(const QVector<StatisticalResult>& stats)
{
    m_statistics = stats;
    populateStatisticsTable();
}

void ResultsPage::setStandardCurveResults(const QVector<StandardCurveResult>& results)
{
    m_curveResults = results;
}

void ResultsPage::updateSummary()
{
    if (m_resultTable.isEmpty()) {
        return;
    }

    // Update counts
    int sampleCount = m_resultTable.rowCount();
    m_sampleCountLabel->setText(QString::number(sampleCount));

    // Count unique genes
    QSet<QString> genes;
    auto geneCol = m_resultTable.getStringColumn("Gene");
    for (const auto& gene : geneCol) {
        genes.insert(gene);
    }
    m_geneCountLabel->setText(QString::number(genes.size()));

    // Build summary text
    QString summary = tr("<h3>Analysis Complete</h3>"
                        "<p>The analysis has been successfully completed with the following results:</p>"
                        "<ul>"
                        "<li>Total samples analyzed: <b>%1</b></li>"
                        "<li>Number of genes: <b>%2</b></li>"
                        "<li>Statistical tests performed: <b>%3</b></li>"
                        "</ul>"
                        "<p>You can now review the results in the tabs above and export them as needed.</p>")
                        .arg(sampleCount)
                        .arg(genes.size())
                        .arg(m_statistics.size());

    m_summaryLabel->setText(summary);
}

void ResultsPage::populateDataTable()
{
    if (m_resultTable.isEmpty()) {
        return;
    }

    // Setup table
    QStringList columns = m_resultTable.columns();
    m_dataTable->setColumnCount(columns.size());
    m_dataTable->setRowCount(m_resultTable.rowCount());
    m_dataTable->setHorizontalHeaderLabels(columns);

    // Fill data
    for (int row = 0; row < m_resultTable.rowCount(); ++row) {
        for (int col = 0; col < columns.size(); ++col) {
            QString colName = columns[col];
            QVariant value = m_resultTable.get(row, colName);

            QTableWidgetItem* item = new QTableWidgetItem(value.toString());

            // Format numeric values
            if (colName.contains("Expression", Qt::CaseInsensitive)) {
                double num = value.toDouble();
                item->setText(QString::number(num, 'f', 4));
            }

            m_dataTable->setItem(row, col, item);
        }
    }

    // Resize columns
    m_dataTable->resizeColumnsToContents();
}

void ResultsPage::populateStatisticsTable()
{
    if (m_statistics.isEmpty()) {
        return;
    }

    m_statsTable->setRowCount(m_statistics.size());

    for (int i = 0; i < m_statistics.size(); ++i) {
        const auto& stat = m_statistics[i];

        m_statsTable->setItem(i, 0, new QTableWidgetItem(stat.gene));
        m_statsTable->setItem(i, 1, new QTableWidgetItem(stat.group));
        m_statsTable->setItem(i, 2, new QTableWidgetItem(QString::number(stat.statistic, 'f', 4)));
        m_statsTable->setItem(i, 3, new QTableWidgetItem(formatPValue(stat.pValue)));
        m_statsTable->setItem(i, 4, new QTableWidgetItem(stat.significance));

        // Add note
        QString note;
        if (stat.isSignificant) {
            note = tr("Significant at p < 0.05");
        } else {
            note = tr("Not significant");
        }
        m_statsTable->setItem(i, 5, new QTableWidgetItem(note));
    }

    m_statsTable->resizeColumnsToContents();
}

void ResultsPage::createFigures()
{
    // Clear placeholder
    m_figuresLayout->clear();

    // TODO: Create and add chart widgets
    // This will be implemented with the plot widgets
}

//=============================================================================
// Slots
//=============================================================================

void ResultsPage::onExportCSV()
{
    if (m_resultTable.isEmpty()) {
        QMessageBox::information(this, tr("No Data"),
            tr("No results to export."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Results as CSV"),
        QString(),
        tr("CSV Files (*.csv)"));

    if (!fileName.isEmpty()) {
        if (m_resultTable.saveCSV(fileName)) {
            QMessageBox::information(this, tr("Success"),
                tr("Results exported successfully to:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Export Failed"),
                tr("Failed to export results."));
        }
    }
}

void ResultsPage::onExportExcel()
{
    if (m_resultTable.isEmpty()) {
        QMessageBox::information(this, tr("No Data"),
            tr("No results to export."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Results as Excel"),
        QString(),
        tr("Excel Files (*.xlsx)"));

    if (!fileName.isEmpty()) {
        if (m_resultTable.saveExcel(fileName)) {
            QMessageBox::information(this, tr("Success"),
                tr("Results exported successfully to:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Export Failed"),
                tr("Failed to export results. Excel support may not be enabled."));
        }
    }
}

void ResultsPage::onExportPDF()
{
    // TODO: Implement PDF export
    QMessageBox::information(this, tr("Coming Soon"),
        tr("PDF export will be implemented soon."));
}

void ResultsPage::onExportImage()
{
    // TODO: Implement chart export
    QMessageBox::information(this, tr("Coming Soon"),
        tr("Chart export will be implemented soon."));
}

void ResultsPage::onTabChanged(int index)
{
    Q_UNUSED(index);
    // Load/create content when tab is changed
}

//=============================================================================
// Helper methods
//=============================================================================

QString ResultsPage::formatPValue(double p)
{
    if (std::isnan(p)) {
        return "N/A";
    } else if (p < 0.001) {
        return "< 0.001";
    } else if (p < 0.01) {
        return QString::number(p, 'f', 3);
    } else if (p < 0.05) {
        return QString::number(p, 'f', 3);
    } else {
        return QString::number(p, 'f', 3);
    }
}

QString ResultsPage::formatSignificance(const QString& sig)
{
    return sig;
}

} // namespace qpcr
