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
#include <QPushButton>
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
TEST_F(WorkspaceTest, SearchIsLiteralTabLocalAndNavigationClears) {
  QTemporaryDir dir;
  QDir(dir.path()).mkdir("Alpha[1]");
  QDir(dir.path()).mkdir("Beta");
  QFile file(dir.path()+"/alpha[1].txt");
  ASSERT_TRUE(file.open(QIODevice::WriteOnly)); file.close();
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  view->navigateTo(dir.path());
  auto *search = window.findChild<QLineEdit *>("folderSearch");
  ASSERT_NE(search, nullptr);
  QTest::qWait(150);
  search->setText("ALPHA[1]");
  auto *table = view->findChild<QTableView *>();
  EXPECT_EQ(table->model()->rowCount(table->rootIndex()), 2);
  window.findChild<QAction *>("actionNewTab")->trigger();
  EXPECT_TRUE(search->text().isEmpty());
  window.findChild<QTabBar *>("folderTabs")->setCurrentIndex(0);
  EXPECT_EQ(search->text(), "ALPHA[1]");
  view->navigateTo(dir.path()+"/Beta");
  EXPECT_TRUE(search->text().isEmpty());
}
TEST_F(WorkspaceTest, SearchClearsHiddenSelectionAndUpdatesForNewFiles) {
  QTemporaryDir dir;
  QFile initial(dir.path()+"/alpha.txt");
  ASSERT_TRUE(initial.open(QIODevice::WriteOnly)); initial.close();
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  view->navigateTo(dir.path());
  QTest::qWait(150);
  auto *table = view->findChild<QTableView *>();
  auto index = table->model()->index(0, 0, table->rootIndex());
  ASSERT_TRUE(index.isValid());
  table->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  auto *search = window.findChild<QLineEdit *>("folderSearch");
  search->setText("beta");
  EXPECT_TRUE(table->selectionModel()->selectedRows().isEmpty());
  EXPECT_EQ(table->model()->rowCount(table->rootIndex()), 0);
  EXPECT_TRUE(window.findChild<QLabel *>("searchState")->text().contains("No matching files"));
  QFile added(dir.path()+"/beta.txt");
  ASSERT_TRUE(added.open(QIODevice::WriteOnly)); added.close();
  for (int i=0; i<100 && table->model()->rowCount(table->rootIndex())==0; ++i) QTest::qWait(20);
  EXPECT_EQ(table->model()->rowCount(table->rootIndex()), 1);
  EXPECT_TRUE(window.findChild<QLabel *>("searchState")->text().contains("1 result"));
  auto visible = table->model()->index(0,0,table->rootIndex());
  table->selectionModel()->select(visible,QItemSelectionModel::Select | QItemSelectionModel::Rows);
  ASSERT_TRUE(QMetaObject::invokeMethod(view,"onCopy"));
  QTemporaryDir destination;
  view->navigateTo(destination.path());
  ASSERT_TRUE(QMetaObject::invokeMethod(view,"onPaste"));
  EXPECT_TRUE(QFileInfo::exists(destination.path()+"/beta.txt"));
  EXPECT_FALSE(QFileInfo::exists(destination.path()+"/alpha.txt"));
}
TEST_F(WorkspaceTest, SearchKeyboardClearAndInvalidNavigation) {
  MainWindow window;
  window.show(); window.activateWindow(); QTest::qWait(40);
  auto *search = window.findChild<QLineEdit *>("folderSearch");
  search->setText("unmatched-name");
  QTest::keyClick(&window, Qt::Key_F, Qt::ControlModifier);
  EXPECT_TRUE(search->hasFocus());
  EXPECT_EQ(search->selectedText(), search->text());
  auto *entry = window.findChild<QLineEdit *>("pathEntry");
  entry->setText("/missing-search-test-path"); QTest::keyClick(entry, Qt::Key_Return);
  EXPECT_EQ(search->text(),"unmatched-name");
  search->setFocus(); QTest::keyClick(search,Qt::Key_Escape);
  EXPECT_TRUE(search->text().isEmpty());
  EXPECT_TRUE(window.findChild<QTableView *>()->hasFocus());
}
TEST_F(WorkspaceTest, SearchDistinguishesLoadingNoMatchesAndUnavailableFolder) {
  QTemporaryDir root;
  ASSERT_TRUE(root.isValid());
  const QString folder = root.path() + "/empty";
  ASSERT_TRUE(QDir().mkdir(folder));
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  auto *search = window.findChild<QLineEdit *>("folderSearch");
  auto *state = window.findChild<QLabel *>("searchState");
  view->navigateTo(folder);
  search->setText("missing");
  EXPECT_TRUE(state->text().contains("Loading"));
  for (int i = 0; i < 100 && !view->folderState().isEmpty(); ++i)
    QTest::qWait(20);
  EXPECT_TRUE(state->text().contains("No matching files"));
  window.findChild<QPushButton *>("clearSearch")->click();
  EXPECT_TRUE(search->text().isEmpty());
  EXPECT_TRUE(state->isHidden());
  search->setText("missing");
  ASSERT_TRUE(QDir().rmdir(folder));
  view->updateStatus();
  EXPECT_TRUE(state->text().contains("unavailable"));
  EXPECT_FALSE(state->text().contains("No matching files"));
}

TEST_F(WorkspaceTest, SearchInListUsesNativeSizeSortAndDoesNotRecurse) {
  QTemporaryDir root;
  ASSERT_TRUE(root.isValid());
  for (const auto &entry : {qMakePair(QString("alpha.bin"), 1000),
                            qMakePair(QString("beta.bin"), 2),
                            qMakePair(QString("gamma.bin"), 10)}) {
    QFile file(root.path() + "/" + entry.first);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(QByteArray(entry.second, 'x')), entry.second);
  }
  ASSERT_TRUE(QDir(root.path()).mkdir("nested"));
  QFile nested(root.path() + "/nested/deep-report.txt");
  ASSERT_TRUE(nested.open(QIODevice::WriteOnly));
  nested.close();
  MainWindow window;
  auto *view = window.findChild<FileView *>("fileView");
  view->navigateTo(root.path());
  for (int i = 0; i < 100 && !view->folderState().isEmpty(); ++i)
    QTest::qWait(20);
  ASSERT_TRUE(view->folderState().isEmpty());
  window.findChild<QComboBox *>("viewMode")->setCurrentIndex(1);
  auto *list = view->findChild<QListView *>();
  ASSERT_NE(list, nullptr);
  auto *search = window.findChild<QLineEdit *>("folderSearch");
  search->setText(".bin");
  window.findChild<QComboBox *>("sortOrder")->setCurrentIndex(2);
  QTest::qWait(100);
  ASSERT_EQ(list->model()->rowCount(list->rootIndex()), 3);
  EXPECT_EQ(list->model()->index(0, 0, list->rootIndex()).data().toString(), "beta.bin");
  EXPECT_EQ(list->model()->index(1, 0, list->rootIndex()).data().toString(), "gamma.bin");
  EXPECT_EQ(list->model()->index(2, 0, list->rootIndex()).data().toString(), "alpha.bin");
  search->setText("deep-report");
  EXPECT_EQ(list->model()->rowCount(list->rootIndex()), 0);
  search->setText("nested");
  EXPECT_EQ(list->model()->rowCount(list->rootIndex()), 1);
}

TEST_F(WorkspaceTest, BackgroundSearchUpdatesDoNotOverwriteActiveTab) {
  QTemporaryDir first, second;
  ASSERT_TRUE(first.isValid());
  ASSERT_TRUE(second.isValid());
  MainWindow window;
  auto *firstView = window.findChild<FileView *>("fileView");
  firstView->navigateTo(first.path());
  auto *search = window.findChild<QLineEdit *>("folderSearch");
  search->setText("first");
  window.findChild<QAction *>("actionNewTab")->trigger();
  auto *stack = window.findChild<QStackedWidget *>("folderStack");
  auto *secondView = qobject_cast<FileView *>(stack->currentWidget());
  ASSERT_NE(secondView, firstView);
  secondView->navigateTo(second.path());
  search->setText("second");
  for (int i = 0; i < 100 && !secondView->folderState().isEmpty(); ++i)
    QTest::qWait(20);
  const QString status = window.findChild<QLabel *>("workspaceStatus")->text();
  QFile file(first.path() + "/first-match.txt");
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  file.close();
  auto *table = firstView->findChild<QTableView *>();
  for (int i = 0; i < 100 && table->model()->rowCount(table->rootIndex()) != 1; ++i)
    QTest::qWait(20);
  EXPECT_EQ(search->text(), "second");
  EXPECT_EQ(window.findChild<QLabel *>("workspaceStatus")->text(), status);
  window.findChild<QTabBar *>("folderTabs")->setCurrentIndex(0);
  EXPECT_EQ(search->text(), "first");
  EXPECT_TRUE(window.findChild<QLabel *>("searchState")->text().contains("1 result"));
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
  window.findChild<QLineEdit *>("folderSearch")->setText("report");
  QTest::qWait(100);
  ASSERT_TRUE(window.grab().save(output + "/search-desktop.png"));
  window.findChild<QLineEdit *>("folderSearch")->setText("missing-report");
  QTest::qWait(100);
  ASSERT_TRUE(window.grab().save(output + "/search-no-results.png"));
  window.findChild<QLineEdit *>("folderSearch")->setText("report");
  window.resize(800, 600);
  QTest::qWait(100);
  ASSERT_TRUE(window.grab().save(output + "/workspace-narrow.png"));
  ASSERT_TRUE(window.grab().save(output + "/search-narrow.png"));
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
