#include "GUI/ImportPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QCheckBox>
#include <QSpinBox>

namespace qpcr {

//=============================================================================
// Constructor / Destructor
//=============================================================================

ImportPage::ImportPage(QWidget* parent)
    : QWizardPage(parent)
    , m_cqFileLoaded(false)
    , m_designFileLoaded(false)
{
    setTitle(tr("Import Data"));
    setSubTitle(tr("Select and import your Cq value and design data files"));
    setupUI();
}

ImportPage::~ImportPage()
{
}

//=============================================================================
// QWizardPage interface
//=============================================================================

void ImportPage::initializePage()
{
    // Reset state when page is initialized
    m_cqFileLoaded = false;
    m_designFileLoaded = false;
    m_statusLabel->setText(tr("Please import data files"));
}

bool ImportPage::validatePage()
{
    // Validate that required files are loaded
    if (!m_cqFileLoaded) {
        QMessageBox::warning(this, tr("Missing Data"),
            tr("Please import the Cq value file."));
        return false;
    }

    return true;
}

int ImportPage::nextId() const
{
    // Return the ID of the next wizard page
    return 1; // ConfigPage
}

//=============================================================================
// UI Setup
//=============================================================================

void ImportPage::setupUI()
{
    // Main layout
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Cq file section
    auto* cqGroup = new QGroupBox(tr("Cq Value File"), this);
    auto* cqLayout = new QGridLayout(cqGroup);

    cqLayout->addWidget(new QLabel(tr("File Path:"), this), 0, 0);
    m_edCqFilePath = new QLineEdit(this);
    m_edCqFilePath->setReadOnly(true);
    cqLayout->addWidget(m_edCqFilePath, 0, 1);

    m_btnBrowseCq = new QPushButton(tr("Browse..."), this);
    connect(m_btnBrowseCq, &QPushButton::clicked, this, &ImportPage::onBrowseCqFile);
    cqLayout->addWidget(m_btnBrowseCq, 0, 2);

    cqLayout->addWidget(new QLabel(tr("Format:"), this), 1, 0);
    m_comboCqFormat = new QComboBox(this);
    m_comboCqFormat->addItem("CSV", "csv");
    m_comboCqFormat->addItem("Excel (.xlsx)", "xlsx");
    m_comboCqFormat->addItem("Tab-separated", "tsv");
    connect(m_comboCqFormat, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &ImportPage::onFileFormatChanged);
    cqLayout->addWidget(m_comboCqFormat, 1, 1);

    mainLayout->addWidget(cqGroup);

    // Design file section
    auto* designGroup = new QGroupBox(tr("Design File (Optional)"), this);
    auto* designLayout = new QGridLayout(designGroup);

    designLayout->addWidget(new QLabel(tr("File Path:"), this), 0, 0);
    m_edDesignFilePath = new QLineEdit(this);
    m_edDesignFilePath->setReadOnly(true);
    designLayout->addWidget(m_edDesignFilePath, 0, 1);

    m_btnBrowseDesign = new QPushButton(tr("Browse..."), this);
    connect(m_btnBrowseDesign, &QPushButton::clicked, this, &ImportPage::onBrowseDesignFile);
    designLayout->addWidget(m_btnBrowseDesign, 0, 2);

    designLayout->addWidget(new QLabel(tr("Format:"), this), 1, 0);
    m_comboDesignFormat = new QComboBox(this);
    m_comboDesignFormat->addItem("CSV", "csv");
    m_comboDesignFormat->addItem("Excel (.xlsx)", "xlsx");
    m_comboDesignFormat->addItem("Tab-separated", "tsv");
    designLayout->addWidget(m_comboDesignFormat, 1, 1);

    mainLayout->addWidget(designGroup);

    // Import options
    auto* optionsGroup = new QGroupBox(tr("Import Options"), this);
    auto* optionsLayout = new QHBoxLayout(optionsGroup);

    m_chkHasHeader = new QCheckBox(tr("File has header row"), this);
    m_chkHasHeader->setChecked(true);
    optionsLayout->addWidget(m_chkHasHeader);

    optionsLayout->addSpacing(20);

    optionsLayout->addWidget(new QLabel(tr("Preview rows:"), this));
    m_spinPreviewRows = new QSpinBox(this);
    m_spinPreviewRows->setRange(5, 100);
    m_spinPreviewRows->setValue(10);
    optionsLayout->addWidget(m_spinPreviewRows);

    optionsLayout->addStretch();
    mainLayout->addWidget(optionsGroup);

    // Column mapping section
    auto* mappingGroup = new QGroupBox(tr("Column Mapping"), this);
    auto* mappingLayout = new QGridLayout(mappingGroup);

    mappingLayout->addWidget(new QLabel(tr("Position Column:"), this), 0, 0);
    m_comboPositionCol = new QComboBox(this);
    mappingLayout->addWidget(m_comboPositionCol, 0, 1);

    mappingLayout->addWidget(new QLabel(tr("Gene Column:"), this), 0, 2);
    m_comboGeneCol = new QComboBox(this);
    mappingLayout->addWidget(m_comboGeneCol, 0, 3);

    mappingLayout->addWidget(new QLabel(tr("Cq Column:"), this), 1, 0);
    m_comboCqCol = new QComboBox(this);
    mappingLayout->addWidget(m_comboCqCol, 1, 1);

    mappingLayout->addWidget(new QLabel(tr("Group Column:"), this), 1, 2);
    m_comboGroupCol = new QComboBox(this);
    mappingLayout->addWidget(m_comboGroupCol, 1, 3);

    mappingLayout->addWidget(new QLabel(tr("BioRep Column:"), this), 2, 0);
    m_comboBioRepCol = new QComboBox(this);
    mappingLayout->addWidget(m_comboBioRepCol, 2, 1);

    auto* btnAutoDetect = new QPushButton(tr("Auto-Detect Columns"), this);
    connect(btnAutoDetect, &QPushButton::clicked, this, &ImportPage::onAutoDetectColumns);
    mappingLayout->addWidget(btnAutoDetect, 2, 2, 1, 2);

    mainLayout->addWidget(mappingGroup);

    // Preview section
    auto* previewGroup = new QGroupBox(tr("Data Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);

    m_previewLabel = new QLabel(tr("No data loaded"), this);
    previewLayout->addWidget(m_previewLabel);

    m_tablePreview = new QTableWidget(this);
    m_tablePreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablePreview->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tablePreview->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tablePreview->horizontalHeader()->setStretchLastSection(true);
    previewLayout->addWidget(m_tablePreview);

    mainLayout->addWidget(previewGroup, 1);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // Register fields for wizard
    registerField("cqTable*", this, "cqTable", SIGNAL(cqTableChanged()));
}

//=============================================================================
// Slots
//=============================================================================

void ImportPage::onBrowseCqFile()
{
    QString filter = tr("Data Files (*.csv *.txt *.xlsx);;CSV Files (*.csv);;Excel Files (*.xlsx);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Cq Value File"), "", filter);

    if (!fileName.isEmpty()) {
        m_edCqFilePath->setText(fileName);

        // Auto-detect format
        QString format = detectFileFormat(fileName);
        if (format == "csv") {
            m_comboCqFormat->setCurrentIndex(0);
        } else if (format == "xlsx") {
            m_comboCqFormat->setCurrentIndex(1);
        } else if (format == "tsv") {
            m_comboCqFormat->setCurrentIndex(2);
        }

        // Import file
        if (importFile(fileName, m_cqTable)) {
            m_cqFileLoaded = true;
            loadFilePreview();
            onAutoDetectColumns();
            m_statusLabel->setText(tr("Cq file loaded: %1 rows, %2 columns")
                .arg(m_cqTable.rowCount())
                .arg(m_cqTable.columnCount()));
        } else {
            m_statusLabel->setText(tr("Failed to load Cq file"));
        }
    }
}

void ImportPage::onBrowseDesignFile()
{
    QString filter = tr("Data Files (*.csv *.txt *.xlsx);;CSV Files (*.csv);;Excel Files (*.xlsx);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Design File"), "", filter);

    if (!fileName.isEmpty()) {
        m_edDesignFilePath->setText(fileName);

        if (importFile(fileName, m_designTable)) {
            m_designFileLoaded = true;
            m_statusLabel->setText(tr("Design file loaded: %1 rows, %2 columns")
                .arg(m_designTable.rowCount())
                .arg(m_designTable.columnCount()));
        } else {
            m_statusLabel->setText(tr("Failed to load design file"));
        }
    }
}

void ImportPage::onFileFormatChanged()
{
    // Update file extension in path
    // This is a placeholder for future functionality
}

void ImportPage::onRefreshPreview()
{
    loadFilePreview();
}

void ImportPage::onAutoDetectColumns()
{
    if (!m_cqFileLoaded) return;

    // Clear combo boxes
    m_comboPositionCol->clear();
    m_comboGeneCol->clear();
    m_comboCqCol->clear();
    m_comboGroupCol->clear();
    m_comboBioRepCol->clear();

    // Add columns from data
    QStringList columns = m_cqTable.columns();
    for (const QString& col : columns) {
        m_comboPositionCol->addItem(col);
        m_comboGeneCol->addItem(col);
        m_comboCqCol->addItem(col);
        m_comboGroupCol->addItem(col);
        m_comboBioRepCol->addItem(col);
    }

    // Auto-detect based on common column names
    for (int i = 0; i < columns.size(); ++i) {
        QString col = columns[i].toLower();

        if (col.contains("position") || col.contains("pos") || col.contains("well")) {
            m_comboPositionCol->setCurrentIndex(i);
        } else if (col.contains("gene") || col.contains("target") || col.contains("name")) {
            m_comboGeneCol->setCurrentIndex(i);
        } else if (col.contains("cq") || col.contains("ct") || col.contains("value")) {
            m_comboCqCol->setCurrentIndex(i);
        } else if (col.contains("group") || col.contains("treatment") || col.contains("sample")) {
            m_comboGroupCol->setCurrentIndex(i);
        } else if (col.contains("biorep") || col.contains("rep") || col.contains("replicate")) {
            m_comboBioRepCol->setCurrentIndex(i);
        }
    }

    m_statusLabel->setText(tr("Columns auto-detected. Please verify the mapping."));
}

//=============================================================================
// Private methods
//=============================================================================

void ImportPage::loadFilePreview()
{
    if (!m_cqFileLoaded) return;

    m_previewData = m_cqTable;
    int previewRows = qMin(m_spinPreviewRows->value(), m_previewData.rowCount());

    // Setup table
    m_tablePreview->setColumnCount(m_previewData.columnCount());
    m_tablePreview->setRowCount(previewRows);

    // Set headers
    m_tablePreview->setHorizontalHeaderLabels(m_previewData.columns());

    // Fill data
    for (int row = 0; row < previewRows; ++row) {
        for (int col = 0; col < m_previewData.columnCount(); ++col) {
            QString colName = m_previewData.columns().at(col);
            QVariant value = m_previewData.get(row, colName);
            QTableWidgetItem* item = new QTableWidgetItem(value.toString());
            m_tablePreview->setItem(row, col, item);
        }
    }

    m_previewLabel->setText(tr("Previewing first %1 rows of %2 total rows")
        .arg(previewRows)
        .arg(m_previewData.rowCount()));
}

bool ImportPage::importFile(const QString& path, DataFrame& table)
{
    QString format = m_comboCqFormat->currentData().toString();
    bool hasHeader = m_chkHasHeader->isChecked();

    try {
        if (format == "csv") {
            table = DataFrame::fromCSV(path, hasHeader);
        } else if (format == "xlsx") {
            table = DataFrame::fromExcel(path, 0, hasHeader);
        } else if (format == "tsv") {
            // For TSV, we need to set tab separator
            // This is a simplified version
            table = DataFrame::fromCSV(path, hasHeader);
        }

        return !table.isEmpty();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Import Error"),
            tr("Failed to import file:\n%1").arg(e.what()));
        return false;
    }
}

QString ImportPage::detectFileFormat(const QString& path)
{
    if (path.endsWith(".xlsx", Qt::CaseInsensitive)) {
        return "xlsx";
    } else if (path.endsWith(".csv", Qt::CaseInsensitive)) {
        return "csv";
    } else if (path.endsWith(".txt", Qt::CaseInsensitive) ||
               path.endsWith(".tsv", Qt::CaseInsensitive)) {
        return "tsv";
    }
    return "csv"; // Default
}

void ImportPage::updateColumnComboBoxes()
{
    // Update column combo boxes based on loaded data
    if (m_cqFileLoaded) {
        onAutoDetectColumns();
    }
}

} // namespace qpcr
