#pragma once

#include <QFileSystemModel>
#include <QList>
#include <QListView>
#include <QModelIndex>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

#include "gui/clipboard_manager.hpp"
#include "gui/cut_state_proxy_model.hpp"

class QMenu;
class QAction;
class QAbstractItemView;

enum class ViewMode { Table, List };

class FileViewTestAccess;  // forward decl — defined in tests/unit/test_file_view.cpp

class FileView : public QWidget {
    Q_OBJECT

    // Test-only read access to private navigation state.
    // Defined in tests/unit/test_file_view.cpp; production code never sees the body.
    friend class FileViewTestAccess;

public:
    explicit FileView(QWidget* parent = nullptr);
    ~FileView();

    // Navigation (not slots — callers use lambdas with connect())
    void navigateTo(const QString& path);
    QString currentPath() const;

    void setClipboardManager(ClipboardManager *manager);
    void refreshClipboardIndicators();
    bool canGoBack() const { return historyIndex_ > 0; }
    bool canGoForward() const { return historyIndex_ + 1 < history_.size(); }
    int sortColumn() const;
    Qt::SortOrder sortOrder() const;
    void updateStatus();
    void setSearchQuery(const QString& query);
    QString searchQuery() const { return searchQuery_; }
    QString folderState() const;
    void focusFiles();

    // View mode
    void setViewMode(ViewMode mode);
    ViewMode currentViewMode() const;

    // Sorting (shared model order applies to both views)
    void setSortColumn(int column, Qt::SortOrder order);

public slots:
    void goBack();
    void goForward();
    void goUp();
    void refresh();
    void createNewFolder();

signals:
    void pathChanged(const QString& newPath);
    void statusChanged(int dirCount, int fileCount);
    void searchChanged(const QString& query);
    void canGoBackChanged(bool canGo);
    void canGoForwardChanged(bool canGo);
    void canGoUpChanged(bool canGo);

private:
    QFileSystemModel* model_     = nullptr;  // owned
    QTableView*       tableView_ = nullptr;  // owned, stack index 0
    QListView*        listView_  = nullptr;  // owned, stack index 1
    QStackedWidget*   stack_     = nullptr;  // layout root

    QList<QString> history_;                       // ordered list of visited paths
    int            historyIndex_ = -1;             // current position in history
    ViewMode       viewMode_     = ViewMode::Table;
    QString searchQuery_;
    QSet<QString> loadedFolders_;

    // Cached capability booleans — used to emit change signals only on flip
    bool prevCanGoBack_    = false;
    bool prevCanGoForward_ = false;
    bool prevCanGoUp_      = false;

private:
    // Set historyIndex_ to newIndex, propagate the new root index to both
    // views, emit pathChanged, and emit each capability signal only when
    // its value flips relative to the last emitted value (tracked via
    // prevCanGoBack_/prevCanGoForward_/prevCanGoUp_).
    //
    // Callers (navigateTo, goBack, goForward) are responsible for any
    // history_ list mutation before invoking this helper; the helper
    // assumes history_ is already in its final shape for newIndex.
    void applyHistoryPosition(int newIndex);

private slots:
    void onItemActivated(const QModelIndex& index);
    void onDirectoryLoaded(const QString& path);

    // Context-menu action slots (Req 1.3, 1.5)
    void onContextMenuRequested(const QPoint& pos);
    void onOpen();
    void onOpenWith();
    void onCopy();
    void onCut();
    void onPaste();
    void onRename();
    void onDelete();
    void onNewFolder();
    void onProperties();

private:
    // --- Context-menu helpers ---
    QStringList selectedPaths() const;
    bool        currentPathIsWritable() const;
    QAbstractItemView* activeView() const;
    void resolveSelectionForContextMenu(const QPoint& pos);
    void updateActionStates();

    // --- Context-menu state (Req 12.1) ---
    ClipboardManager    clipboard_;
    ClipboardManager* sharedClipboard_ = &clipboard_;
    CutStateProxyModel* proxy_       = nullptr;   // owned; wraps model_
    QMenu*              contextMenu_ = nullptr;   // owned; built once

    // QAction* — created once, reused; only enabled state mutates per show
    QAction* actOpen_       = nullptr;
    QAction* actOpenWith_   = nullptr;
    QAction* actCopy_       = nullptr;
    QAction* actCut_        = nullptr;
    QAction* actPaste_      = nullptr;
    QAction* actRename_     = nullptr;
    QAction* actDelete_     = nullptr;
    QAction* actNewFolder_  = nullptr;
    QAction* actProperties_ = nullptr;
};
