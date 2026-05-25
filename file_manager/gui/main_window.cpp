#include "gui/main_window.hpp"
#include "gui/file_view.hpp"
#include "ui_mainwindow.h"

#include <QDir>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QTreeWidgetItem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupModernUI();
    setupConnections();

    // Set initial path to Home — FileView owns navigation now.
    ui->fileView->navigateTo(QDir::homePath());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupModernUI() {
    setWindowTitle("File Manager");

    // Configure splitter
    ui->mainSplitter->setSizes({250, 950});
    ui->mainSplitter->setStretchFactor(0, 0);  // Sidebar doesn't stretch
    ui->mainSplitter->setStretchFactor(1, 1);  // Main content stretches

    // Configure toolbar
    ui->toolBar->setIconSize(QSize(20, 20));
    ui->toolBar->setMovable(false);

    // Sidebar tree widget
    ui->navigationTreeWidget->setRootIsDecorated(true);
    ui->navigationTreeWidget->expandAll();
}

void MainWindow::setupConnections() {
    FileView* fv = ui->fileView;

    // Initial action states must match an empty FileView: no history yet,
    // so back/forward/up are all impossible. FileView's capability signals
    // are flip-only (Requirements 3.9–3.11) — they do not re-emit "false"
    // on the first home navigation because the value never changed from
    // its starting false. Without this explicit init the toolbar would
    // show all three buttons enabled at startup but they'd do nothing.
    ui->actionBack->setEnabled(false);
    ui->actionForward->setEnabled(false);
    ui->actionUp->setEnabled(false);

    // FileView → MainWindow UI sync
    connect(fv, &FileView::pathChanged, this, [this](const QString& path) {
        setWindowTitle("File Manager - " + path);
        statusBar()->showMessage(path);
    });
    connect(fv, &FileView::statusChanged, this, &MainWindow::updateStatusBar);
    connect(fv, &FileView::canGoBackChanged,    ui->actionBack,    &QAction::setEnabled);
    connect(fv, &FileView::canGoForwardChanged, ui->actionForward, &QAction::setEnabled);
    connect(fv, &FileView::canGoUpChanged,      ui->actionUp,      &QAction::setEnabled);

    // Toolbar actions → FileView slots
    connect(ui->actionBack,    &QAction::triggered, fv, &FileView::goBack);
    connect(ui->actionForward, &QAction::triggered, fv, &FileView::goForward);
    connect(ui->actionUp,      &QAction::triggered, fv, &FileView::goUp);
    connect(ui->actionRefresh, &QAction::triggered, fv, &FileView::refresh);

    // Sidebar navigation
    connect(ui->navigationTreeWidget, &QTreeWidget::itemClicked,
            this, &MainWindow::on_navigationTreeWidget_itemClicked);
}

void MainWindow::updateStatusBar(int dirCount, int fileCount) {
    ui->sidebarStatusLabel->setText(
        QString("%1 dir(s), %2 file(s)").arg(dirCount).arg(fileCount));
}

void MainWindow::on_navigationTreeWidget_itemClicked(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column)

    if (!item) return;

    const QString location = item->text(0);
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
        ui->fileView->navigateTo(path);
    }
}
