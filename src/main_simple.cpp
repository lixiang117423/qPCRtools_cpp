#include <QApplication>
#include "GUI/WebMainWindow.h"
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    QApplication::setApplicationName("qPCRtools");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("qPCRtools");

    // 创建并显示主窗口
    qpcr::WebMainWindow mainWindow;
    mainWindow.show();

    qDebug() << "qPCRtools started with QWebEngineView interface";

    return app.exec();
}
