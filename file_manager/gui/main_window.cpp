#include "gui/main_window.hpp"
#include "ui_mainwindow.h" // The header file generated from mainwindow.ui

#include <QTreeWidgetItem>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Note on Icons: The .ui file does not include icons. For a complete look,
    // you would create a Qt Resource file (.qrc), add your icon files to it,
    // and then set them in the designer or in code like this:
    // ui->actionBack->setIcon(QIcon(":/icons/arrow_back.png"));

    setupModels();
    setupConnections();

    // Set initial path to Home
    navigateToPath(QDir::homePath());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupModels() {
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setRootPath(""); // Allow viewing the whole filesystem
    m_fileSystemModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    ui->fileTableView->setModel(m_fileSystemModel);

    // Set table view properties
    ui->fileTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTableView->setSortingEnabled(true);
    ui->fileTableView->sortByColumn(0, Qt::AscendingOrder);
}

void MainWindow::setupConnections() {
    // These connections are for the custom slots defined in the header
    connect(ui->actionBack, &QAction::triggered, this, &MainWindow::on_actionBack_triggered);
    connect(ui->actionForward, &QAction::triggered, this, &MainWindow::on_actionForward_triggered);
    connect(ui->actionUp, &QAction::triggered, this, &MainWindow::on_actionUp_triggered);

    connect(ui->fileTableView, &QTableView::doubleClicked, this, &MainWindow::on_fileTableView_doubleClicked);
    connect(ui->navigationTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::on_navigationTreeWidget_itemClicked);
}

void MainWindow::navigateToPath(const QString& path) {
    if (path.isEmpty()) return;

    QDir dir(path);
    if (!dir.exists()) return;

    ui->fileTableView->setRootIndex(m_fileSystemModel->index(path));
    setWindowTitle(path);

    // Add to history
    if (m_historyIndex < 0 || m_history[m_historyIndex] != path) {
        if (m_historyIndex < m_history.size() - 1) {
            m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
        }
        m_history.append(path);
        m_historyIndex++;
    }

    ui->actionBack->setEnabled(m_historyIndex > 0);
    ui->actionForward->setEnabled(m_historyIndex < m_history.size() - 1);
}

void MainWindow::on_fileTableView_doubleClicked(const QModelIndex &index) {
    const QString path = m_fileSystemModel->filePath(index);
    if (m_fileSystemModel->isDir(index)) {
        navigateToPath(path);
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::on_navigationTreeWidget_itemClicked(QTreeWidgetItem *item, int column) {
    QString location = item->text(column);
    QString path;

    if (location == "Home") path = QDir::homePath();
    else if (location == "Desktop") path = QDir::homePath() + "/Desktop";
    else if (location == "Downloads") path = QDir::homePath() + "/Downloads";
    else if (location == "Filesystem Root") path = "/";
    // Add other favorite locations here

    if (!path.isEmpty()) {
        navigateToPath(path);
    }
}

void MainWindow::on_actionUp_triggered() {
    QModelIndex currentRootIndex = ui->fileTableView->rootIndex();
    QString currentPath = m_fileSystemModel->filePath(currentRootIndex);
    QDir dir(currentPath);
    if (dir.cdUp()) {
        navigateToPath(dir.path());
    }
}

void MainWindow::on_actionBack_triggered() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        navigateToPath(m_history[m_historyIndex]);
    }
}

void MainWindow::on_actionForward_triggered() {
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        navigateToPath(m_history[m_historyIndex]);
    }
}