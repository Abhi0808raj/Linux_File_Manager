#include <QApplication>
#include <QFile>
#include <QDebug>
#include "gui/main_window.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("File Manager");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Abhi0808raj");

    // Load and apply the modern stylesheet
    QFile styleFile(":/stylesheet.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        qDebug() << "Stylesheet loaded successfully";
    } else {
        qWarning() << "Could not find the stylesheet file";
    }

    MainWindow mainWin;
    mainWin.resize(1200, 800);
    mainWin.show();

    return app.exec();
}