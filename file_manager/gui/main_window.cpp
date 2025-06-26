#include "gui/main_window.hpp"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileSystemModel>
#include <QTreeView>
#include <QSplitter>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("C++ Qt File Manager");

    // Central widget setup
    auto* treeView = new QTreeView(this);
    auto* model = new QFileSystemModel(this);
    model->setRootPath(QDir::rootPath());
    treeView->setModel(model);
    treeView->setRootIndex(model->index(QDir::homePath()));

    setCentralWidget(treeView);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("Exit", this, &MainWindow::close);
}

void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar("Main Toolbar");
    toolbar->addAction("Home", [this]() {
        // Go to home directory (update root index)
    });
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Ready");
}
