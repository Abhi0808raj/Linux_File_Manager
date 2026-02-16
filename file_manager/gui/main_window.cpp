#include "gui/main_window.hpp"
#include "ui_mainwindow.h"

#include <QTreeWidgetItem>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QIcon>
#include <QHeaderView>
#include <QSplitter>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set up the modern appearance
    setupModernUI();
    setupModels();
    setupConnections();

    // Set initial path to Home
    navigateToPath(QDir::homePath());

    // Update status bar
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupModernUI() {
    // Set window properties
    setWindowTitle("File Manager");

    // Configure splitter
    ui->mainSplitter->setSizes({250, 950});
    ui->mainSplitter->setStretchFactor(0, 0);  // Sidebar doesn't stretch
    ui->mainSplitter->setStretchFactor(1, 1);  // Main content stretches

    // Configure toolbar
    ui->toolBar->setIconSize(QSize(20, 20));
    ui->toolBar->setMovable(false);

    // Configure table view for better appearance
    ui->fileTableView->setAlternatingRowColors(true);
    ui->fileTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->fileTableView->setSortingEnabled(true);
    ui->fileTableView->verticalHeader()->setVisible(false);
    ui->fileTableView->horizontalHeader()->setStretchLastSection(true);

    // Set up sidebar tree widget
    ui->navigationTreeWidget->setRootIsDecorated(true);
    ui->navigationTreeWidget->expandAll();
}

void MainWindow::setupModels() {
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setRootPath("");
    m_fileSystemModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    ui->fileTableView->setModel(m_fileSystemModel);

    // Configure column widths
    ui->fileTableView->setColumnWidth(0, 200);  // Name
    ui->fileTableView->setColumnWidth(1, 80);   // Size
    ui->fileTableView->setColumnWidth(2, 80);   // Type
    ui->fileTableView->setColumnWidth(3, 150);  // Date Modified

    // Sort by name initially
    ui->fileTableView->sortByColumn(0, Qt::AscendingOrder);
}

void MainWindow::setupConnections() {
    // Toolbar actions
    connect(ui->actionBack, &QAction::triggered, this, &MainWindow::on_actionBack_triggered);
    connect(ui->actionForward, &QAction::triggered, this, &MainWindow::on_actionForward_triggered);
    connect(ui->actionUp, &QAction::triggered, this, &MainWindow::on_actionUp_triggered);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::refreshView);

    // File view
    connect(ui->fileTableView, &QTableView::doubleClicked, this, &MainWindow::on_fileTableView_doubleClicked);

    // Sidebar navigation
    connect(ui->navigationTreeWidget, &QTreeWidget::itemClicked, this, &MainWindow::on_navigationTreeWidget_itemClicked);

    // File system model signals
    connect(m_fileSystemModel, &QFileSystemModel::directoryLoaded, this, &MainWindow::updateStatusBar);
}

void MainWindow::navigateToPath(const QString& path) {
    if (path.isEmpty()) return;

    QDir dir(path);
    if (!dir.exists()) return;

    // Set the root index for the table view
    QModelIndex rootIndex = m_fileSystemModel->index(path);
    ui->fileTableView->setRootIndex(rootIndex);

    // Update window title with current path
    setWindowTitle(QString("File Manager - %1").arg(path));

    // Manage navigation history
    if (m_historyIndex < 0 || m_history[m_historyIndex] != path) {
        // Remove any forward history if we're navigating to a new path
        if (m_historyIndex < m_history.size() - 1) {
            m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
        }
        m_history.append(path);
        m_historyIndex++;
    }

    // Update navigation buttons
    ui->actionBack->setEnabled(m_historyIndex > 0);
    ui->actionForward->setEnabled(m_historyIndex < m_history.size() - 1);
    ui->actionUp->setEnabled(dir.cdUp());

    // Store current path
    m_currentPath = path;

    // Update status bar
    updateStatusBar();
}

void MainWindow::updateStatusBar() {
    if (m_currentPath.isEmpty()) return;

    QDir currentDir(m_currentPath);
    QFileInfoList entries = currentDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    int dirCount = 0;
    int fileCount = 0;

    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            dirCount++;
        } else {
            fileCount++;
        }
    }

    QString statusText = QString("%1 dir(s), %2 file(s)").arg(dirCount).arg(fileCount);
    ui->sidebarStatusLabel->setText(statusText);
    statusBar()->showMessage(m_currentPath);
}

void MainWindow::refreshView() {
    if (!m_currentPath.isEmpty()) {
        m_fileSystemModel->setRootPath("");
        m_fileSystemModel->setRootPath(m_currentPath);
        updateStatusBar();
    }
}

// Slot implementations
void MainWindow::on_fileTableView_doubleClicked(const QModelIndex &index) {
    if (!index.isValid()) return;

    const QString path = m_fileSystemModel->filePath(index);

    if (m_fileSystemModel->isDir(index)) {
        navigateToPath(path);
    } else {
        // Open file with system default application
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::on_navigationTreeWidget_itemClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column)

    if (!item) return;

    QString location = item->text(0);
    QString path;

    // Map sidebar items to actual paths
    if (location == "Home") {
        path = QDir::homePath();
    } else if (location == "Desktop") {
        path = QDir::homePath() + "/Desktop";
    } else if (location == "Downloads") {
        path = QDir::homePath() + "/Downloads";
    } else if (location == "Bookmarks") {
        path = QDir::homePath();  // Default to home for bookmarks
    } else if (location == "Filesystem") {
        path = "/";
    }

    if (!path.isEmpty() && QDir(path).exists()) {
        navigateToPath(path);
    }
}

void MainWindow::on_actionUp_triggered() {
    if (m_currentPath.isEmpty()) return;

    QDir currentDir(m_currentPath);
    if (currentDir.cdUp()) {
        navigateToPath(currentDir.absolutePath());
    }
}

void MainWindow::on_actionBack_triggered() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        QString path = m_history[m_historyIndex];

        // Navigate without adding to history
        QModelIndex rootIndex = m_fileSystemModel->index(path);
        ui->fileTableView->setRootIndex(rootIndex);
        setWindowTitle(QString("File Manager - %1").arg(path));
        m_currentPath = path;

        // Update navigation buttons
        ui->actionBack->setEnabled(m_historyIndex > 0);
        ui->actionForward->setEnabled(m_historyIndex < m_history.size() - 1);

        updateStatusBar();
    }
}

void MainWindow::on_actionForward_triggered() {
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        QString path = m_history[m_historyIndex];

        // Navigate without adding to history
        QModelIndex rootIndex = m_fileSystemModel->index(path);
        ui->fileTableView->setRootIndex(rootIndex);
        setWindowTitle(QString("File Manager - %1").arg(path));
        m_currentPath = path;

        // Update navigation buttons
        ui->actionBack->setEnabled(m_historyIndex > 0);
        ui->actionForward->setEnabled(m_historyIndex < m_history.size() - 1);

        updateStatusBar();
    }
}