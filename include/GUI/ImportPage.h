#ifndef IMPORTPAGE_H
#define IMPORTPAGE_H

#include <QWizardPage>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include "Data/DataFrame.h"

namespace qpcr {

/**
 * @brief 数据导入向导页面
 *
 * 用于导入 Cq 值和设计数据文件
 */
class ImportPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ImportPage(QWidget* parent = nullptr);
    ~ImportPage();

    // 获取导入的数据
    DataFrame cqTable() const { return m_cqTable; }
    DataFrame designTable() const { return m_designTable; }

    // 设置/获取数据
    void setCqTable(const DataFrame& table) { m_cqTable = table; }
    void setDesignTable(const DataFrame& table) { m_designTable = table; }

    // QWizardPage interface
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

private slots:
    void onBrowseCqFile();
    void onBrowseDesignFile();
    void onFileFormatChanged();
    void onRefreshPreview();
    void onAutoDetectColumns();

private:
    void setupUI();
    void loadFilePreview();
    bool importFile(const QString& path, DataFrame& table);
    QString detectFileFormat(const QString& path);
    void updateColumnComboBoxes();

    // UI Components
    QLineEdit* m_edCqFilePath;
    QPushButton* m_btnBrowseCq;
    QComboBox* m_comboCqFormat;

    QLineEdit* m_edDesignFilePath;
    QPushButton* m_btnBrowseDesign;
    QComboBox* m_comboDesignFormat;

    QTableWidget* m_tablePreview;
    QLabel* m_previewLabel;
    QLabel* m_statusLabel;

    // Column mapping
    QComboBox* m_comboPositionCol;
    QComboBox* m_comboGeneCol;
    QComboBox* m_comboCqCol;
    QComboBox* m_comboGroupCol;
    QComboBox* m_comboBioRepCol;

    // Options
    QCheckBox* m_chkHasHeader;
    QSpinBox* m_spinPreviewRows;

    // Data
    DataFrame m_cqTable;
    DataFrame m_designTable;
    DataFrame m_previewData;

    // State
    bool m_cqFileLoaded;
    bool m_designFileLoaded;
};

} // namespace qpcr

#endif // IMPORTPAGE_H
