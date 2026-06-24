#include "MainWindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Agriculture Sensor Dashboard"));
    app.setOrganizationName(QStringLiteral("AgriTech"));

    QFile styleFile(QStringLiteral(":/styles/dashboard.qss"));
    if (styleFile.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow window;
    window.show();

    return app.exec();
}
