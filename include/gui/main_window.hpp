#pragma once

#include <QMainWindow>
#include <QTreeWidgetItem>

// Forward declaration of the auto-generated UI class
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Forward declaration — full definition lives in include/gui/file_view.hpp
// and is pulled in transitively via ui_mainwindow.h in main_window.cpp.
class FileView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_navigationTreeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void updateStatusBar(int dirCount, int fileCount);

private:
    void setupModernUI();
    void setupConnections();

    Ui::MainWindow* ui;
};
