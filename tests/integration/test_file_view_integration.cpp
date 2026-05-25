// tests/integration/test_file_view_integration.cpp
//
// Integration tests for FileView <-> MainWindow signal/slot wiring.
//
// These tests construct real Qt widgets (FileView, MainWindow) and verify
// the asynchronous and connection-level behavior that unit tests cannot
// easily cover:
//
//   1. statusChanged emits the correct (dirCount, fileCount) after the
//      QFileSystemModel finishes loading a real on-disk directory.
//      Validates: Requirements 4.2.
//
//   2. MainWindow wires FileView::canGoBackChanged to actionBack::setEnabled.
//      Validates: Requirements 4.3, 4.5.
//
//   3. MainWindow wires FileView::canGoForwardChanged to actionForward::
//      setEnabled.
//      Validates: Requirements 4.4, 4.6, 4.7.
//
// Requirements: 4.2, 4.3, 4.4, 4.5, 4.6, 4.7

#include <gtest/gtest.h>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>

#include "gui/file_view.hpp"
#include "gui/main_window.hpp"

// ---------------------------------------------------------------------------
// QApplication fixture
//
// Qt widgets require a live QApplication for construction. We share a single
// instance across the suite (stored statically) to mirror the unit-test
// pattern used in tests/unit/test_file_view.cpp.
// ---------------------------------------------------------------------------
class FileViewIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int   argc   = 0;
            static char* argv[] = { nullptr };
            new QApplication(argc, argv);
        }
    }

    // Helper: create a directory tree with `numDirs` subdirectories and
    // `numFiles` files under `root`. Aborts the test on failure.
    static void populate(const QString& root, int numDirs, int numFiles) {
        QDir base(root);
        for (int i = 0; i < numDirs; ++i) {
            ASSERT_TRUE(base.mkdir(QString("dir%1").arg(i)))
                << "Failed to mkdir under " << root.toStdString();
        }
        for (int i = 0; i < numFiles; ++i) {
            QFile f(root + "/file" + QString::number(i) + ".txt");
            ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text))
                << "Failed to create file under " << root.toStdString();
            QTextStream(&f) << "payload " << i;
            f.close();
        }
    }
};

// ---------------------------------------------------------------------------
// Test 1: statusChanged emits correct dirCount and fileCount.
//
// Build a known directory layout with QTemporaryDir, attach a QSignalSpy to
// FileView::statusChanged, navigate into the temp directory, then drive the
// Qt event loop via QSignalSpy::wait() until the asynchronous
// QFileSystemModel::directoryLoaded → onDirectoryLoaded chain fires.
// Verify the most recent emission carries (2, 3).
//
// Validates: Requirements 4.2.
// ---------------------------------------------------------------------------
TEST_F(FileViewIntegrationTest, StatusChangedEmitsCorrectCounts) {
    // tempDir must outlive the FileView (QFileSystemModel watches paths);
    // C++ destroys locals in reverse declaration order, so declare tempDir
    // before the view.
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // 2 subdirectories + 3 files.
    populate(tempDir.path(), /*numDirs=*/2, /*numFiles=*/3);

    FileView   view;
    QSignalSpy spy(&view, &FileView::statusChanged);
    ASSERT_TRUE(spy.isValid());

    view.navigateTo(tempDir.path());

    // QFileSystemModel populates asynchronously; wait for at least one
    // statusChanged emission. The first directoryLoaded may carry stale
    // counts, so we keep waiting for additional emissions until the timeout
    // budget is exhausted. The most-recent emission is the one we verify.
    const int kTimeoutMs = 2000;
    ASSERT_TRUE(spy.wait(kTimeoutMs))
        << "statusChanged did not fire within " << kTimeoutMs << " ms";

    // Drain any further emissions that arrive within a short additional
    // window (QFileSystemModel can emit directoryLoaded multiple times as
    // it discovers entries).
    while (spy.wait(200)) {
        // keep collecting
    }

    ASSERT_GE(spy.count(), 1);

    const QList<QVariant> last = spy.takeLast();
    ASSERT_EQ(last.size(), 2);
    const int dirCount  = last.at(0).toInt();
    const int fileCount = last.at(1).toInt();

    EXPECT_EQ(dirCount, 2)  << "Expected 2 subdirectories";
    EXPECT_EQ(fileCount, 3) << "Expected 3 files";
}

// ---------------------------------------------------------------------------
// Test 2: MainWindow wires FileView::canGoBackChanged to
// actionBack::setEnabled.
//
// Construct a full MainWindow. The constructor calls
// fileView->navigateTo(QDir::homePath()), which leaves history with one
// entry (canGoBack == false). Navigating to a second path enables Back;
// going back to the first entry disables it again.
//
// Access internal widgets via QObject::findChild — the MainWindow::ui
// member is private but the .ui file assigns object names ("actionBack",
// "fileView") that findChild can locate.
//
// Validates: Requirements 4.3, 4.5.
// ---------------------------------------------------------------------------
TEST_F(FileViewIntegrationTest, MainWindowActionBackWiring) {
    // tempDir for the second navigation target. Must outlive the MainWindow
    // because FileView's QFileSystemModel watches paths inside it.
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    MainWindow mainWindow;

    auto* actionBack = mainWindow.findChild<QAction*>("actionBack");
    auto* fileView   = mainWindow.findChild<FileView*>("fileView");
    ASSERT_NE(actionBack, nullptr) << "actionBack not found in MainWindow";
    ASSERT_NE(fileView,   nullptr) << "fileView not found in MainWindow";

    // After the constructor's navigateTo(home), history has exactly one
    // entry, so canGoBack is false → actionBack should be disabled.
    EXPECT_FALSE(actionBack->isEnabled())
        << "actionBack should start disabled (single-entry history → canGoBack==false)";

    // Navigate to a second path. Now history has two entries, the index is
    // at the end, so canGoBack flips false→true and the connected
    // setEnabled(true) is invoked.
    fileView->navigateTo(tempDir.path());
    EXPECT_TRUE(actionBack->isEnabled())
        << "actionBack should be enabled after a second navigation";

    // goBack() returns to the first entry, canGoBack flips true→false
    // and actionBack should be disabled again.
    fileView->goBack();
    EXPECT_FALSE(actionBack->isEnabled())
        << "actionBack should be disabled after returning to the first entry";
}

// ---------------------------------------------------------------------------
// Test 3: MainWindow wires FileView::canGoForwardChanged to
// actionForward::setEnabled.
//
// Construct MainWindow (one history entry from the home navigation), then
// navigate to a second path. canGoForward stays false at this point
// (index is at the end). Calling goBack() moves the index off the end,
// so canGoForward flips false→true and actionForward becomes enabled.
//
// Validates: Requirements 4.4, 4.6, 4.7.
// ---------------------------------------------------------------------------
TEST_F(FileViewIntegrationTest, MainWindowActionForwardWiring) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    MainWindow mainWindow;

    auto* actionForward = mainWindow.findChild<QAction*>("actionForward");
    auto* fileView      = mainWindow.findChild<FileView*>("fileView");
    ASSERT_NE(actionForward, nullptr) << "actionForward not found in MainWindow";
    ASSERT_NE(fileView,      nullptr) << "fileView not found in MainWindow";

    // After construction (single-entry history) and after a second
    // navigateTo (two-entry history with index at the end), the user is at
    // the most-recent entry → canGoForward is false → actionForward stays
    // disabled.
    fileView->navigateTo(tempDir.path());
    EXPECT_FALSE(actionForward->isEnabled())
        << "actionForward should be disabled while index is at end of history";

    // goBack() moves the index off the end, so canGoForward flips
    // false→true and actionForward should be enabled via the wired slot.
    fileView->goBack();
    EXPECT_TRUE(actionForward->isEnabled())
        << "actionForward should be enabled after goBack() moves off the end";
}
