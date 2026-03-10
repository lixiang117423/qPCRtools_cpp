#include <QApplication>
#include "GUI/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Set application information
    app.setApplicationName("qPCRtools");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("qPCRtools");
    app.setOrganizationDomain("github.com/lixiang117423");

    // Create and show main window
    qpcr::MainWindow window;
    window.show();

    return app.exec();
}
