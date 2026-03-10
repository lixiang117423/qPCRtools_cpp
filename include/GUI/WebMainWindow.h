#ifndef WEBMAINWINDOW_H
#define WEBMAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebChannel>
#include <QProgressBar>
#include <QLabel>
#include "WebBridge.h"

namespace qpcr {

/**
 * @brief 基于WebEngine的主窗口
 *
 * 使用QWebEngineView加载HTML界面，通过QWebChannel与JavaScript通信
 */
class WebMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WebMainWindow(QWidget *parent = nullptr);
    ~WebMainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onWebLoadProgress(int progress);
    void onWebLoadFinished(bool success);
    void onError(const QString &error);
    void onProgressChanged(int progress, const QString &message);

private:
    void setupUI();
    void setupConnections();
    void loadWebInterface();
    QString getWebInterfacePath();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();

    QWebEngineView *m_webView;
    QWebChannel *m_webChannel;
    WebBridge *m_bridge;

    // 菜单和工具栏
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QStatusBar *m_statusBar;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;

    // 语言
    QString m_currentLanguage;
};

} // namespace qpcr

#endif // WEBMAINWINDOW_H
