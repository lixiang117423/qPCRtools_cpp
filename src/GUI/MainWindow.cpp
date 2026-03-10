#include "GUI/MainWindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QSettings>
#include <QAction>
#include <QPixmap>
#include <QSplitter>

namespace qpcr {

//=============================================================================
// Constructor / Destructor
//=============================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_currentLanguage("en")
{
    loadSettings();
    setupUI();
    retranslateUi();

    // Show welcome screen by default
    m_stackedWidget->setCurrentWidget(m_welcomeWidget);

    statusMessage()->setText(tr("Welcome to qPCRtools"));
}

MainWindow::~MainWindow()
{
    saveSettings();
}

//=============================================================================
// Event handlers
//=============================================================================

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save window geometry and state
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

//=============================================================================
// UI Setup
//=============================================================================

void MainWindow::setupUI()
{
    // Set window properties
    setWindowTitle("qPCRtools");
    resize(1000, 700);
    setMinimumSize(800, 600);

    // Create central widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Create stacked widget for different screens
    m_stackedWidget = new QStackedWidget(m_centralWidget);

    // Create welcome screen
    createWelcomeScreen();

    // Setup layout
    auto* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_stackedWidget);

    // Setup menu bar, toolbar, status bar
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // Restore window geometry
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::setupMenuBar()
{
    // File menu
    m_menuFile = menuBar()->addMenu(tr("&File"));

    m_actionNew = m_menuFile->addAction(tr("&New Analysis"));
    m_actionNew->setShortcut(QKeySequence::New);
    m_actionNew->setStatusTip(tr("Start a new analysis"));
    connect(m_actionNew, &QAction::triggered, this, &MainWindow::onNewAnalysis);

    m_actionOpen = m_menuFile->addAction(tr("&Open File..."));
    m_actionOpen->setShortcut(QKeySequence::Open);
    m_actionOpen->setStatusTip(tr("Open a data file"));
    connect(m_actionOpen, &QAction::triggered, this, &MainWindow::onOpenFile);

    m_menuFile->addSeparator();

    m_actionExport = m_menuFile->addAction(tr("&Export Results..."));
    m_actionExport->setShortcut(QKeySequence("Ctrl+E"));
    m_actionExport->setStatusTip(tr("Export analysis results"));
    connect(m_actionExport, &QAction::triggered, this, &MainWindow::onExportResults);

    m_actionPrint = m_menuFile->addAction(tr("&Print..."));
    m_actionPrint->setShortcut(QKeySequence::Print);
    m_actionPrint->setStatusTip(tr("Print results"));
    connect(m_actionPrint, &QAction::triggered, this, &MainWindow::onPrint);

    m_menuFile->addSeparator();

    m_actionExit = m_menuFile->addAction(tr("E&xit"));
    m_actionExit->setShortcut(QKeySequence::Quit);
    m_actionExit->setStatusTip(tr("Exit application"));
    connect(m_actionExit, &QAction::triggered, this, &MainWindow::onExit);

    // Tools menu
    m_menuTools = menuBar()->addMenu(tr("&Tools"));

    m_actionOptions = m_menuTools->addAction(tr("&Options..."));
    m_actionOptions->setStatusTip(tr("Configure application settings"));
    connect(m_actionOptions, &QAction::triggered, this, &MainWindow::onOptions);

    m_menuLanguage = m_menuTools->addMenu(tr("&Language"));

    QAction* langEn = m_menuLanguage->addAction(tr("English"));
    connect(langEn, &QAction::triggered, this, &MainWindow::switchToEnglish);

    QAction* langZh = m_menuLanguage->addAction(tr("中文 (Chinese)"));
    connect(langZh, &QAction::triggered, this, &MainWindow::switchToChinese);

    // Help menu
    m_menuHelp = menuBar()->addMenu(tr("&Help"));

    m_actionDocumentation = m_menuHelp->addAction(tr("&Documentation"));
    m_actionDocumentation->setStatusTip(tr("View documentation"));
    connect(m_actionDocumentation, &QAction::triggered, this, &MainWindow::onDocumentation);

    m_menuHelp->addSeparator();

    m_actionAbout = m_menuHelp->addAction(tr("&About"));
    m_actionAbout->setStatusTip(tr("About qPCRtools"));
    connect(m_actionAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar* toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);

    toolbar->addAction(m_actionNew);
    toolbar->addAction(m_actionOpen);
    toolbar->addSeparator();
    toolbar->addAction(m_actionExport);
    toolbar->addAction(m_actionPrint);
}

void MainWindow::setupStatusBar()
{
    QStatusBar* statusBar = this->statusBar();

    m_statusLabel = new QLabel(statusBar);
    statusBar->addWidget(m_statusLabel, 1);

    m_languageLabel = new QLabel(statusBar);
    m_languageLabel->setText(m_currentLanguage == "zh" ? "中文" : "English");
    statusBar->addPermanentWidget(m_languageLabel);
}

void MainWindow::createWelcomeScreen()
{
    m_welcomeWidget = new QWidget();
    m_stackedWidget->addWidget(m_welcomeWidget);

    // Main layout
    auto* mainLayout = new QVBoxLayout(m_welcomeWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(40, 40, 40, 40);

    // Title
    auto* titleLabel = new QLabel(tr("qPCRtools - qPCR Data Analysis"), m_welcomeWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Subtitle
    auto* subtitleLabel = new QLabel(
        tr("Cross-platform desktop application for qPCR data processing and visualization"),
        m_welcomeWidget);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(12);
    subtitleLabel->setFont(subtitleFont);
    mainLayout->addWidget(subtitleLabel);

    mainLayout->addSpacing(40);

    // Analysis type selection
    auto* analysisLabel = new QLabel(tr("Select Analysis Type:"), m_welcomeWidget);
    QFont labelFont = analysisLabel->font();
    labelFont.setPointSize(14);
    labelFont.setBold(true);
    analysisLabel->setFont(labelFont);
    mainLayout->addWidget(analysisLabel);

    mainLayout->addSpacing(20);

    // Create analysis buttons grid
    createAnalysisButtons();

    // Button grid layout
    auto* buttonGrid = new QGridLayout();
    buttonGrid->setHorizontalSpacing(20);
    buttonGrid->setVerticalSpacing(20);

    buttonGrid->addWidget(m_btnStandardCurve, 0, 0);
    buttonGrid->addWidget(m_btnDeltaCt, 0, 1);
    buttonGrid->addWidget(m_btnDeltaDeltaCt, 1, 0);
    buttonGrid->addWidget(m_btnRqPCR, 1, 1);
    buttonGrid->addWidget(m_btnReverseTranscription, 2, 0, 1, 2);

    // Center the button grid
    auto* centerWidget = new QWidget();
    centerWidget->setLayout(buttonGrid);
    centerWidget->setMaximumWidth(600);

    auto* centerLayout = new QHBoxLayout();
    centerLayout->addStretch();
    centerLayout->addWidget(centerWidget);
    centerLayout->addStretch();

    mainLayout->addLayout(centerLayout);

    mainLayout->addStretch();

    // Footer
    auto* footerLabel = new QLabel(
        tr("Version 1.0.0 | Based on R package qPCRtools"),
        m_welcomeWidget);
    footerLabel->setAlignment(Qt::AlignCenter);
    QFont footerFont = footerLabel->font();
    footerFont.setPointSize(9);
    footerLabel->setFont(footerFont);
    mainLayout->addWidget(footerLabel);
}

void MainWindow::createAnalysisButtons()
{
    int buttonWidth = 250;
    int buttonHeight = 80;

    m_btnStandardCurve = new QPushButton(tr("Standard Curve"), m_welcomeWidget);
    m_btnStandardCurve->setMinimumSize(buttonWidth, buttonHeight);
    m_btnStandardCurve->setToolTip(tr("Calculate standard curve and amplification efficiency"));
    connect(m_btnStandardCurve, &QPushButton::clicked, this, &MainWindow::startStandardCurveAnalysis);

    m_btnDeltaCt = new QPushButton(tr("ΔCt Method"), m_welcomeWidget);
    m_btnDeltaCt->setMinimumSize(buttonWidth, buttonHeight);
    m_btnDeltaCt->setToolTip(tr("Calculate relative expression using 2^-ΔCt method"));
    connect(m_btnDeltaCt, &QPushButton::clicked, this, &MainWindow::startDeltaCtAnalysis);

    m_btnDeltaDeltaCt = new QPushButton(tr("ΔΔCt Method"), m_welcomeWidget);
    m_btnDeltaDeltaCt->setMinimumSize(buttonWidth, buttonHeight);
    m_btnDeltaDeltaCt->setToolTip(tr("Calculate relative expression using 2^-ΔΔCt method"));
    connect(m_btnDeltaDeltaCt, &QPushButton::clicked, this, &MainWindow::startDeltaDeltaCtAnalysis);

    m_btnRqPCR = new QPushButton(tr("RqPCR Method"), m_welcomeWidget);
    m_btnRqPCR->setMinimumSize(buttonWidth, buttonHeight);
    m_btnRqPCR->setToolTip(tr("Calculate expression using RqPCR algorithm"));
    connect(m_btnRqPCR, &QPushButton::clicked, this, &MainWindow::startRqPCRAnalysis);

    m_btnReverseTranscription = new QPushButton(tr("Reverse Transcription"), m_welcomeWidget);
    m_btnReverseTranscription->setMinimumSize(buttonWidth, buttonHeight);
    m_btnReverseTranscription->setToolTip(tr("Calculate RNA volume for reverse transcription"));
    connect(m_btnReverseTranscription, &QPushButton::clicked, this, &MainWindow::startReverseTranscription);

    // Style buttons
    QString buttonStyle =
        "QPushButton {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: 2px solid #3C5488;"
        "   border-radius: 8px;"
        "   background-color: #4DBBD5;"
        "   color: white;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3C5488;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #00A087;"
        "}";

    m_btnStandardCurve->setStyleSheet(buttonStyle);
    m_btnDeltaCt->setStyleSheet(buttonStyle);
    m_btnDeltaDeltaCt->setStyleSheet(buttonStyle);
    m_btnRqPCR->setStyleSheet(buttonStyle);
    m_btnReverseTranscription->setStyleSheet(buttonStyle);
}

//=============================================================================
// Menu actions
//=============================================================================

void MainWindow::onNewAnalysis()
{
    m_stackedWidget->setCurrentWidget(m_welcomeWidget);
    m_statusLabel->setText(tr("Ready to start new analysis"));
}

void MainWindow::onOpenFile()
{
    QString filter = tr("Data Files (*.csv *.txt *.xlsx);;CSV Files (*.csv);;Excel Files (*.xlsx);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data File"), m_lastOpenPath, filter);

    if (!fileName.isEmpty()) {
        m_lastOpenPath = QFileInfo(fileName).absolutePath();
        // TODO: Load and preview file
        m_statusLabel->setText(tr("Opened: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::onExportResults()
{
    QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx);;PDF Files (*.pdf)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Results"), m_lastExportPath, filter);

    if (!fileName.isEmpty()) {
        m_lastExportPath = QFileInfo(fileName).absolutePath();
        m_statusLabel->setText(tr("Results exported to: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::onExportChart()
{
    QString filter = tr("PNG Files (*.png);;JPEG Files (*.jpg);;PDF Files (*.pdf);;SVG Files (*.svg)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Chart"), m_lastExportPath, filter);

    if (!fileName.isEmpty()) {
        m_lastExportPath = QFileInfo(fileName).absolutePath();
        m_statusLabel->setText(tr("Chart exported to: %1").arg(QFileInfo(fileName).fileName()));
    }
}

void MainWindow::onPrint()
{
    // TODO: Implement printing
    m_statusLabel->setText(tr("Printing..."));
}

void MainWindow::onExit()
{
    QApplication::instance()->quit();
}

void MainWindow::onOptions()
{
    // TODO: Show options dialog
    m_statusLabel->setText(tr("Options..."));
}

void MainWindow::onLanguageSwitch()
{
    // Language switching handled by menu actions
}

void MainWindow::onDocumentation()
{
    // TODO: Open documentation
    m_statusLabel->setText(tr("Opening documentation..."));
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About qPCRtools"),
        tr("<h2>qPCRtools v1.0.0</h2>"
           "<p>Cross-platform desktop application for qPCR data analysis.</p>"
           "<p><b>Author:</b> Xiang LI<br>"
           "<b>Email:</b> lixiang117423@gmail.com<br>"
           "<b>License:</b> MIT</p>"
           "<p>This is a C++ reimplementation of the R package qPCRtools.</p>"
           "<p>© 2025 Xiang LI. All rights reserved.</p>"));
}

void MainWindow::onCheckUpdates()
{
    m_statusLabel->setText(tr("Checking for updates..."));
    // TODO: Implement update check
}

//=============================================================================
// Language
//=============================================================================

void MainWindow::switchToEnglish()
{
    installTranslator("en");
    m_currentLanguage = "en";
    m_languageLabel->setText("English");
}

void MainWindow::switchToChinese()
{
    installTranslator("zh");
    m_currentLanguage = "zh";
    m_languageLabel->setText("中文");
}

void MainWindow::installTranslator(const QString& language)
{
    // Remove old translator
    if (!m_translator.isEmpty()) {
        QApplication::instance()->removeTranslator(&m_translator);
    }

    // Load new translator
    QString qmFile = QString(":/translations/qpcr_tools_%1.qm").arg(language == "zh" ? "zh_CN" : "en_US");

    if (m_translator.load(qmFile)) {
        QApplication::instance()->installTranslator(&m_translator);
        m_statusLabel->setText(tr("Language changed"));
    } else {
        m_statusLabel->setText(tr("Translation file not found"));
    }
}

//=============================================================================
// Analysis wizards
//=============================================================================

void MainWindow::startStandardCurveAnalysis()
{
    // TODO: Launch Standard Curve wizard
    m_statusLabel->setText(tr("Starting Standard Curve analysis..."));
}

void MainWindow::startDeltaCtAnalysis()
{
    // TODO: Launch Delta Ct wizard
    m_statusLabel->setText(tr("Starting ΔCt analysis..."));
}

void MainWindow::startDeltaDeltaCtAnalysis()
{
    // TODO: Launch Delta-Delta Ct wizard
    m_statusLabel->setText(tr("Starting ΔΔCt analysis..."));
}

void MainWindow::startRqPCRAnalysis()
{
    // TODO: Launch RqPCR wizard
    m_statusLabel->setText(tr("Starting RqPCR analysis..."));
}

void MainWindow::startReverseTranscription()
{
    // TODO: Launch Reverse Transcription calculator
    m_statusLabel->setText(tr("Starting Reverse Transcription calculator..."));
}

//=============================================================================
// Settings
//=============================================================================

void MainWindow::loadSettings()
{
    QSettings settings;
    m_lastOpenPath = settings.value("lastOpenPath", QDir::homePath()).toString();
    m_lastExportPath = settings.value("lastExportPath", QDir::homePath()).toString();
    m_currentLanguage = settings.value("language", "en").toString();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("lastOpenPath", m_lastOpenPath);
    settings.setValue("lastExportPath", m_lastExportPath);
    settings.setValue("language", m_currentLanguage);
}

//=============================================================================
// UI Translation
//=============================================================================

void MainWindow::retranslateUi()
{
    // Retranslate menu bar
    m_menuFile->setTitle(tr("&File"));
    m_menuTools->setTitle(tr("&Tools"));
    m_menuLanguage->setTitle(tr("&Language"));
    m_menuHelp->setTitle(tr("&Help"));

    // Retranslate actions
    m_actionNew->setText(tr("&New Analysis"));
    m_actionOpen->setText(tr("&Open File..."));
    m_actionExport->setText(tr("&Export Results..."));
    m_actionPrint->setText(tr("&Print..."));
    m_actionExit->setText(tr("E&xit"));
    m_actionOptions->setText(tr("&Options..."));
    m_actionDocumentation->setText(tr("&Documentation"));
    m_actionAbout->setText(tr("&About"));

    setWindowTitle("qPCRtools");
}

} // namespace qpcr
