#include "gui/main_window.hpp"
#include "gui/file_view.hpp"
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTabBar>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle(tr("File Manager"));
  resize(1280, 800);
  setMinimumSize(800, 540);
  auto *central = new QWidget(this);
  setCentralWidget(central);
  auto *outer = new QVBoxLayout(central);
  outer->setContentsMargins(24, 24, 24, 24);
  outer->setSpacing(24);
  auto *heading = new QLabel(tr("File Manager"));
  heading->setObjectName("applicationTitle");
  QFont headingFont = heading->font();
  headingFont.setPixelSize(16);
  headingFont.setWeight(QFont::Medium);
  heading->setFont(headingFont);
  auto *headingRow = new QHBoxLayout;
  headingRow->setSpacing(12);
  heading->setFixedWidth(180);
  headingRow->addWidget(heading);
  search_ = new QLineEdit;
  search_->setObjectName("folderSearch");
  search_->setAccessibleName(tr("Search this folder"));
  search_->setPlaceholderText(tr("Search this folder"));
  search_->setMaximumWidth(340);
  search_->setMinimumWidth(200);
  search_->setFixedHeight(48);
  auto *searchIcon = new QLabel(search_);
  searchIcon->setPixmap(QIcon(":/icons/figma/search.svg").pixmap(24, 24));
  searchIcon->setFixedSize(24, 24);
  searchIcon->move(12, 12);
  searchIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
  search_->setTextMargins(32, 0, 0, 0);
  search_->installEventFilter(this);
  headingRow->addWidget(search_);
  headingRow->addStretch();
  outer->addLayout(headingRow);
  connect(search_, &QLineEdit::textChanged, this, [this](const QString &text) {
    if (fileView_) fileView_->setSearchQuery(text);
  });
  auto *body = new QHBoxLayout;
  body->setSpacing(24);
  outer->addLayout(body, 1);
  auto *sidebar = new QFrame;
  sidebar->setObjectName("workspaceSidebar");
  sidebar->setFixedWidth(208);
  auto *sideLayout = new QVBoxLayout(sidebar);
  sideLayout->setContentsMargins(12, 16, 12, 16);
  sideLayout->setSpacing(16);
  auto *sideHeading = new QLabel(tr("Places"));
  sideHeading->setObjectName("sidebarHeading");
  sideLayout->addWidget(sideHeading);
  places_ = new QListWidget;
  places_->setObjectName("placesList");
  places_->setIconSize(QSize(24, 24));
  sideLayout->addWidget(places_);
  const auto addPlace = [this](const QString &label, const QString &path,
                               const QString &icon) {
    if (path.isEmpty() || !QDir(path).exists())
      return;
    auto *item = new QListWidgetItem(QIcon(":/icons/figma/" + icon + ".svg"),
                                     label, places_);
    item->setData(Qt::UserRole, QDir(path).absolutePath());
    item->setToolTip(path);
    item->setSizeHint(QSize(160, 48));
  };
  addPlace(tr("Home"), QDir::homePath(), "home");
  addPlace(tr("Documents"),
           QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
           "folder");
  addPlace(tr("Downloads"),
           QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
           "downloads");
  addPlace(tr("Pictures"),
           QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
           "pictures");
  addPlace(tr("Desktop"),
           QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
           "folder");
  addPlace(tr("Filesystem"), QDir::rootPath(), "folder");
  body->addWidget(sidebar);
  auto *content = new QVBoxLayout;
  content->setSpacing(16);
  body->addLayout(content, 1);
  auto *titleRow = new QHBoxLayout;
  title_ = new QLabel;
  title_->setObjectName("workspaceTitle");
  title_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  titleRow->addWidget(title_, 1);
  auto *newFolder = new QPushButton(tr("New folder"));
  newFolder->setObjectName("primaryButton");
  content->addLayout(titleRow);
  auto *tabRow = new QHBoxLayout;
  tabRow->setSpacing(12);
  tabs_ = new QTabBar;
  tabs_->setObjectName("folderTabs");
  tabs_->setAccessibleName(tr("Folder tabs"));
  tabs_->setIconSize(QSize(24, 24));
  tabs_->setExpanding(false);
  tabs_->setDrawBase(false);
  tabs_->setUsesScrollButtons(true);
  tabs_->setElideMode(Qt::ElideRight);
  tabs_->setFocusPolicy(Qt::StrongFocus);
  tabs_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  tabRow->addWidget(tabs_);
  auto *newTab = new QPushButton(tr("+ Tab"));
  newTab->setObjectName("newTab");
  newTab->setFixedWidth(96);
  tabRow->addWidget(newTab);
  tabRow->addStretch();
  content->addLayout(tabRow);
  auto *nav = new QHBoxLayout;
  nav->setSpacing(8);
  const auto button = [this, nav](const QString &text, const QString &name,
                                  const QString &tooltip) {
    auto *b = new QToolButton;
    b->setText(text);
    b->setObjectName(name);
    b->setToolTip(tooltip);
    b->setAccessibleName(tooltip);
    b->setFixedSize(40, 40);
    nav->addWidget(b);
    return b;
  };
  auto *back =
      button(QString::fromUtf8("←"), "navigateBack", tr("Back (Alt+Left)"));
  auto *forward = button(QString::fromUtf8("→"), "navigateForward",
                         tr("Forward (Alt+Right)"));
  auto *up = button(QString::fromUtf8("↑"), "navigateUp", tr("Up (Alt+Up)"));
  pathEntry_ = new QLineEdit;
  pathEntry_->setObjectName("pathEntry");
  pathEntry_->setAccessibleName(tr("Folder path"));
  pathEntry_->setMinimumWidth(80);
  nav->addWidget(pathEntry_, 1);
  auto *refresh =
      button(QString::fromUtf8("↻"), "navigateRefresh", tr("Refresh (F5)"));
  content->addLayout(nav);
  pathError_ = new QLabel;
  pathError_->setObjectName("pathError");
  pathError_->setWordWrap(true);
  pathError_->hide();
  content->addWidget(pathError_);
  auto *actions = new QHBoxLayout;
  actions->addWidget(newFolder);
  auto *viewMode = viewMode_ = new QComboBox;
  viewMode->setObjectName("viewMode");
  viewMode->setAccessibleName(tr("View mode"));
  viewMode->addItems({tr("Details"), tr("List")});
  actions->addWidget(viewMode);
  auto *sort = sort_ = new QComboBox;
  sort->setObjectName("sortOrder");
  sort->setAccessibleName(tr("Sort files"));
  sort->addItems(
      {tr("Name A–Z"), tr("Name Z–A"), tr("Size"), tr("Type"), tr("Modified")});
  actions->addWidget(sort);
  actions->addStretch();
  content->addLayout(actions);
  auto *scopeRow = new QHBoxLayout;
  searchState_ = new QLabel;
  searchState_->setObjectName("searchState");
  scopeRow->addWidget(searchState_, 1);
  clearSearch_ = new QPushButton(tr("Clear search"));
  clearSearch_->setObjectName("clearSearch");
  scopeRow->addWidget(clearSearch_);
  connect(clearSearch_, &QPushButton::clicked, search_, &QLineEdit::clear);
  content->addLayout(scopeRow);
  stack_ = new QStackedWidget;
  stack_->setObjectName("folderStack");
  content->addWidget(stack_, 1);
  status_ = new QLabel;
  status_->setObjectName("workspaceStatus");
  content->addWidget(status_);
  const auto navigationAction = [this](QToolButton *button, const QString &name,
                                       const QKeySequence &keys, auto handler) {
    auto *action = new QAction(button->text(), this);
    action->setObjectName(name);
    action->setToolTip(button->toolTip());
    action->setShortcut(keys);
    addAction(action);
    button->setDefaultAction(action);
    connect(action, &QAction::triggered, this, [this, handler] { (fileView_->*handler)(); });
    return action;
  };
  auto *actionBack = navigationAction(
      back, "actionBack", QKeySequence("Alt+Left"), &FileView::goBack);
  auto *actionForward =
      navigationAction(forward, "actionForward", QKeySequence("Alt+Right"),
                       &FileView::goForward);
  auto *actionUp =
      navigationAction(up, "actionUp", QKeySequence("Alt+Up"), &FileView::goUp);
  navigationAction(refresh, "actionRefresh", QKeySequence("F5"),
                   &FileView::refresh);
  actionBack->setEnabled(false);
  actionForward->setEnabled(false);
  actionUp->setEnabled(false);
  connect(pathEntry_, &QLineEdit::returnPressed, this, [this] {
    QString path = pathEntry_->text().trimmed();
    if (path == "~")
      path = QDir::homePath();
    else if (path.startsWith("~/"))
      path = QDir::homePath() + path.mid(1);
    if (QDir::isRelativePath(path))
      path = QDir(fileView_->currentPath()).absoluteFilePath(path);
    if (!QFileInfo(path).isDir()) {
      pathError_->setText(tr("Folder does not exist: %1").arg(path));
      pathError_->show();
      pathEntry_->setFocus();
      return;
    }
    fileView_->navigateTo(QDir(path).absolutePath());
  });
  const auto navigatePlace = [this](QListWidgetItem *item) {
    if (item && item->data(Qt::UserRole).toString() != fileView_->currentPath())
      fileView_->navigateTo(item->data(Qt::UserRole).toString());
  };
  connect(places_, &QListWidget::itemClicked, this, navigatePlace);
  connect(places_, &QListWidget::itemActivated, this, navigatePlace);
  connect(viewMode, &QComboBox::currentIndexChanged, this, [this](int index) {
    fileView_->setViewMode(index == 0 ? ViewMode::Table : ViewMode::List);
  });
  connect(sort, &QComboBox::currentIndexChanged, this, [this](int index) {
    fileView_->setSortColumn(index < 2 ? 0 : index - 1,
                             index == 1 ? Qt::DescendingOrder
                                        : Qt::AscendingOrder);
  });
  connect(newFolder, &QPushButton::clicked, this, [this] { fileView_->createNewFolder(); });
  const auto shortcut = [this](const QKeySequence &keys, auto handler) {
    auto *s = new QShortcut(keys, this);
    connect(s, &QShortcut::activated, this, handler);
  };
  shortcut(QKeySequence("Ctrl+L"), [this] {
    pathEntry_->setFocus();
    pathEntry_->selectAll();
  });
  shortcut(QKeySequence("Ctrl+F"), [this] {
    search_->setFocus(); search_->selectAll();
  });
  auto *newTabAction = new QAction(tr("New tab"), this);
  newTabAction->setObjectName("actionNewTab");
  newTabAction->setShortcut(QKeySequence("Ctrl+T"));
  addAction(newTabAction);
  connect(newTabAction, &QAction::triggered, this, [this] { addTab(fileView_->currentPath()); });
  connect(newTab, &QPushButton::clicked, newTabAction, &QAction::trigger);
  closeTab_ = new QAction(tr("Close tab"), this);
  closeTab_->setObjectName("actionCloseTab");
  closeTab_->setShortcut(QKeySequence("Ctrl+W"));
  addAction(closeTab_);
  connect(closeTab_, &QAction::triggered, this, [this] { closeTab(tabs_->currentIndex()); });
  connect(tabs_, &QTabBar::currentChanged, this, &MainWindow::activateTab);
  shortcut(QKeySequence("Ctrl+Tab"), [this] { tabs_->setCurrentIndex((tabs_->currentIndex()+1)%tabs_->count()); });
  shortcut(QKeySequence("Ctrl+Shift+Tab"), [this] { tabs_->setCurrentIndex((tabs_->currentIndex()+tabs_->count()-1)%tabs_->count()); });
  addTab(QDir::homePath());
}
void MainWindow::updateLocation(const QString &path) {
  pathEntry_->setText(QDir::toNativeSeparators(path));
  pathEntry_->setCursorPosition(0);
  pathError_->hide();
  QString name = QFileInfo(path).fileName();
  if (name.isEmpty())
    name = QDir::toNativeSeparators(path);
  title_->setText(name);
  title_->setToolTip(path);
  setWindowTitle(tr("%1 — File Manager").arg(name));
  status_->setText(tr("Loading folder…"));
  places_->clearSelection();
  for (int i = 0; i < places_->count(); ++i)
    if (places_->item(i)->data(Qt::UserRole).toString() == path)
      places_->item(i)->setSelected(true);
}

void MainWindow::addTab(const QString &path) {
  if (!QFileInfo(path).isDir()) {
    pathError_->setText(tr("Folder does not exist: %1").arg(path));
    pathError_->show();
    return;
  }
  auto *view = new FileView;
  view->setObjectName("fileView");
  view->setClipboardManager(&clipboard_);
  stack_->addWidget(view);
  connect(view, &FileView::pathChanged, this, [this, view](const QString &path) {
    const int index = stack_->indexOf(view);
    QString label = QFileInfo(path).fileName();
    if (label.isEmpty()) label = QDir::toNativeSeparators(path);
    tabs_->setTabText(index, label);
    tabs_->setTabToolTip(index, path);
    if (view == fileView_) updateLocation(path);
  });
  connect(view, &FileView::statusChanged, this, [this, view](int dirs, int files) {
    if (view == fileView_) updateSearchStatus(dirs, files);
  });
  connect(view, &FileView::searchChanged, this, [this, view](const QString& query) {
    if (view != fileView_) return;
    QSignalBlocker block(search_); search_->setText(query);
  });
  const auto capability = [this, view](bool) { if (view == fileView_) syncControls(); };
  connect(view, &FileView::canGoBackChanged, this, capability);
  connect(view, &FileView::canGoForwardChanged, this, capability);
  connect(view, &FileView::canGoUpChanged, this, capability);
  QSignalBlocker tabBlock(tabs_);
  const int index = tabs_->addTab(QIcon(":/icons/figma/folder.svg"), QString());
  auto *close = new QToolButton;
  close->setIcon(QIcon(":/icons/figma/close.svg"));
  close->setIconSize(QSize(24, 24));
  close->setAccessibleName(tr("Close tab"));
  close->setToolTip(tr("Close tab"));
  close->setFixedSize(24,24);
  tabs_->setTabButton(index, QTabBar::RightSide, close);
  connect(close, &QToolButton::clicked, this, [this, view] { closeTab(stack_->indexOf(view)); });
  view->navigateTo(path);
  tabs_->setCurrentIndex(index);
  activateTab(index);
}
void MainWindow::activateTab(int index) {
  if (index < 0 || index >= stack_->count()) return;
  stack_->setCurrentIndex(index);
  fileView_ = qobject_cast<FileView *>(stack_->widget(index));
  { QSignalBlocker block(search_); search_->setText(fileView_->searchQuery()); }
  updateLocation(fileView_->currentPath());
  fileView_->refreshClipboardIndicators();
  fileView_->updateStatus();
  syncControls();
}
void MainWindow::updateSearchStatus(int dirs, int files) {
  const QString state = fileView_->folderState();
  const bool searching = !fileView_->searchQuery().isEmpty();
  clearSearch_->setVisible(searching);
  searchState_->setVisible(searching || !state.isEmpty());
  if (!state.isEmpty()) {
    searchState_->setText(state); status_->setText(state); return;
  }
  const int count = dirs + files;
  searchState_->setText(count == 0 ? tr("This folder · No matching files")
                                 : count == 1 ? tr("This folder · 1 result")
                                              : tr("This folder · %1 results").arg(count));
  status_->setText(tr("%1 items · %2 folders · %3 files").arg(dirs+files).arg(dirs).arg(files));
}
bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == search_ && event->type() == QEvent::KeyPress &&
      static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
    search_->clear(); if (fileView_) fileView_->focusFiles(); return true;
  }
  return QMainWindow::eventFilter(watched, event);
}
void MainWindow::syncControls() {
  if (!fileView_) return;
  findChild<QAction *>("actionBack")->setEnabled(fileView_->canGoBack());
  findChild<QAction *>("actionForward")->setEnabled(fileView_->canGoForward());
  findChild<QAction *>("actionUp")->setEnabled(!fileView_->currentPath().isEmpty() && fileView_->currentPath() != QDir::rootPath());
  QSignalBlocker modeBlock(viewMode_), sortBlock(sort_);
  viewMode_->setCurrentIndex(fileView_->currentViewMode() == ViewMode::Table ? 0 : 1);
  sort_->setCurrentIndex(fileView_->sortColumn() == 0 ? (fileView_->sortOrder() == Qt::DescendingOrder ? 1 : 0) : fileView_->sortColumn()+1);
  closeTab_->setEnabled(tabs_->count() > 1);
  for (int i=0; i<tabs_->count(); ++i)
    if (auto *button = tabs_->tabButton(i, QTabBar::RightSide)) button->setEnabled(tabs_->count()>1);
}
void MainWindow::closeTab(int index) {
  if (tabs_->count() <= 1 || index < 0) return;
  QWidget *view = stack_->widget(index);
  QSignalBlocker block(tabs_);
  stack_->removeWidget(view);
  tabs_->removeTab(index);
  delete view;
  activateTab(tabs_->currentIndex());
}
