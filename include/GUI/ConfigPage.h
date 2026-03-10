#ifndef CONFIGPAGE_H
#define CONFIGPAGE_H

#include <QWizardPage>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>

namespace qpcr {

/**
 * @brief 参数配置向导页面
 *
 * 配置分析参数，如内参基因、对照组、统计方法等
 */
class ConfigPage : public QWizardPage
{
    Q_OBJECT

public:
    enum AnalysisType {
        StandardCurve,
        DeltaCt,
        DeltaDeltaCt,
        RqPCR,
        ReverseTranscription
    };

    explicit ConfigPage(QWidget* parent = nullptr);
    ~ConfigPage();

    void setAnalysisType(AnalysisType type);
    AnalysisType analysisType() const { return m_analysisType; }

    // Parameter getters
    QString referenceGene() const { return m_comboRefGene->currentText(); }
    QString controlGroup() const { return m_comboControlGroup->currentText(); }
    QString statMethod() const { return m_comboStatMethod->currentData().toString(); }
    bool removeOutliers() const { return m_chkRemoveOutliers->isChecked(); }
    QString figureType() const { return m_comboFigureType->currentData().toString(); }

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

private slots:
    void onAnalysisTypeChanged();
    void onReferenceGeneChanged(const QString& gene);
    void onControlGroupChanged(const QString& group);

private:
    void setupUI();
    void setupStandardCurveOptions();
    void setupDeltaCtOptions();
    void setupDeltaDeltaCtOptions();
    void setupRqPCROptions();
    void updateAvailableGenes();
    void updateAvailableGroups();

    // UI Components
    QComboBox* m_comboRefGene;
    QComboBox* m_comboControlGroup;
    QComboBox* m_comboStatMethod;
    QCheckBox* m_chkRemoveOutliers;
    QComboBox* m_comboFigureType;

    // Standard curve specific
    QDoubleSpinBox* m_spinLowestConc;
    QDoubleSpinBox* m_spinHighestConc;
    QDoubleSpinBox* m_spinDilution;
    QCheckBox* m_chkUseMean;

    // RqPCR specific
    QCheckBox* m_chkAutoDetectRef;

    // Info labels
    QLabel* m_infoRefGene;
    QLabel* m_infoControlGroup;
    QLabel* m_infoStatMethod;

    // State
    AnalysisType m_analysisType;
    QStringList m_availableGenes;
    QStringList m_availableGroups;
};

} // namespace qpcr

#endif // CONFIGPAGE_H
