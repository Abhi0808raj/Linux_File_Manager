#include "gui/file_view.hpp"

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QProcess>
#include <QUrl>

#include "core/file_system.hpp"
#include "gui/context_menu_action_state.hpp"
#include "gui/properties_dialog.hpp"

FileView::FileView(QWidget* parent)
    : QWidget(parent)
{
    // --- Model ---
    model_ = new QFileSystemModel(this);
    model_->setRootPath(QString());
    model_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    // --- Proxy (cut-state visual indicator) ---
    proxy_ = new CutStateProxyModel(this);
    proxy_->setSourceModel(model_);

    // --- Views ---
    tableView_ = new QTableView(this);
    tableView_->setModel(proxy_);

    listView_ = new QListView(this);
    listView_->setModel(proxy_);

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

    // --- Context menu (built once, reused per right-click) ---
    contextMenu_ = new QMenu(this);

    actOpen_       = new QAction(tr("Open"), this);
    actOpenWith_   = new QAction(tr("Open With..."), this);
    actCopy_       = new QAction(tr("Copy"), this);
    actCut_        = new QAction(tr("Cut"), this);
    actPaste_      = new QAction(tr("Paste"), this);
    actRename_     = new QAction(tr("Rename"), this);
    actDelete_     = new QAction(tr("Delete"), this);
    actNewFolder_  = new QAction(tr("New Folder"), this);
    actProperties_ = new QAction(tr("Properties"), this);

    contextMenu_->addAction(actOpen_);
    contextMenu_->addAction(actOpenWith_);
    contextMenu_->addSeparator();
    contextMenu_->addAction(actCopy_);
    contextMenu_->addAction(actCut_);
    contextMenu_->addAction(actPaste_);
    contextMenu_->addSeparator();
    contextMenu_->addAction(actRename_);
    contextMenu_->addAction(actDelete_);
    contextMenu_->addSeparator();
    contextMenu_->addAction(actNewFolder_);
    contextMenu_->addSeparator();
    contextMenu_->addAction(actProperties_);

    connect(actOpen_,       &QAction::triggered, this, &FileView::onOpen);
    connect(actOpenWith_,   &QAction::triggered, this, &FileView::onOpenWith);
    connect(actCopy_,       &QAction::triggered, this, &FileView::onCopy);
    connect(actCut_,        &QAction::triggered, this, &FileView::onCut);
    connect(actPaste_,      &QAction::triggered, this, &FileView::onPaste);
    connect(actRename_,     &QAction::triggered, this, &FileView::onRename);
    connect(actDelete_,     &QAction::triggered, this, &FileView::onDelete);
    connect(actNewFolder_,  &QAction::triggered, this, &FileView::onNewFolder);
    connect(actProperties_, &QAction::triggered, this, &FileView::onProperties);

    // --- Context menu policy on both views ---
    tableView_->setContextMenuPolicy(Qt::CustomContextMenu);
    listView_->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tableView_, &QWidget::customContextMenuRequested,
            this, &FileView::onContextMenuRequested);
    connect(listView_, &QWidget::customContextMenuRequested,
            this, &FileView::onContextMenuRequested);
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
    const QModelIndex sourceRoot = model_->index(path);
    const QModelIndex proxyRoot = proxy_->mapFromSource(sourceRoot);
    tableView_->setRootIndex(proxyRoot);
    listView_->setRootIndex(proxyRoot);

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
    const QModelIndex sourceRoot = model_->index(path);
    const QModelIndex proxyRoot = proxy_->mapFromSource(sourceRoot);
    tableView_->setRootIndex(proxyRoot);
    listView_->setRootIndex(proxyRoot);
}

// Double-click handler: navigate into directories or open files with the
// system default application. Connected to both tableView_ and listView_ in
// the constructor (task 2.1).
void FileView::onItemActivated(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    const QModelIndex sourceIdx = proxy_->mapToSource(index);
    if (model_->isDir(sourceIdx)) {
        navigateTo(model_->filePath(sourceIdx));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(model_->filePath(sourceIdx)));
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

// ---------------------------------------------------------------------------
// Context-menu slot and helpers
// (action handler bodies will be filled in by task 10)
// ---------------------------------------------------------------------------

void FileView::onContextMenuRequested(const QPoint& pos)
{
    resolveSelectionForContextMenu(pos);
    updateActionStates();
    contextMenu_->exec(activeView()->viewport()->mapToGlobal(pos));
}

void FileView::onOpen()
{
    const QStringList paths = selectedPaths();
    if (paths.size() != 1)
        return;

    QString resolved = paths.first();
    QFileInfo fi(resolved);
    if (!fi.symLinkTarget().isEmpty())
        resolved = fi.symLinkTarget();

    QFileInfo resolvedInfo(resolved);
    if (resolvedInfo.isDir()) {
        navigateTo(resolved);
    } else {
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(resolved))) {
            QMessageBox::critical(this, tr("Open Failed"),
                                  tr("Could not open '%1'.").arg(resolved));
        }
    }
}

void FileView::onOpenWith()
{
    const QStringList paths = selectedPaths();
    if (paths.size() != 1)
        return;

    const QString path = paths.first();

    // Try the platform application chooser via xdg-open
    qint64 pid = 0;
    if (QProcess::startDetached(QStringLiteral("xdg-open"), {path}, QString(), &pid)) {
        return;  // launched successfully
    }

    // Fall back to asking the user for an application command
    bool ok = false;
    const QString command = QInputDialog::getText(
        this, tr("Open With"),
        tr("Enter application command:"),
        QLineEdit::Normal, QString(), &ok);

    if (!ok || command.trimmed().isEmpty())
        return;  // cancelled or empty — no-op

    if (!QProcess::startDetached(command.trimmed(), {path})) {
        QMessageBox::critical(this, tr("Open With Failed"),
                              tr("Could not launch application '%1'.").arg(command.trimmed()));
    }
}

void FileView::onCopy()
{
    clipboard_.set(selectedPaths(), ClipboardOperation::Copy);
    proxy_->clearCutPaths();
}

void FileView::onCut()
{
    const QStringList paths = selectedPaths();
    clipboard_.set(paths, ClipboardOperation::Cut);
    proxy_->setCutPaths(QSet<QString>(paths.begin(), paths.end()));
}

void FileView::onPaste()
{
    QStringList failures;
    const QList<QString> sources = clipboard_.paths();
    const ClipboardOperation op = clipboard_.operation();

    for (const QString& src : sources) {
        const QString dest = QDir(currentPath()).filePath(QFileInfo(src).fileName());
        bool success = false;

        if (op == ClipboardOperation::Copy) {
            success = FileSystem::copy(src.toStdString(), dest.toStdString());
        } else {
            success = FileSystem::move(src.toStdString(), dest.toStdString());
        }

        if (!success) {
            failures << QStringLiteral("%1 \u2192 %2").arg(src, dest);
        }
    }

    if (op == ClipboardOperation::Cut) {
        clipboard_.clear();
        proxy_->clearCutPaths();
    }

    if (!failures.isEmpty()) {
        QMessageBox::critical(this, tr("Paste Failed"),
                              tr("The following items could not be pasted:\n%1")
                                  .arg(failures.join(QLatin1Char('\n'))));
    }

    refresh();
}

void FileView::onRename()
{
    const QStringList paths = selectedPaths();
    if (paths.size() != 1)
        return;

    const QString path = paths.first();
    const QString oldName = QFileInfo(path).fileName();

    bool ok = false;
    const QString newName = QInputDialog::getText(
        this, tr("Rename"),
        tr("New name:"),
        QLineEdit::Normal, oldName, &ok);

    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == oldName)
        return;  // cancelled or unchanged — no-op

    const QString newPath = QDir(currentPath()).filePath(newName.trimmed());
    if (!FileSystem::move(path.toStdString(), newPath.toStdString())) {
        QMessageBox::critical(this, tr("Rename Failed"),
                              tr("Could not rename '%1' to '%2'.").arg(path, newName.trimmed()));
    } else {
        refresh();
    }
}

void FileView::onDelete()
{
    const QStringList paths = selectedPaths();
    if (paths.isEmpty())
        return;

    // Confirmation dialog
    QString confirmMsg;
    if (paths.size() == 1) {
        confirmMsg = tr("Delete '%1'?").arg(QFileInfo(paths.first()).fileName());
    } else {
        confirmMsg = tr("Delete %1 items?").arg(paths.size());
    }

    const int result = QMessageBox::question(
        this, tr("Confirm Delete"), confirmMsg,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result != QMessageBox::Yes)
        return;

    QStringList failures;
    for (const QString& path : paths) {
        if (!FileSystem::remove(path.toStdString())) {
            failures << path;
        }
    }

    if (!failures.isEmpty()) {
        QMessageBox::critical(this, tr("Delete Failed"),
                              tr("Could not delete the following items:\n%1")
                                  .arg(failures.join(QLatin1Char('\n'))));
    }

    refresh();
}

void FileView::onNewFolder()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("New Folder"),
        tr("Folder name:"),
        QLineEdit::Normal, QStringLiteral("New Folder"), &ok);

    if (!ok || name.trimmed().isEmpty())
        return;  // cancelled — no-op

    const QString targetPath = QDir(currentPath()).filePath(name.trimmed());
    if (!FileSystem::createDirectory(targetPath.toStdString())) {
        QMessageBox::critical(this, tr("New Folder Failed"),
                              tr("Could not create folder '%1'.").arg(targetPath));
    } else {
        refresh();
    }
}

void FileView::onProperties()
{
    const QStringList paths = selectedPaths();
    if (paths.size() != 1)
        return;

    const QString path = paths.first();
    if (!PropertiesDialog::showFor(path, this)) {
        QMessageBox::critical(this, tr("Properties"),
                              tr("Could not read metadata for '%1'.").arg(path));
    }
}

QStringList FileView::selectedPaths() const
{
    QStringList paths;
    QAbstractItemView* view = activeView();
    if (!view || !view->selectionModel())
        return paths;

    const QModelIndexList rows = view->selectionModel()->selectedRows(0);
    for (const QModelIndex& proxyIdx : rows) {
        const QModelIndex sourceIdx = proxy_->mapToSource(proxyIdx);
        paths << model_->filePath(sourceIdx);
    }
    return paths;
}

bool FileView::currentPathIsWritable() const
{
    return QFileInfo(currentPath()).isWritable();
}

QAbstractItemView* FileView::activeView() const
{
    return (stack_->currentIndex() == 0)
        ? static_cast<QAbstractItemView*>(tableView_)
        : static_cast<QAbstractItemView*>(listView_);
}

void FileView::resolveSelectionForContextMenu(const QPoint& pos)
{
    QAbstractItemView* view = activeView();
    if (!view)
        return;

    const QModelIndex idx = view->indexAt(pos);
    QItemSelectionModel* sel = view->selectionModel();
    if (!sel)
        return;

    if (idx.isValid()) {
        // If the clicked index is not already part of the selection, replace selection
        if (!sel->isSelected(idx)) {
            sel->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
        // Otherwise preserve existing multi-selection (no-op)
    } else {
        // Empty-area click — clear selection
        sel->clearSelection();
    }
}

void FileView::updateActionStates()
{
    const QStringList paths = selectedPaths();
    const int selectionCount = paths.size();
    const bool isWritable = currentPathIsWritable();
    const bool clipboardEmpty = clipboard_.isEmpty();

    // Determine if the single selected item is a file
    bool selectionIsFile = false;
    if (selectionCount == 1) {
        selectionIsFile = QFileInfo(paths.first()).isFile();
    }

    const ContextMenuActionState state =
        ContextMenuActionState::compute(selectionCount, isWritable, clipboardEmpty, selectionIsFile);

    actOpen_->setEnabled(state.open);
    actOpenWith_->setEnabled(state.openWith);
    actCopy_->setEnabled(state.copy);
    actCut_->setEnabled(state.cut);
    actPaste_->setEnabled(state.paste);
    actRename_->setEnabled(state.rename);
    actDelete_->setEnabled(state.del);
    actNewFolder_->setEnabled(state.newFolder);
    actProperties_->setEnabled(state.properties);
}
