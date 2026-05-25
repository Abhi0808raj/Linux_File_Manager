#include "gui/file_view.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QHeaderView>
#include <QModelIndex>
#include <QUrl>

FileView::FileView(QWidget* parent)
    : QWidget(parent)
{
    // --- Model ---
    model_ = new QFileSystemModel(this);
    model_->setRootPath(QString());
    model_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    // --- Views ---
    tableView_ = new QTableView(this);
    tableView_->setModel(model_);

    listView_ = new QListView(this);
    listView_->setModel(model_);

    // --- Stacked widget + layout ---
    stack_ = new QStackedWidget(this);
    stack_->addWidget(tableView_);   // index 0
    stack_->addWidget(listView_);    // index 1

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(stack_);
    setLayout(layout);

    // --- QTableView configuration ---
    tableView_->setAlternatingRowColors(true);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSortingEnabled(true);
    tableView_->verticalHeader()->setVisible(false);
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->setColumnWidth(0, 200);  // Name
    tableView_->setColumnWidth(1, 80);   // Size
    tableView_->setColumnWidth(2, 80);   // Type
    tableView_->setColumnWidth(3, 150);  // Date Modified
    tableView_->sortByColumn(0, Qt::AscendingOrder);

    // --- QListView configuration ---
    listView_->setViewMode(QListView::ListMode);

    // --- Default state ---
    viewMode_     = ViewMode::Table;
    historyIndex_ = -1;

    // --- Double-click connections ---
    connect(tableView_, &QTableView::doubleClicked,
            this, &FileView::onItemActivated);
    connect(listView_, &QListView::doubleClicked,
            this, &FileView::onItemActivated);

    // --- Directory loaded → status signal ---
    connect(model_, &QFileSystemModel::directoryLoaded,
            this, &FileView::onDirectoryLoaded);
}

FileView::~FileView() = default;

// ---------------------------------------------------------------------------
// Stub implementations for methods declared in the header
// (full bodies will be filled in by subsequent tasks)
// ---------------------------------------------------------------------------

void FileView::applyHistoryPosition(int newIndex)
{
    // Caller is responsible for any history_ mutation before calling this
    // helper; we only move the index, refresh the active root, and emit the
    // capability/path signals.
    historyIndex_ = newIndex;

    const QString path = currentPath();
    const QModelIndex root = model_->index(path);
    tableView_->setRootIndex(root);
    listView_->setRootIndex(root);

    emit pathChanged(path);

    // Emit capability signals only when their value flips relative to the
    // last emitted value (tracked via prevCanGo*_ cached members).
    const bool newCanGoBack    = historyIndex_ > 0;
    const bool newCanGoForward = historyIndex_ < history_.size() - 1;
    const bool newCanGoUp      = !path.isEmpty() && path != QDir::rootPath();

    if (newCanGoBack != prevCanGoBack_) {
        prevCanGoBack_ = newCanGoBack;
        emit canGoBackChanged(newCanGoBack);
    }
    if (newCanGoForward != prevCanGoForward_) {
        prevCanGoForward_ = newCanGoForward;
        emit canGoForwardChanged(newCanGoForward);
    }
    if (newCanGoUp != prevCanGoUp_) {
        prevCanGoUp_ = newCanGoUp;
        emit canGoUpChanged(newCanGoUp);
    }
}

void FileView::navigateTo(const QString& path)
{
    if (!QDir(path).exists())
        return;

    // Discard forward entries when navigating from a mid-history position
    if (historyIndex_ < history_.size() - 1)
        history_.erase(history_.begin() + historyIndex_ + 1, history_.end());

    history_.append(path);
    applyHistoryPosition(historyIndex_ + 1);
}

QString FileView::currentPath() const
{
    if (historyIndex_ < 0 || historyIndex_ >= history_.size())
        return QString();
    return history_[historyIndex_];
}

void FileView::setViewMode(ViewMode mode)
{
    stack_->setCurrentIndex(mode == ViewMode::Table ? 0 : 1);
    viewMode_ = mode;
}

ViewMode FileView::currentViewMode() const
{
    return viewMode_;
}

void FileView::setSortColumn(int column, Qt::SortOrder order)
{
    // Only the QTableView supports column sorting; the QListView has no
    // notion of columns, so this is inherently a no-op when the active
    // view is ListMode (the sort indicator on the table is updated either
    // way, which is the documented behavior).
    tableView_->sortByColumn(column, order);
}

void FileView::goBack()
{
    if (historyIndex_ <= 0)
        return;
    applyHistoryPosition(historyIndex_ - 1);
}

void FileView::goForward()
{
    if (historyIndex_ >= history_.size() - 1)
        return;
    applyHistoryPosition(historyIndex_ + 1);
}

void FileView::goUp()
{
    const QString cur = currentPath();
    if (cur.isEmpty())
        return;

    QDir d(cur);
    if (!d.cdUp())
        return;
    navigateTo(d.absolutePath());
}
void FileView::refresh()
{
    const QString path = currentPath();
    if (path.isEmpty()) return;

    // If the current path no longer exists, walk up to find an existing ancestor.
    if (!QDir(path).exists()) {
        QDir d(path);
        while (d.cdUp()) {
            if (d.exists()) {
                navigateTo(d.absolutePath());
                return;
            }
        }
        // No ancestor exists (extremely rare) — give up silently.
        return;
    }

    // Force the model to re-read this directory.  setRootPath("") then back to
    // the current path triggers QFileSystemModel::directoryLoaded which fires
    // statusChanged via our existing onDirectoryLoaded slot.
    model_->setRootPath(QString());
    model_->setRootPath(path);

    // Re-set root indexes so the views display the freshly-reloaded data.
    const QModelIndex root = model_->index(path);
    tableView_->setRootIndex(root);
    listView_->setRootIndex(root);
}

// Double-click handler: navigate into directories or open files with the
// system default application. Connected to both tableView_ and listView_ in
// the constructor (task 2.1).
void FileView::onItemActivated(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    if (model_->isDir(index)) {
        navigateTo(model_->filePath(index));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(model_->filePath(index)));
    }
}

void FileView::onDirectoryLoaded(const QString& path)
{
    // Count subdirectories and files in the loaded directory
    int dirCount  = 0;
    int fileCount = 0;

    const QModelIndex parent = model_->index(path);
    const int rows = model_->rowCount(parent);
    for (int i = 0; i < rows; ++i) {
        const QModelIndex child = model_->index(i, 0, parent);
        if (model_->isDir(child))
            ++dirCount;
        else
            ++fileCount;
    }

    emit statusChanged(dirCount, fileCount);
}
