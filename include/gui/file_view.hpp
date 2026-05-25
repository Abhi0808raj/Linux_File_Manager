#pragma once

#include <QFileSystemModel>
#include <QList>
#include <QListView>
#include <QModelIndex>
#include <QStackedWidget>
#include <QString>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

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

    // View mode
    void setViewMode(ViewMode mode);
    ViewMode currentViewMode() const;

    // Sorting (table mode only; no-op in list mode)
    void setSortColumn(int column, Qt::SortOrder order);

public slots:
    void goBack();
    void goForward();
    void goUp();
    void refresh();

signals:
    void pathChanged(const QString& newPath);
    void statusChanged(int dirCount, int fileCount);
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
};
