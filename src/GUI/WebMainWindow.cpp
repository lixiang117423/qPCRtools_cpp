#include "GUI/WebMainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include <QFileInfo>
#include <QDebug>
#include <QTimer>
#include <QKeySequence>
#include <QCoreApplication>
#include <QDateTime>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace qpcr {

WebMainWindow::WebMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_webView(nullptr)
    , m_webChannel(nullptr)
    , m_bridge(nullptr)
    , m_currentLanguage("zh")
{
    setupUI();
    setupConnections();
    loadWebInterface();
}

WebMainWindow::~WebMainWindow()
{
}

void WebMainWindow::setupUI()
{
    // 设置窗口属性
    setWindowTitle(tr("qPCRtools - qPCR Data Analysis"));
    resize(1280, 800);
    setMinimumSize(1024, 768);

    // 创建Web视图
    m_webView = new QWebEngineView(this);
    setCentralWidget(m_webView);

    // 创建Web通道
    m_webChannel = new QWebChannel(this);
    m_bridge = new WebBridge(this);

    // 注册bridge对象到Web通道
    m_webChannel->registerObject(QLatin1String("bridge"), m_bridge);

    // 设置Web通道
    m_webView->page()->setWebChannel(m_webChannel);

    // 清除缓存以确保加载最新的网页文件
    m_webView->page()->profile()->clearHttpCache();

    // 禁用缓存以防止浏览器缓存旧的CSS/JS文件
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    m_webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

    // 初始化成员变量
    m_menuBar = nullptr;
    m_toolBar = nullptr;
    m_statusBar = nullptr;
    m_statusLabel = nullptr;
    m_progressBar = nullptr;

    // 创建菜单栏
    createMenuBar();

    // 不需要工具栏，因为Web界面已经有了所有的交互功能
    // createToolBar();

    // 创建状态栏
    createStatusBar();
}

void WebMainWindow::createMenuBar()
{
    // 使用QMainWindow的menuBar()方法
    m_menuBar = menuBar();
    m_menuBar->clear();

    // 文件菜单
    QMenu *fileMenu = m_menuBar->addMenu(tr("&File"));

    QAction *openCqAction = fileMenu->addAction(tr("Open Cq File..."));
    openCqAction->setShortcut(QKeySequence::Open);
    connect(openCqAction, &QAction::triggered, this, [this]() {
        QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx *.xls);;All Files (*.*)");
        QString filePath = m_bridge->showFileDialog(tr("Open Cq File"), filter);
        if (!filePath.isEmpty()) {
            m_bridge->loadCqFile(filePath);
        }
    });

    QAction *openDesignAction = fileMenu->addAction(tr("Open Design File..."));
    connect(openDesignAction, &QAction::triggered, this, [this]() {
        QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx *.xls);;All Files (*.*)");
        QString filePath = m_bridge->showFileDialog(tr("Open Design File"), filter);
        if (!filePath.isEmpty()) {
            m_bridge->loadDesignFile(filePath);
        }
    });

    fileMenu->addSeparator();

    QAction *exportAction = fileMenu->addAction(tr("Export Results..."));
    exportAction->setShortcut(QKeySequence::Save);
    connect(exportAction, &QAction::triggered, this, [this]() {
        QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx);;PDF Files (*.pdf)");
        m_bridge->showSaveDialog(tr("Export Results"), filter);
    });

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &WebMainWindow::close);

    // 编辑菜单
    QMenu *editMenu = m_menuBar->addMenu(tr("&Edit"));

    QAction *preferencesAction = editMenu->addAction(tr("&Preferences..."));
    connect(preferencesAction, &QAction::triggered, this, [this]() {
        // TODO: Open preferences dialog
    });

    // 视图菜单
    QMenu *viewMenu = m_menuBar->addMenu(tr("&View"));

    QAction *reloadAction = viewMenu->addAction(tr("&Reload"));
    reloadAction->setShortcut(QKeySequence::Refresh);
    connect(reloadAction, &QAction::triggered, m_webView, &QWebEngineView::reload);

    viewMenu->addSeparator();

    QAction *zoomInAction = viewMenu->addAction(tr("Zoom &In"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, m_webView, []() {
        // TODO: Implement zoom
    });

    QAction *zoomOutAction = viewMenu->addAction(tr("Zoom &Out"));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, m_webView, []() {
        // TODO: Implement zoom
    });

    // 工具菜单
    QMenu *toolsMenu = m_menuBar->addMenu(tr("&Tools"));

    QAction *languageAction = toolsMenu->addAction(tr("&Language"));
    connect(languageAction, &QAction::triggered, this, [this]() {
        // TODO: Language selection dialog
    });

    // 帮助菜单
    QMenu *helpMenu = m_menuBar->addMenu(tr("&Help"));

    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QString aboutText = tr(
            "<h3>qPCRtools v%1</h3>"
            "<p>qPCR Data Analysis Tool</p>"
            "<p>Features:</p>"
            "<ul>"
            "<li>Standard curve analysis</li>"
            "<li>ΔCt and ΔΔCt methods</li>"
            "<li>Statistical tests (t-test, ANOVA, Wilcoxon)</li>"
            "<li>ggplot2-style charts</li>"
            "</ul>"
            "<p>Copyright © 2025</p>"
        ).arg(m_bridge->getAppVersion());
        m_bridge->showMessage(tr("About qPCRtools"), aboutText);
    });
}

void WebMainWindow::createToolBar()
{
    // 清除已存在的工具栏
    QList<QToolBar*> allToolBars = findChildren<QToolBar*>();
    for (QToolBar* bar : allToolBars) {
        delete bar;
    }

    m_toolBar = addToolBar(tr("Main Toolbar"));
    m_toolBar->setMovable(false);

    QAction *openCqAction = m_toolBar->addAction(tr("Open Cq"));
    connect(openCqAction, &QAction::triggered, this, [this]() {
        QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx *.xls)");
        QString filePath = m_bridge->showFileDialog(tr("Open Cq File"), filter);
        if (!filePath.isEmpty()) {
            m_bridge->loadCqFile(filePath);
        }
    });

    QAction *openDesignAction = m_toolBar->addAction(tr("Open Design"));
    connect(openDesignAction, &QAction::triggered, this, [this]() {
        QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx *.xls)");
        QString filePath = m_bridge->showFileDialog(tr("Open Design File"), filter);
        if (!filePath.isEmpty()) {
            m_bridge->loadDesignFile(filePath);
        }
    });

    m_toolBar->addSeparator();

    QAction *calculateAction = m_toolBar->addAction(tr("Calculate"));
    connect(calculateAction, &QAction::triggered, this, [this]() {
        // TODO: Trigger calculation in web interface
    });

    m_toolBar->addSeparator();

    QAction *exportAction = m_toolBar->addAction(tr("Export"));
    connect(exportAction, &QAction::triggered, this, [this]() {
        QString filter = tr("CSV Files (*.csv);;Excel Files (*.xlsx);;PDF Files (*.pdf)");
        m_bridge->showSaveDialog(tr("Export Results"), filter);
    });
}

void WebMainWindow::createStatusBar()
{
    // 使用QMainWindow的statusBar()方法
    m_statusBar = statusBar();

    m_statusLabel = new QLabel(tr("Ready"));
    m_statusBar->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false);
    m_statusBar->addPermanentWidget(m_progressBar);
}

void WebMainWindow::setupConnections()
{
    // Web视图加载进度
    connect(m_webView, &QWebEngineView::loadProgress,
            this, &WebMainWindow::onWebLoadProgress);

    // Web视图加载完成
    connect(m_webView, &QWebEngineView::loadFinished,
            this, &WebMainWindow::onWebLoadFinished);

    // Bridge信号连接
    connect(m_bridge, &WebBridge::errorOccurred,
            this, &WebMainWindow::onError);

    connect(m_bridge, &WebBridge::progressChanged,
            this, &WebMainWindow::onProgressChanged);

    connect(m_bridge, &WebBridge::dataLoaded,
            this, [this](bool success, const QString &message) {
                if (success) {
                    m_statusLabel->setText(tr("Data loaded: %1").arg(message));
                } else {
                    m_statusLabel->setText(tr("Failed to load data: %1").arg(message));
                }
            });

    connect(m_bridge, &WebBridge::calculationCompleted,
            this, [this](bool success, const QString &message) {
                if (success) {
                    m_statusLabel->setText(tr("Calculation completed: %1").arg(message));
                } else {
                    m_statusLabel->setText(tr("Calculation failed: %1").arg(message));
                }
            });
}

void WebMainWindow::loadWebInterface()
{
    QString htmlPath = getWebInterfacePath();
    QFileInfo fileInfo(htmlPath);

    if (!fileInfo.exists()) {
        m_statusLabel->setText(tr("Error: Web interface not found at %1").arg(htmlPath));
        qWarning() << "Web interface file not found:" << htmlPath;
        return;
    }

    QUrl url = QUrl::fromLocalFile(htmlPath);

    // 添加时间戳参数来强制刷新，避免缓存问题
    url.setQuery(QString("_t=%1").arg(QDateTime::currentMSecsSinceEpoch()));

    m_webView->load(url);

    m_statusLabel->setText(tr("Loading web interface..."));
}

QString WebMainWindow::getWebInterfacePath()
{
    // 始终从应用程序目录的web文件夹中加载
    QString path = QCoreApplication::applicationDirPath() + "/web/index.html";

    // 检查文件是否存在
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qWarning() << "Web interface not found at:" << path;
        qWarning() << "Current application directory:" << QCoreApplication::applicationDirPath();

        // 尝试相对于当前工作目录的路径
        path = "web/index.html";
        fileInfo.setFile(path);
        if (!fileInfo.exists()) {
            qWarning() << "Web interface also not found at:" << path;
        }
    }

    return fileInfo.absoluteFilePath();
}

void WebMainWindow::onWebLoadProgress(int progress)
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(progress);
    m_statusLabel->setText(tr("Loading interface... %1%").arg(progress));
}

void WebMainWindow::onWebLoadFinished(bool success)
{
    m_progressBar->setVisible(false);

    if (success) {
        m_statusLabel->setText(tr("Web interface loaded successfully"));
    } else {
        m_statusLabel->setText(tr("Failed to load web interface"));
        QMessageBox::warning(this, tr("Loading Error"),
                             tr("Failed to load the web interface. Please check if the web files are present."));
    }
}

void WebMainWindow::onError(const QString &error)
{
    m_statusLabel->setText(tr("Error: %1").arg(error));
    QMessageBox::critical(this, tr("Error"), error);
}

void WebMainWindow::onProgressChanged(int progress, const QString &message)
{
    m_progressBar->setVisible(true);
    m_progressBar->setValue(progress);
    m_statusLabel->setText(message);

    if (progress >= 100) {
        QTimer::singleShot(2000, this, [this]() {
            m_progressBar->setVisible(false);
        });
    }
}

void WebMainWindow::closeEvent(QCloseEvent *event)
{
    // 检查是否有未保存的更改
    // TODO: Implement unsaved changes check

    QMainWindow::closeEvent(event);
}

} // namespace qpcr
