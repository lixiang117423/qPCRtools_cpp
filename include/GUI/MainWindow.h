#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>
#include <QStackedWidget>
#include <QWizard>

class QMenu;
class QToolBar;
class QStatusBar;
class QLabel;
class QPushButton;

namespace qpcr {

/**
 * @brief 主窗口类
 *
 * qPCRtools 的主界面，提供向导式工作流程
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File menu
    void onNewAnalysis();
    void onOpenFile();
    void onExportResults();
    void onExportChart();
    void onPrint();
    void onExit();

    // Tools menu
    void onOptions();
    void onLanguageSwitch();

    // Help menu
    void onDocumentation();
    void onAbout();
    void onCheckUpdates();

    // Language
    void switchToEnglish();
    void switchToChinese();

    // Analysis type selection
    void startStandardCurveAnalysis();
    void startDeltaCtAnalysis();
    void startDeltaDeltaCtAnalysis();
    void startRqPCRAnalysis();
    void startReverseTranscription();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void createWelcomeScreen();
    void createAnalysisButtons();

    void retranslateUi();
    void loadSettings();
    void saveSettings();

    // Language
    void installTranslator(const QString& language);

    // UI Components
    QWidget* m_centralWidget;
    QStackedWidget* m_stackedWidget;

    // Welcome screen
    QWidget* m_welcomeWidget;
    QPushButton* m_btnStandardCurve;
    QPushButton* m_btnDeltaCt;
    QPushButton* m_btnDeltaDeltaCt;
    QPushButton* m_btnRqPCR;
    QPushButton* m_btnReverseTranscription;

    // Menu bar
    QMenu* m_menuFile;
    QMenu* m_menuTools;
    QMenu* m_menuLanguage;
    QMenu* m_menuHelp;

    // Actions
    QAction* m_actionNew;
    QAction* m_actionOpen;
    QAction* m_actionExport;
    QAction* m_actionPrint;
    QAction* m_actionExit;
    QAction* m_actionOptions;
    QAction* m_actionDocumentation;
    QAction* m_actionAbout;

    // Status bar
    QLabel* m_statusLabel;
    QLabel* m_languageLabel;

    // Translation
    QTranslator m_translator;
    QString m_currentLanguage;

    // Settings
    QString m_lastOpenPath;
    QString m_lastExportPath;
};

} // namespace qpcr

#endif // MAINWINDOW_H
