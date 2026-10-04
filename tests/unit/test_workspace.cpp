#include "gui/file_view.hpp"
#include "gui/main_window.hpp"
#include "gui/workspace_theme.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QTabBar>
#include <QStackedWidget>
#include <QAction>
#include <gtest/gtest.h>
class WorkspaceTest : public testing::Test {
protected:
  static void SetUpTestSuite() {
    if (!QApplication::instance()) {
      static int argc = 1;
      static char name[] = "workspace-test";
      static char *argv[] = {name, nullptr};
      new QApplication(argc, argv);
    }
    const QString captureFont = qEnvironmentVariable("FM_SCREENSHOT_FONT");
    if (!captureFont.isEmpty()) {
      const int id = QFontDatabase::addApplicationFont(captureFont);
      const auto families = QFontDatabase::applicationFontFamilies(id);
      ASSERT_FALSE(families.isEmpty());
      QFont font(families.first());
      font.setPixelSize(14);
      QApplication::setFont(font);
    }
    applyWorkspaceTheme(
        *qobject_cast<QApplication *>(QApplication::instance()));
  }
};
TEST_F(WorkspaceTest, PathEntryNavigatesAndHistoryRestoresLocation) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  QDir(dir.path()).mkdir("child");
  MainWindow window;
  auto *entry = window.findChild<QLineEdit *>("pathEntry");
  auto *view = window.findChild<FileView *>("fileView");
  ASSERT_NE(entry, nullptr);
  entry->setText(dir.path());
  QTest::keyClick(entry, Qt::Key_Return);
  EXPECT_EQ(view->currentPath(), dir.path());
  entry->setText("child");
  QTest::keyClick(entry, Qt::Key_Return);
  EXPECT_EQ(view->currentPath(), dir.path() + "/child");
  window.findChild<QToolButton *>("navigateBack")->click();
  EXPECT_EQ(view->currentPath(), dir.path());
  EXPECT_EQ(QDir::fromNativeSeparators(entry->text()), dir.path());
  window.findChild<QToolButton *>("navigateForward")->click();
  EXPECT_EQ(view->currentPath(), dir.path() + "/child");
}
TEST_F(WorkspaceTest, InvalidPathKeepsLocationAndShowsInlineError) {
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  auto before = view->currentPath();
  auto *entry = window.findChild<QLineEdit *>("pathEntry");
  entry->setText(before + "/missing-workspace-test-folder");
  QTest::keyClick(entry, Qt::Key_Return);
  EXPECT_EQ(view->currentPath(), before);
  EXPECT_FALSE(window.findChild<QLabel *>("pathError")->isHidden());
}
TEST_F(WorkspaceTest, PlacesAndViewSelectorOperateOnRealFileView) {
  MainWindow window;
  auto *places = window.findChild<QListWidget *>("placesList");
  ASSERT_GT(places->count(), 0);
  auto *view = window.findChild<FileView *>("fileView");
  places->itemActivated(places->item(0));
  EXPECT_EQ(view->currentPath(),
            places->item(0)->data(Qt::UserRole).toString());
  auto *modes = window.findChild<QComboBox *>("viewMode");
  modes->setCurrentIndex(1);
  EXPECT_EQ(view->currentViewMode(), ViewMode::List);
  modes->setCurrentIndex(0);
  EXPECT_EQ(view->currentViewMode(), ViewMode::Table);
}
TEST_F(WorkspaceTest, CtrlLAndNarrowLayoutRemainUsable) {
  MainWindow window;
  window.resize(800, 600);
  window.show();
  window.activateWindow();
  QTest::qWait(40);
  auto *entry = window.findChild<QLineEdit *>("pathEntry");
  QTest::keyClick(&window, Qt::Key_L, Qt::ControlModifier);
  EXPECT_TRUE(entry->hasFocus());
  EXPECT_EQ(entry->selectedText(), entry->text());
  EXPECT_GT(entry->width(), 80);
}
TEST_F(WorkspaceTest, OptionalWorkspaceScreenshots) {
  QString output = qEnvironmentVariable("FM_SCREENSHOT_DIR");
  if (output.isEmpty())
    GTEST_SKIP() << "FM_SCREENSHOT_DIR unset";
  QDir().mkpath(output);
  QTemporaryDir sample;
  QDir(sample.path()).mkdir("Documents");
  QDir(sample.path()).mkdir("Downloads");
  QDir(sample.path()).mkdir("Projects");
  QDir(sample.path()+"/Documents").mkdir("Projects");
  QDir(sample.path()+"/Documents").mkdir("Design assets");
  for (const QString &name : {QString("Quarterly report.pdf"),
                              QString("A very long document name demonstrating "
                                      "the filename column elision.txt"),
                              QString("notes.md")}) {
    QFile file(sample.path() + "/Documents/" + name);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("sample workspace content");
  }
  MainWindow window;
  window.findChild<FileView *>("fileView")->navigateTo(sample.path());
  window.findChild<QAction *>("actionNewTab")->trigger();
  auto *stack = window.findChild<QStackedWidget *>("folderStack");
  qobject_cast<FileView *>(stack->currentWidget())->navigateTo(sample.path()+"/Downloads");
  auto *tabs = window.findChild<QTabBar *>("folderTabs");
  tabs->setCurrentIndex(0);
  qobject_cast<FileView *>(stack->currentWidget())->navigateTo(sample.path()+"/Documents");
  window.show();
  QTest::qWait(700);
  ASSERT_TRUE(window.grab().save(output + "/workspace-desktop.png"));
  window.resize(800, 600);
  QTest::qWait(100);
  ASSERT_TRUE(window.grab().save(output + "/workspace-narrow.png"));
  for (int i=0; i<9; ++i) window.findChild<QAction *>("actionNewTab")->trigger();
  QTest::qWait(100);
  ASSERT_TRUE(window.grab().save(output + "/workspace-overflow.png"));
}

TEST_F(WorkspaceTest, IconsLoadAndCachedFolderStatusUpdates) {
  EXPECT_FALSE(QIcon(":/icons/figma/home.svg").isNull());
  QTemporaryDir sample;
  QFile file(sample.path() + "/notes.txt");
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.write("notes");
  file.close();
  QDir(sample.path()).mkdir("child");
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  auto *status = window.findChild<QLabel *>("workspaceStatus");
  view->navigateTo(sample.path());
  QTest::qWait(150);
  EXPECT_TRUE(status->text().contains("2 items"));
  view->navigateTo(sample.path() + "/child");
  view->goBack();
  EXPECT_TRUE(status->text().contains("2 items"));
}

TEST_F(WorkspaceTest, PlacesDoubleClickCreatesOnlyOneHistoryEntry) {
  QTemporaryDir prior;
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  auto *places = window.findChild<QListWidget *>("placesList");
  view->navigateTo(prior.path());
  window.show();
  QTest::qWait(30);
  const QPoint point = places->visualItemRect(places->item(0)).center();
  QTest::mouseClick(places->viewport(), Qt::LeftButton, Qt::NoModifier, point);
  QTest::mouseDClick(places->viewport(), Qt::LeftButton, Qt::NoModifier, point);
  EXPECT_EQ(view->currentPath(),
            places->item(0)->data(Qt::UserRole).toString());
  view->goBack();
  EXPECT_EQ(view->currentPath(), prior.path());
}
TEST_F(WorkspaceTest, SortSelectorOwnsSortingAndLongNamesDoNotWrap) {
  MainWindow window;
  auto *sort = window.findChild<QComboBox *>("sortOrder");
  auto *table = window.findChild<QTableView *>();
  auto *header = table->horizontalHeader();
  sort->setCurrentIndex(1);
  EXPECT_EQ(header->sortIndicatorSection(), 0);
  EXPECT_EQ(header->sortIndicatorOrder(), Qt::DescendingOrder);
  EXPECT_FALSE(header->sectionsClickable());
  sort->setCurrentIndex(4);
  EXPECT_EQ(header->sortIndicatorSection(), 3);
  EXPECT_EQ(header->sortIndicatorOrder(), Qt::AscendingOrder);
  EXPECT_FALSE(table->wordWrap());
  EXPECT_EQ(table->textElideMode(), Qt::ElideRight);
}

TEST_F(WorkspaceTest, TabsKeepIndependentHistoryAndRouteNavigation) {
  QTemporaryDir dir;
  QDir(dir.path()).mkdir("child");
  MainWindow window;
  auto *first = window.findChild<FileView *>("fileView");
  first->navigateTo(dir.path());
  auto *tabs = window.findChild<QTabBar *>("folderTabs");
  ASSERT_NE(tabs, nullptr);
  window.findChild<QAction *>("actionNewTab")->trigger();
  auto *stack = window.findChild<QStackedWidget *>("folderStack");
  auto *second = qobject_cast<FileView *>(stack->currentWidget());
  ASSERT_NE(first, second);
  EXPECT_EQ(second->currentPath(), dir.path());
  EXPECT_FALSE(window.findChild<QAction *>("actionBack")->isEnabled());
  second->navigateTo(dir.path()+"/child");
  tabs->setCurrentIndex(0);
  EXPECT_EQ(first->currentPath(), dir.path());
  window.findChild<QToolButton *>("navigateBack")->click();
  EXPECT_NE(first->currentPath(), dir.path());
  EXPECT_EQ(second->currentPath(), dir.path()+"/child");
}
TEST_F(WorkspaceTest, TabsRestoreViewSortAndKeepLastTab) {
  MainWindow window;
  auto *tabs = window.findChild<QTabBar *>("folderTabs");
  ASSERT_NE(tabs, nullptr);
  auto *mode = window.findChild<QComboBox *>("viewMode");
  auto *sort = window.findChild<QComboBox *>("sortOrder");
  mode->setCurrentIndex(1);
  sort->setCurrentIndex(4);
  window.findChild<QAction *>("actionNewTab")->trigger();
  EXPECT_EQ(mode->currentIndex(), 0);
  EXPECT_EQ(sort->currentIndex(), 0);
  tabs->setCurrentIndex(0);
  EXPECT_EQ(mode->currentIndex(), 1);
  EXPECT_EQ(sort->currentIndex(), 4);
  window.findChild<QAction *>("actionCloseTab")->trigger();
  EXPECT_EQ(tabs->count(), 1);
  EXPECT_FALSE(window.findChild<QAction *>("actionCloseTab")->isEnabled());
  window.findChild<QAction *>("actionCloseTab")->trigger();
  EXPECT_EQ(tabs->count(), 1);
}

TEST_F(WorkspaceTest, ClosingInactiveAndActiveTabsKeepsNeighborAndLabels) {
  QTemporaryDir dir;
  QDir(dir.path()).mkdir("child");
  MainWindow window;
  auto *tabs = window.findChild<QTabBar *>("folderTabs");
  auto *stack = window.findChild<QStackedWidget *>("folderStack");
  auto *first = qobject_cast<FileView *>(stack->currentWidget());
  first->navigateTo(dir.path());
  EXPECT_EQ(tabs->tabToolTip(0), dir.path());
  EXPECT_EQ(tabs->tabText(0), QFileInfo(dir.path()).fileName());
  window.findChild<QAction *>("actionNewTab")->trigger();
  auto *second = qobject_cast<FileView *>(stack->currentWidget());
  second->navigateTo(dir.path()+"/child");
  window.findChild<QAction *>("actionNewTab")->trigger();
  auto *third = qobject_cast<FileView *>(stack->currentWidget());
  auto *close = qobject_cast<QToolButton *>(tabs->tabButton(0, QTabBar::RightSide));
  ASSERT_NE(close, nullptr);
  close->click();
  EXPECT_EQ(stack->currentWidget(), third);
  EXPECT_EQ(tabs->count(), 2);
  window.findChild<QAction *>("actionCloseTab")->trigger();
  EXPECT_EQ(stack->currentWidget(), second);
  EXPECT_EQ(tabs->count(), 1);
}
TEST_F(WorkspaceTest, ClipboardSurvivesSourceTabCloseAndSelectionRestores) {
  for (const char *operation : {"onCopy", "onCut"}) {
    QTemporaryDir source, destination;
    QFile file(source.path()+"/payload.txt");
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("payload"); file.close();
    MainWindow window;
    auto *tabs = window.findChild<QTabBar *>("folderTabs");
    auto *stack = window.findChild<QStackedWidget *>("folderStack");
    auto *view = qobject_cast<FileView *>(stack->currentWidget());
    view->navigateTo(source.path());
    auto *table = view->findChild<QTableView *>();
    for (int i=0; i<30 && table->model()->rowCount(table->rootIndex())==0; ++i) QTest::qWait(20);
    auto index = table->model()->index(0, 0, table->rootIndex());
    ASSERT_TRUE(index.isValid());
    table->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ASSERT_TRUE(QMetaObject::invokeMethod(view, operation));
    window.findChild<QAction *>("actionNewTab")->trigger();
    auto *target = qobject_cast<FileView *>(stack->currentWidget());
    target->navigateTo(destination.path());
    tabs->setCurrentIndex(0);
    EXPECT_FALSE(table->selectionModel()->selectedRows().isEmpty());
    // Inactive location/status/capability signals must not overwrite the active controls.
    target->navigateTo(destination.path());
    EXPECT_EQ(QDir::fromNativeSeparators(window.findChild<QLineEdit *>("pathEntry")->text()), source.path());
    tabs->setCurrentIndex(1);
    qobject_cast<QToolButton *>(tabs->tabButton(0, QTabBar::RightSide))->click();
    ASSERT_TRUE(QMetaObject::invokeMethod(target, "onPaste"));
    EXPECT_TRUE(QFileInfo::exists(destination.path()+"/payload.txt"));
    EXPECT_EQ(QFileInfo::exists(source.path()+"/payload.txt"), QString(operation)=="onCopy");
  }
}
TEST_F(WorkspaceTest, TabKeyboardShortcutsCycleAndClose) {
  MainWindow window;
  window.show(); window.activateWindow(); QTest::qWait(40);
  auto *tabs = window.findChild<QTabBar *>("folderTabs");
  QTest::keyClick(&window, Qt::Key_T, Qt::ControlModifier);
  ASSERT_EQ(tabs->count(), 2);
  EXPECT_EQ(tabs->currentIndex(), 1);
  QTest::keyClick(&window, Qt::Key_Tab, Qt::ControlModifier);
  EXPECT_EQ(tabs->currentIndex(), 0);
  QTest::keyClick(&window, Qt::Key_Tab, Qt::ControlModifier | Qt::ShiftModifier);
  EXPECT_EQ(tabs->currentIndex(), 1);
  QTest::keyClick(&window, Qt::Key_W, Qt::ControlModifier);
  EXPECT_EQ(tabs->count(), 1);
}

TEST_F(WorkspaceTest, NewTabFromDeletedLocationKeepsUsableExistingTab) {
  QTemporaryDir dir;
  const QString path = dir.path()+"/removed";
  ASSERT_TRUE(QDir().mkdir(path));
  MainWindow window;
  window.findChild<FileView *>("fileView")->navigateTo(path);
  ASSERT_TRUE(QDir().rmdir(path));
  window.findChild<QAction *>("actionNewTab")->trigger();
  EXPECT_EQ(window.findChild<QTabBar *>("folderTabs")->count(), 1);
  EXPECT_FALSE(window.findChild<QLabel *>("pathError")->isHidden());
  window.findChild<QAction *>("actionUp")->trigger();
  EXPECT_EQ(window.findChild<FileView *>("fileView")->currentPath(), dir.path());
}
