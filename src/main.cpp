#include <QApplication>
#include "GeomProcessorWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("GeomProcessor");
    app.setOrganizationName("SimToolChain");

    GeomProcessorWindow win;
    win.show();

    return app.exec();
}
