#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    void setupUi();         // setup widgets/layouts
    void setupMenuBar();    // file/edit/view/help menus
    void setupToolBar();    // navigation buttons
    void setupStatusBar();  // current path, messages
};
