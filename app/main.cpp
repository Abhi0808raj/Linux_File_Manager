#include <QApplication>
#include "gui/main_window.hpp"
#include "gui/workspace_theme.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application properties
    app.setApplicationName("File Manager");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Abhi0808raj");

    applyWorkspaceTheme(app);

    MainWindow mainWin;
    mainWin.resize(1200, 800);
    mainWin.show();

    return app.exec();
}
