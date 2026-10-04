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
  QDir(sample.path()).mkdir("Projects");
  QDir(sample.path()).mkdir("Design assets");
  for (const QString &name : {QString("Quarterly report.pdf"),
                              QString("A very long document name demonstrating "
                                      "the filename column elision.txt"),
                              QString("notes.md")}) {
    QFile file(sample.path() + "/" + name);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("sample workspace content");
  }
  MainWindow window;
  window.findChild<FileView *>("fileView")->navigateTo(sample.path());
  window.show();
  QTest::qWait(700);
  ASSERT_TRUE(window.grab().save(output + "/workspace-desktop.png"));
  window.resize(800, 600);
  QTest::qWait(100);
  ASSERT_TRUE(window.grab().save(output + "/workspace-narrow.png"));
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
