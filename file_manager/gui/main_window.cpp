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
  outer->addWidget(heading);
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
  auto *viewMode = new QComboBox;
  viewMode->setObjectName("viewMode");
  viewMode->setAccessibleName(tr("View mode"));
  viewMode->addItems({tr("Details"), tr("List")});
  actions->addWidget(viewMode);
  auto *sort = new QComboBox;
  sort->setObjectName("sortOrder");
  sort->setAccessibleName(tr("Sort files"));
  sort->addItems(
      {tr("Name A–Z"), tr("Name Z–A"), tr("Size"), tr("Type"), tr("Modified")});
  actions->addWidget(sort);
  actions->addStretch();
  content->addLayout(actions);
  fileView_ = new FileView;
  fileView_->setObjectName("fileView");
  content->addWidget(fileView_, 1);
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
    connect(action, &QAction::triggered, fileView_, handler);
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
  connect(fileView_, &FileView::canGoBackChanged, actionBack,
          &QAction::setEnabled);
  connect(fileView_, &FileView::canGoForwardChanged, actionForward,
          &QAction::setEnabled);
  connect(fileView_, &FileView::canGoUpChanged, actionUp, &QAction::setEnabled);
  connect(fileView_, &FileView::pathChanged, this, &MainWindow::updateLocation);
  connect(fileView_, &FileView::statusChanged, this,
          [this](int dirs, int files) {
            status_->setText(tr("%1 items · %2 folders · %3 files")
                                 .arg(dirs + files)
                                 .arg(dirs)
                                 .arg(files));
          });
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
  connect(newFolder, &QPushButton::clicked, fileView_,
          &FileView::createNewFolder);
  const auto shortcut = [this](const QKeySequence &keys, auto handler) {
    auto *s = new QShortcut(keys, this);
    connect(s, &QShortcut::activated, this, handler);
  };
  shortcut(QKeySequence("Ctrl+L"), [this] {
    pathEntry_->setFocus();
    pathEntry_->selectAll();
  });
  fileView_->navigateTo(QDir::homePath());
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
