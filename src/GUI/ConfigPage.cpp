#include "GUI/ConfigPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QMessageBox>

namespace qpcr {

//=============================================================================
// Constructor / Destructor
//=============================================================================

ConfigPage::ConfigPage(QWidget* parent)
    : QWizardPage(parent)
    , m_analysisType(DeltaDeltaCt)
{
    setTitle(tr("Configure Analysis Parameters"));
    setSubTitle(tr("Set up the parameters for your analysis"));
    setupUI();
}

ConfigPage::~ConfigPage()
{
}

//=============================================================================
// QWizardPage interface
//=============================================================================

void ConfigPage::initializePage()
{
    // Get data from previous page
    auto* wizard = this->wizard();
    // TODO: Extract genes and groups from imported data
    updateAvailableGenes();
    updateAvailableGroups();

    // Set default values
    if (!m_availableGroups.isEmpty()) {
        m_comboControlGroup->setCurrentIndex(0);
    }
}

bool ConfigPage::validatePage()
{
    // Validate parameters based on analysis type
    if (m_analysisType == DeltaCt ||
        m_analysisType == DeltaDeltaCt ||
        m_analysisType == RqPCR) {

        if (m_comboRefGene->currentText().isEmpty()) {
            QMessageBox::warning(this, tr("Missing Parameter"),
                tr("Please select a reference gene."));
            return false;
        }
    }

    if (m_analysisType == DeltaDeltaCt ||
        m_analysisType == RqPCR) {

        if (m_comboControlGroup->currentText().isEmpty()) {
            QMessageBox::warning(this, tr("Missing Parameter"),
                tr("Please select a control group."));
            return false;
        }
    }

    return true;
}

int ConfigPage::nextId() const
{
    return 2; // ResultsPage
}

//=============================================================================
// UI Setup
//=============================================================================

void ConfigPage::setupUI()
{
    // Main layout
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // Common parameters group
    auto* commonGroup = new QGroupBox(tr("Common Parameters"), this);
    auto* commonLayout = new QFormLayout(commonGroup);

    // Reference gene
    m_comboRefGene = new QComboBox(this);
    m_comboRefGene->setEditable(true);
    connect(m_comboRefGene, &QComboBox::currentTextChanged,
        this, &ConfigPage::onReferenceGeneChanged);
    commonLayout->addRow(tr("Reference Gene:"), m_comboRefGene);

    m_infoRefGene = new QLabel(tr(""), this);
    m_infoRefGene->setWordWrap(true);
    m_infoRefGene->setStyleSheet("color: #666; font-size: 9pt;");
    commonLayout->addRow("", m_infoRefGene);

    // Control group
    m_comboControlGroup = new QComboBox(this);
    m_comboControlGroup->setEditable(true);
    connect(m_comboControlGroup, &QComboBox::currentTextChanged,
        this, &ConfigPage::onControlGroupChanged);
    commonLayout->addRow(tr("Control Group:"), m_comboControlGroup);

    m_infoControlGroup = new QLabel(tr(""), this);
    m_infoControlGroup->setWordWrap(true);
    m_infoControlGroup->setStyleSheet("color: #666; font-size: 9pt;");
    commonLayout->addRow("", m_infoControlGroup);

    mainLayout->addWidget(commonGroup);

    // Statistical analysis group
    auto* statsGroup = new QGroupBox(tr("Statistical Analysis"), this);
    auto* statsLayout = new QFormLayout(statsGroup);

    // Statistical method
    m_comboStatMethod = new QComboBox(this);
    m_comboStatMethod->addItem("t-test", "t.test");
    m_comboStatMethod->addItem("Wilcoxon test", "wilcox.test");
    m_comboStatMethod->addItem("ANOVA", "anova");
    statsLayout->addRow(tr("Statistical Method:"), m_comboStatMethod);

    m_infoStatMethod = new QLabel(
        tr("t-test: Compare each group to control\n"
           "Wilcoxon: Non-parametric alternative\n"
           "ANOVA: Compare all groups with post-hoc test"),
        this);
    m_infoStatMethod->setWordWrap(true);
    m_infoStatMethod->setStyleSheet("color: #666; font-size: 9pt;");
    statsLayout->addRow("", m_infoStatMethod);

    // Remove outliers
    m_chkRemoveOutliers = new QCheckBox(tr("Remove outliers using IQR method"), this);
    m_chkRemoveOutliers->setChecked(true);
    statsLayout->addRow("", m_chkRemoveOutliers);

    mainLayout->addWidget(statsGroup);

    // Figure options group
    auto* figureGroup = new QGroupBox(tr("Figure Options"), this);
    auto* figureLayout = new QFormLayout(figureGroup);

    m_comboFigureType = new QComboBox(this);
    m_comboFigureType->addItem("Box plot", "box");
    m_comboFigureType->addItem("Bar plot", "bar");
    figureLayout->addRow(tr("Figure Type:"), m_comboFigureType);

    mainLayout->addWidget(figureGroup);

    // Standard curve options (hidden by default)
    auto* curveGroup = new QGroupBox(tr("Standard Curve Options"), this);
    auto* curveLayout = new QFormLayout(curveGroup);

    m_spinLowestConc = new QDoubleSpinBox(this);
    m_spinLowestConc->setRange(0, 1e10);
    m_spinLowestConc->setValue(4);
    m_spinLowestConc->setDecimals(0);
    curveLayout->addRow(tr("Lowest Concentration:"), m_spinLowestConc);

    m_spinHighestConc = new QDoubleSpinBox(this);
    m_spinHighestConc->setRange(0, 1e10);
    m_spinHighestConc->setValue(4096);
    m_spinHighestConc->setDecimals(0);
    curveLayout->addRow(tr("Highest Concentration:"), m_spinHighestConc);

    m_spinDilution = new QDoubleSpinBox(this);
    m_spinDilution->setRange(1, 100);
    m_spinDilution->setValue(4);
    m_spinDilution->setSingleStep(0.5);
    curveLayout->addRow(tr("Dilution Factor:"), m_spinDilution);

    m_chkUseMean = new QCheckBox(tr("Use mean Cq values for standard curve"), this);
    m_chkUseMean->setChecked(true);
    curveLayout->addRow("", m_chkUseMean);

    mainLayout->addWidget(curveGroup);
    curveGroup->hide(); // Hidden by default

    // Register fields
    registerField("referenceGene*", m_comboRefGene, "currentText", "currentTextChanged");
    registerField("controlGroup*", m_comboControlGroup, "currentText", "currentTextChanged");
}

void ConfigPage::setupStandardCurveOptions()
{
    // Show standard curve specific options
}

void ConfigPage::setupDeltaCtOptions()
{
    // DeltaCt doesn't need control group
}

void ConfigPage::setupDeltaDeltaCtOptions()
{
    // DeltaDeltaCt requires control group
}

void ConfigPage::setupRqPCROptions()
{
    // RqPCR specific options
}

void ConfigPage::updateAvailableGenes()
{
    // TODO: Get genes from imported data
    m_availableGenes << "OsUBQ" << "Actin" << "GAPDH";
    m_comboRefGene->clear();
    m_comboRefGene->addItems(m_availableGenes);
}

void ConfigPage::updateAvailableGroups()
{
    // TODO: Get groups from imported data
    m_availableGroups << "CK" << "Treatment1" << "Treatment2";
    m_comboControlGroup->clear();
    m_comboControlGroup->addItems(m_availableGroups);
}

//=============================================================================
// Slots
//=============================================================================

void ConfigPage::onAnalysisTypeChanged()
{
    // Update UI based on analysis type
    switch (m_analysisType) {
    case StandardCurve:
        setupStandardCurveOptions();
        break;
    case DeltaCt:
        setupDeltaCtOptions();
        break;
    case DeltaDeltaCt:
        setupDeltaDeltaCtOptions();
        break;
    case RqPCR:
        setupRqPCROptions();
        break;
    default:
        break;
    }
}

void ConfigPage::onReferenceGeneChanged(const QString& gene)
{
    m_infoRefGene->setText(tr("Selected: %1").arg(gene));
}

void ConfigPage::onControlGroupChanged(const QString& group)
{
    m_infoControlGroup->setText(tr("Selected: %1").arg(group));
}

void ConfigPage::setAnalysisType(AnalysisType type)
{
    m_analysisType = type;
    onAnalysisTypeChanged();
}

} // namespace qpcr
