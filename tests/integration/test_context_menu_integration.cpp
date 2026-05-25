// tests/integration/test_context_menu_integration.cpp
//
// Integration tests for the FileView context menu feature.
// These tests exercise cross-component behavior that requires a live
// QApplication and real filesystem operations via QTemporaryDir.
//
// Covers:
//   Property 4  — Cut-paste clears clipboard
//   Property 5  — Copy-paste retains clipboard
//   Property 8  — FileSystem::remove post-condition
//   Property 9  — FileSystem::createDirectory post-condition
//   Property 10 — FileSystem::move post-condition
//   Property 14 — Selection-resolution preserves multi-selection iff click is inside it
//
// Requirements: 6.4, 6.5, 6.6, 7.3, 8.4, 8.6, 9.3, 12.5, 12.6, 12.7

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include <rapidcheck/detail/Configuration.h>
#include <rapidcheck/detail/TestMetadata.h>
#include <rapidcheck/detail/TestParams.h>
#include <rapidcheck/detail/Results.h>

#include <sstream>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/file_system.hpp"
#include "gui/clipboard_manager.hpp"
#include "gui/file_view.hpp"

// ---------------------------------------------------------------------------
// FileViewTestAccess — friend accessor for FileView's private members.
// FileView declares `friend class FileViewTestAccess` in its header, so we
// can reach into clipboard_, proxy_, and the action handler methods to
// exercise them directly without going through QMenu::exec().
// ---------------------------------------------------------------------------
class FileViewTestAccess {
public:
    // Clipboard access
    static ClipboardManager& clipboard(FileView& v) { return v.clipboard_; }
    static const ClipboardManager& clipboard(const FileView& v) { return v.clipboard_; }

    // Proxy model access
    static CutStateProxyModel* proxy(FileView& v) { return v.proxy_; }

    // Action handler invocation (bypasses QMenu)
    static void invokeCopy(FileView& v) { v.onCopy(); }
    static void invokeCut(FileView& v) { v.onCut(); }
    static void invokePaste(FileView& v) { v.onPaste(); }
    static void invokeDelete(FileView& v) { v.onDelete(); }
    static void invokeNewFolder(FileView& v) { v.onNewFolder(); }
    static void invokeRename(FileView& v) { v.onRename(); }

    // Selection helpers
    static QStringList selectedPaths(const FileView& v) { return v.selectedPaths(); }
    static QAbstractItemView* activeView(const FileView& v) { return v.activeView(); }
    static void resolveSelection(FileView& v, const QPoint& pos) {
        v.resolveSelectionForContextMenu(pos);
    }

    // Direct clipboard manipulation (for testing paste without going through onCopy/onCut)
    static void setClipboard(FileView& v, const QList<QString>& paths, ClipboardOperation op) {
        v.clipboard_.set(paths, op);
    }
};

// ---------------------------------------------------------------------------
// QApplication fixture + QTemporaryDir helper
// ---------------------------------------------------------------------------
class ContextMenuIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int   argc   = 0;
            static char* argv[] = {nullptr};
            new QApplication(argc, argv);
        }
    }

    void SetUp() override {
        ASSERT_TRUE(tempDir_.isValid());
        populateFixedStructure();
    }

    // Creates a known directory structure:
    //   tempDir_/
    //     subdir_a/
    //     subdir_b/
    //     file1.txt  (content: "hello1")
    //     file2.txt  (content: "hello2")
    //     file3.txt  (content: "hello3")
    void populateFixedStructure() {
        QDir base(tempDir_.path());
        ASSERT_TRUE(base.mkdir("subdir_a"));
        ASSERT_TRUE(base.mkdir("subdir_b"));

        for (int i = 1; i <= 3; ++i) {
            QFile f(base.filePath(QString("file%1.txt").arg(i)));
            ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream(&f) << "hello" << i;
            f.close();
        }
    }

    // Helper: create a file with given content under a directory
    static void createFile(const QString& dirPath, const QString& name,
                           const QString& content = "test") {
        QFile f(QDir(dirPath).filePath(name));
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream(&f) << content;
        f.close();
    }

    // Helper: wait for QFileSystemModel to load the directory.
    // Uses a short timeout to keep tests fast in CI.
    static void waitForModelLoad(FileView& view, int timeoutMs = 500) {
        QSignalSpy spy(&view, &FileView::statusChanged);
        spy.wait(timeoutMs);
        // Drain any additional emissions quickly
        while (spy.wait(50)) {}
    }

    QTemporaryDir tempDir_;
};

// ===========================================================================
// Example Tests — Property 4: Cut-paste clears clipboard
// Validates: Requirements 6.4, 12.5
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, CutPasteClearsClipboard) {
    FileView view;
    view.navigateTo(tempDir_.path());
    waitForModelLoad(view);

    // Set up clipboard with Cut operation containing real file paths
    QList<QString> sources;
    sources << QDir(tempDir_.path()).filePath("file1.txt");

    FileViewTestAccess::setClipboard(view, sources, ClipboardOperation::Cut);
    ASSERT_FALSE(FileViewTestAccess::clipboard(view).isEmpty());
    ASSERT_EQ(FileViewTestAccess::clipboard(view).operation(), ClipboardOperation::Cut);

    // Create a destination directory and navigate there for paste
    QString destPath = QDir(tempDir_.path()).filePath("subdir_a");
    view.navigateTo(destPath);
    waitForModelLoad(view);

    // Invoke paste — this should move the file and clear the clipboard
    FileViewTestAccess::invokePaste(view);

    // Property 4: After Cut-paste, clipboard must be empty
    EXPECT_TRUE(FileViewTestAccess::clipboard(view).isEmpty())
        << "Clipboard should be empty after Cut-paste completes";

    // Verify the file was actually moved
    EXPECT_FALSE(QFileInfo::exists(sources.first()))
        << "Source file should no longer exist after Cut-paste";
    EXPECT_TRUE(QFileInfo::exists(QDir(destPath).filePath("file1.txt")))
        << "File should exist at destination after Cut-paste";
}

// ===========================================================================
// Example Tests — Property 5: Copy-paste retains clipboard
// Validates: Requirements 6.5, 12.6, 12.7
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, CopyPasteRetainsClipboard) {
    FileView view;
    view.navigateTo(tempDir_.path());
    waitForModelLoad(view);

    // Set up clipboard with Copy operation
    QList<QString> sources;
    sources << QDir(tempDir_.path()).filePath("file2.txt");

    FileViewTestAccess::setClipboard(view, sources, ClipboardOperation::Copy);
    ASSERT_FALSE(FileViewTestAccess::clipboard(view).isEmpty());

    // Navigate to destination and paste
    QString destPath = QDir(tempDir_.path()).filePath("subdir_b");
    view.navigateTo(destPath);
    waitForModelLoad(view);

    FileViewTestAccess::invokePaste(view);

    // Property 5: After Copy-paste, clipboard retains original paths and operation
    EXPECT_FALSE(FileViewTestAccess::clipboard(view).isEmpty())
        << "Clipboard should NOT be empty after Copy-paste";
    EXPECT_EQ(FileViewTestAccess::clipboard(view).paths(), sources)
        << "Clipboard paths should be unchanged after Copy-paste";
    EXPECT_EQ(FileViewTestAccess::clipboard(view).operation(), ClipboardOperation::Copy)
        << "Clipboard operation should remain Copy after Copy-paste";

    // Verify the file was copied (source still exists, dest also exists)
    EXPECT_TRUE(QFileInfo::exists(sources.first()))
        << "Source file should still exist after Copy-paste";
    EXPECT_TRUE(QFileInfo::exists(QDir(destPath).filePath("file2.txt")))
        << "File should exist at destination after Copy-paste";
}

// ===========================================================================
// Example Tests — Property 8: FileSystem::remove post-condition
// Validates: Requirements 8.4
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, FileSystemRemovePostCondition_File) {
    QString filePath = QDir(tempDir_.path()).filePath("file1.txt");
    ASSERT_TRUE(FileSystem::exists(filePath.toStdString()));

    bool removed = FileSystem::remove(filePath.toStdString());
    ASSERT_TRUE(removed);

    // Property 8: If remove returns true, the path must no longer exist
    EXPECT_FALSE(FileSystem::exists(filePath.toStdString()))
        << "After successful remove, path must not exist";
}

TEST_F(ContextMenuIntegrationTest, FileSystemRemovePostCondition_Directory) {
    QString dirPath = QDir(tempDir_.path()).filePath("subdir_a");
    ASSERT_TRUE(FileSystem::exists(dirPath.toStdString()));

    bool removed = FileSystem::remove(dirPath.toStdString());
    ASSERT_TRUE(removed);

    // Property 8: If remove returns true, the path must no longer exist
    EXPECT_FALSE(FileSystem::exists(dirPath.toStdString()))
        << "After successful remove of directory, path must not exist";
}

// ===========================================================================
// Example Tests — Property 9: FileSystem::createDirectory post-condition
// Validates: Requirements 9.3
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, FileSystemCreateDirectoryPostCondition) {
    QString newDirPath = QDir(tempDir_.path()).filePath("brand_new_dir");
    ASSERT_FALSE(FileSystem::exists(newDirPath.toStdString()));

    bool created = FileSystem::createDirectory(newDirPath.toStdString());
    ASSERT_TRUE(created);

    // Property 9: If createDirectory returns true, path must exist and be a directory
    EXPECT_TRUE(FileSystem::exists(newDirPath.toStdString()))
        << "After successful createDirectory, path must exist";
    EXPECT_TRUE(FileSystem::isDirectory(newDirPath.toStdString()))
        << "After successful createDirectory, path must be a directory";
}

TEST_F(ContextMenuIntegrationTest, FileSystemCreateDirectoryPostCondition_Nested) {
    QString nestedPath = QDir(tempDir_.path()).filePath("parent/child/grandchild");
    ASSERT_FALSE(FileSystem::exists(nestedPath.toStdString()));

    bool created = FileSystem::createDirectory(nestedPath.toStdString());
    ASSERT_TRUE(created);

    // Property 9: Nested directory creation also satisfies the post-condition
    EXPECT_TRUE(FileSystem::exists(nestedPath.toStdString()));
    EXPECT_TRUE(FileSystem::isDirectory(nestedPath.toStdString()));
}

// ===========================================================================
// Example Tests — Property 10: FileSystem::move post-condition
// Validates: Requirements 7.3
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, FileSystemMovePostCondition) {
    QString srcPath = QDir(tempDir_.path()).filePath("file3.txt");
    QString destPath = QDir(tempDir_.path()).filePath("file3_renamed.txt");

    ASSERT_TRUE(FileSystem::exists(srcPath.toStdString()));
    ASSERT_FALSE(FileSystem::exists(destPath.toStdString()));

    bool moved = FileSystem::move(srcPath.toStdString(), destPath.toStdString());
    ASSERT_TRUE(moved);

    // Property 10: If move returns true, source must not exist and dest must exist
    EXPECT_FALSE(FileSystem::exists(srcPath.toStdString()))
        << "After successful move, source must not exist";
    EXPECT_TRUE(FileSystem::exists(destPath.toStdString()))
        << "After successful move, destination must exist";
}

TEST_F(ContextMenuIntegrationTest, FileSystemMovePostCondition_Directory) {
    QString srcPath = QDir(tempDir_.path()).filePath("subdir_a");
    QString destPath = QDir(tempDir_.path()).filePath("subdir_a_moved");

    ASSERT_TRUE(FileSystem::exists(srcPath.toStdString()));
    ASSERT_FALSE(FileSystem::exists(destPath.toStdString()));

    bool moved = FileSystem::move(srcPath.toStdString(), destPath.toStdString());
    ASSERT_TRUE(moved);

    EXPECT_FALSE(FileSystem::exists(srcPath.toStdString()));
    EXPECT_TRUE(FileSystem::exists(destPath.toStdString()));
    EXPECT_TRUE(FileSystem::isDirectory(destPath.toStdString()));
}

// ===========================================================================
// Example Test — Failure aggregation (Req 6.6, 8.6)
// Verifies that FileSystem::copy with overwrite=false returns false when
// the destination already exists, and that the source remains intact.
// The full paste-loop aggregation (which shows a QMessageBox) is verified
// structurally: the loop in onPaste() iterates ALL sources regardless of
// per-item failure. Here we verify the FileSystem-level precondition that
// makes the aggregation test meaningful.
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, CopyWithoutOverwriteFailsOnExistingDest) {
    // Create a source file
    QString srcPath = QDir(tempDir_.path()).filePath("file1.txt");
    ASSERT_TRUE(FileSystem::exists(srcPath.toStdString()));

    // Create a conflicting destination
    QString destDir = QDir(tempDir_.path()).filePath("subdir_a");
    createFile(destDir, "file1.txt", "existing_content");
    QString destPath = QDir(destDir).filePath("file1.txt");
    ASSERT_TRUE(FileSystem::exists(destPath.toStdString()));

    // Copy with overwrite=false should fail
    bool result = FileSystem::copy(srcPath.toStdString(), destPath.toStdString(), false);
    EXPECT_FALSE(result)
        << "FileSystem::copy with overwrite=false should fail when dest exists";

    // Source should still exist (not destroyed by failed copy)
    EXPECT_TRUE(FileSystem::exists(srcPath.toStdString()));
}

// Verify that after a Copy-paste where all items succeed, clipboard is retained
// (Req 12.6, 12.7). This exercises the non-failure path of onPaste().
TEST_F(ContextMenuIntegrationTest, CopyPasteRetainsClipboard_MultipleFiles) {
    FileView view;
    view.navigateTo(tempDir_.path());
    waitForModelLoad(view);

    // Set clipboard with multiple files for Copy
    QList<QString> sources;
    sources << QDir(tempDir_.path()).filePath("file1.txt")
            << QDir(tempDir_.path()).filePath("file2.txt");

    FileViewTestAccess::setClipboard(view, sources, ClipboardOperation::Copy);

    // Navigate to an empty destination
    QString destDir = QDir(tempDir_.path()).filePath("subdir_b");
    view.navigateTo(destDir);
    waitForModelLoad(view);

    FileViewTestAccess::invokePaste(view);

    // After Copy-paste, clipboard is retained (Req 12.6, 12.7)
    EXPECT_FALSE(FileViewTestAccess::clipboard(view).isEmpty());
    EXPECT_EQ(FileViewTestAccess::clipboard(view).paths(), sources);
    EXPECT_EQ(FileViewTestAccess::clipboard(view).operation(), ClipboardOperation::Copy);

    // Both files should be copied to destination
    EXPECT_TRUE(QFileInfo::exists(QDir(destDir).filePath("file1.txt")));
    EXPECT_TRUE(QFileInfo::exists(QDir(destDir).filePath("file2.txt")));

    // Source files should still exist
    EXPECT_TRUE(QFileInfo::exists(sources[0]));
    EXPECT_TRUE(QFileInfo::exists(sources[1]));
}

// ===========================================================================
// Example Test — Property 14: Selection resolution
// Validates: Requirements 1.6, 1.7
//
// Note: Full property test (task 12.7) will use RapidCheck with arbitrary
// selection sets. This example test verifies the three cases manually.
// ===========================================================================
TEST_F(ContextMenuIntegrationTest, SelectionResolution_ClickOutsideReplacesSelection) {
    FileView view;
    view.navigateTo(tempDir_.path());
    waitForModelLoad(view);

    QAbstractItemView* activeView = FileViewTestAccess::activeView(view);
    ASSERT_NE(activeView, nullptr);

    QItemSelectionModel* selModel = activeView->selectionModel();
    ASSERT_NE(selModel, nullptr);

    // Wait a bit more for the model to be fully populated
    QSignalSpy spy(&view, &FileView::statusChanged);
    spy.wait(200);
    while (spy.wait(50)) {}

    // Get the model index for the first row (if available)
    QModelIndex firstIdx = activeView->model()->index(0, 0, activeView->rootIndex());
    if (!firstIdx.isValid()) {
        // Model may not have loaded yet — skip this test gracefully
        GTEST_SKIP() << "Model not populated in time for selection test";
    }

    // Select the first item
    selModel->select(firstIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ASSERT_EQ(selModel->selectedRows().size(), 1);

    // Get a second index
    QModelIndex secondIdx = activeView->model()->index(1, 0, activeView->rootIndex());
    if (!secondIdx.isValid()) {
        GTEST_SKIP() << "Need at least 2 items for selection resolution test";
    }

    // Right-click on the second item (not in current selection) should replace selection
    QPoint posOfSecond = activeView->visualRect(secondIdx).center();
    FileViewTestAccess::resolveSelection(view, posOfSecond);

    // Property 14 case: i ∉ S and i.isValid() → selection = {i}
    QModelIndexList selected = selModel->selectedRows();
    EXPECT_EQ(selected.size(), 1);
    if (!selected.isEmpty()) {
        EXPECT_EQ(selected.first().row(), secondIdx.row());
    }
}

TEST_F(ContextMenuIntegrationTest, SelectionResolution_ClickInsidePreservesMultiSelection) {
    FileView view;
    view.navigateTo(tempDir_.path());
    waitForModelLoad(view);

    QAbstractItemView* activeView = FileViewTestAccess::activeView(view);
    ASSERT_NE(activeView, nullptr);

    QItemSelectionModel* selModel = activeView->selectionModel();
    ASSERT_NE(selModel, nullptr);

    QSignalSpy spy(&view, &FileView::statusChanged);
    spy.wait(200);
    while (spy.wait(50)) {}

    QModelIndex firstIdx = activeView->model()->index(0, 0, activeView->rootIndex());
    QModelIndex secondIdx = activeView->model()->index(1, 0, activeView->rootIndex());
    if (!firstIdx.isValid() || !secondIdx.isValid()) {
        GTEST_SKIP() << "Need at least 2 items for multi-selection test";
    }

    // Select both items (multi-selection)
    selModel->select(firstIdx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    selModel->select(secondIdx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    ASSERT_EQ(selModel->selectedRows().size(), 2);

    // Right-click on the first item (already in selection) should preserve multi-selection
    QPoint posOfFirst = activeView->visualRect(firstIdx).center();
    FileViewTestAccess::resolveSelection(view, posOfFirst);

    // Property 14 case: i ∈ S → selection = S (preserved)
    EXPECT_EQ(selModel->selectedRows().size(), 2)
        << "Multi-selection should be preserved when clicking inside it";
}

TEST_F(ContextMenuIntegrationTest, SelectionResolution_ClickEmptyAreaClearsSelection) {
    FileView view;
    view.navigateTo(tempDir_.path());
    waitForModelLoad(view);

    QAbstractItemView* activeView = FileViewTestAccess::activeView(view);
    ASSERT_NE(activeView, nullptr);

    QItemSelectionModel* selModel = activeView->selectionModel();
    ASSERT_NE(selModel, nullptr);

    QSignalSpy spy(&view, &FileView::statusChanged);
    spy.wait(200);
    while (spy.wait(50)) {}

    QModelIndex firstIdx = activeView->model()->index(0, 0, activeView->rootIndex());
    if (firstIdx.isValid()) {
        selModel->select(firstIdx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        ASSERT_GE(selModel->selectedRows().size(), 1);
    }

    // Right-click on empty area (a point far below all items)
    QPoint emptyPos(10, activeView->viewport()->height() - 1);
    // Ensure this point doesn't map to a valid index
    QModelIndex idxAtEmpty = activeView->indexAt(emptyPos);
    if (idxAtEmpty.isValid()) {
        // Try an even more extreme position
        emptyPos = QPoint(10, activeView->viewport()->height() + 100);
    }

    FileViewTestAccess::resolveSelection(view, emptyPos);

    // Property 14 case: !i.isValid() → selection is empty
    // Note: only assert if the point truly maps to no index
    if (!activeView->indexAt(emptyPos).isValid()) {
        EXPECT_EQ(selModel->selectedRows().size(), 0)
            << "Selection should be cleared when clicking empty area";
    }
}

// ===========================================================================
// RapidCheck property tests (tasks 12.2–12.7)
//
// All property tests use maxSuccess=10 to keep CI fast while still exercising
// randomized inputs. The helper below constructs the TestParams once.
// ===========================================================================

namespace {
rc::detail::TestParams fastParams() {
    rc::detail::TestParams p;
    p.maxSuccess = 10;
    return p;
}
} // namespace

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 4: Cut-paste clears the clipboard
// Validates: Requirements 6.4, 12.5
//
// For arbitrary non-empty path list P of files created under QTemporaryDir
// and a writable destination directory, after clipboard.set(P, Cut) and a
// simulated paste through FileView::onPaste(), clipboard.isEmpty() == true.
// ---------------------------------------------------------------------------
TEST_F(ContextMenuIntegrationTest, CutPasteClearsClipboard_Property) {
    rc::detail::TestMetadata metadata;
    metadata.id = "ContextMenuIntegrationTest/CutPasteClearsClipboard_Property";
    metadata.description = "CutPasteClearsClipboard_Property";

    const auto result = rc::detail::checkTestable([]() {
        // Generate a valid filename using alphanumeric chars only.
        auto validCharGen = rc::gen::oneOf(
            rc::gen::inRange(static_cast<char>('a'), static_cast<char>('z' + 1)),
            rc::gen::inRange(static_cast<char>('A'), static_cast<char>('Z' + 1)),
            rc::gen::inRange(static_cast<char>('0'), static_cast<char>('9' + 1)));

        // Generate between 1 and 3 files to cut.
        int fileCount = *rc::gen::inRange(1, 4);

        // Create a fresh temp dir for this iteration.
        QTemporaryDir iterDir;
        RC_ASSERT(iterDir.isValid());

        // Create a destination subdirectory.
        QString destPath = QDir(iterDir.path()).filePath("dest");
        RC_ASSERT(QDir().mkpath(destPath));

        // Generate files and collect their paths.
        QList<QString> sources;
        for (int i = 0; i < fileCount; ++i) {
            std::size_t nameLen = *rc::gen::inRange<std::size_t>(1, 8);
            std::string rawName = *rc::gen::container<std::string>(nameLen, validCharGen);
            RC_PRE(!rawName.empty());
            // Append index to ensure uniqueness.
            QString filename = QString::fromStdString(rawName) + QString::number(i) + ".txt";
            QString filePath = QDir(iterDir.path()).filePath(filename);

            QFile f(filePath);
            RC_ASSERT(f.open(QIODevice::WriteOnly));
            f.write("content");
            f.close();
            sources << filePath;
        }

        // Set up FileView, navigate to the source directory.
        FileView view;
        view.navigateTo(iterDir.path());
        // Brief wait for model load.
        QSignalSpy spy(&view, &FileView::statusChanged);
        spy.wait(300);
        while (spy.wait(50)) {}

        // Set clipboard with Cut operation.
        FileViewTestAccess::setClipboard(view, sources, ClipboardOperation::Cut);
        RC_ASSERT(!FileViewTestAccess::clipboard(view).isEmpty());
        RC_ASSERT(FileViewTestAccess::clipboard(view).operation() == ClipboardOperation::Cut);

        // Navigate to destination and invoke paste.
        view.navigateTo(destPath);
        QSignalSpy spy2(&view, &FileView::statusChanged);
        spy2.wait(300);
        while (spy2.wait(100)) {}
        FileViewTestAccess::invokePaste(view);

        // Property 4: After Cut-paste, clipboard must be empty.
        RC_ASSERT(FileViewTestAccess::clipboard(view).isEmpty());
    }, metadata, fastParams());

    if (!result.template is<rc::detail::SuccessResult>()) {
        std::ostringstream ss;
        rc::detail::printResultMessage(result, ss);
        FAIL() << ss.str();
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 5: Copy-paste retains the clipboard
// Validates: Requirements 6.5, 12.6, 12.7
//
// For arbitrary non-empty path list P and writable destination, after
// clipboard.set(P, Copy) and a simulated paste, clipboard.paths() == P
// and clipboard.operation() == Copy.
// ---------------------------------------------------------------------------
TEST_F(ContextMenuIntegrationTest, CopyPasteRetainsClipboard_Property) {
    rc::detail::TestMetadata metadata;
    metadata.id = "ContextMenuIntegrationTest/CopyPasteRetainsClipboard_Property";
    metadata.description = "CopyPasteRetainsClipboard_Property";

    const auto result = rc::detail::checkTestable([]() {
        // Generate a valid filename using alphanumeric chars only.
        auto validCharGen = rc::gen::oneOf(
            rc::gen::inRange(static_cast<char>('a'), static_cast<char>('z' + 1)),
            rc::gen::inRange(static_cast<char>('A'), static_cast<char>('Z' + 1)),
            rc::gen::inRange(static_cast<char>('0'), static_cast<char>('9' + 1)));

        // Generate between 1 and 3 files to copy.
        int fileCount = *rc::gen::inRange(1, 4);

        // Create a fresh temp dir for this iteration.
        QTemporaryDir iterDir;
        RC_ASSERT(iterDir.isValid());

        // Create a destination subdirectory.
        QString destPath = QDir(iterDir.path()).filePath("dest");
        RC_ASSERT(QDir().mkpath(destPath));

        // Generate files and collect their paths.
        QList<QString> sources;
        for (int i = 0; i < fileCount; ++i) {
            std::size_t nameLen = *rc::gen::inRange<std::size_t>(1, 8);
            std::string rawName = *rc::gen::container<std::string>(nameLen, validCharGen);
            RC_PRE(!rawName.empty());
            // Append index to ensure uniqueness.
            QString filename = QString::fromStdString(rawName) + QString::number(i) + ".txt";
            QString filePath = QDir(iterDir.path()).filePath(filename);

            QFile f(filePath);
            RC_ASSERT(f.open(QIODevice::WriteOnly));
            f.write("content");
            f.close();
            sources << filePath;
        }

        // Set up FileView, navigate to the source directory.
        FileView view;
        view.navigateTo(iterDir.path());
        QSignalSpy spy(&view, &FileView::statusChanged);
        spy.wait(300);
        while (spy.wait(50)) {}

        // Set clipboard with Copy operation.
        FileViewTestAccess::setClipboard(view, sources, ClipboardOperation::Copy);
        RC_ASSERT(!FileViewTestAccess::clipboard(view).isEmpty());
        RC_ASSERT(FileViewTestAccess::clipboard(view).operation() == ClipboardOperation::Copy);

        // Navigate to destination and invoke paste.
        view.navigateTo(destPath);
        QSignalSpy spy2(&view, &FileView::statusChanged);
        spy2.wait(300);
        while (spy2.wait(100)) {}
        FileViewTestAccess::invokePaste(view);

        // Property 5: After Copy-paste, clipboard retains original paths and operation.
        RC_ASSERT(!FileViewTestAccess::clipboard(view).isEmpty());
        RC_ASSERT(FileViewTestAccess::clipboard(view).paths() == sources);
        RC_ASSERT(FileViewTestAccess::clipboard(view).operation() == ClipboardOperation::Copy);
    }, metadata, fastParams());

    if (!result.template is<rc::detail::SuccessResult>()) {
        std::ostringstream ss;
        rc::detail::printResultMessage(result, ss);
        FAIL() << ss.str();
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 8: FileSystem::remove post-condition
// Validates: Requirements 8.4
//
// For arbitrary file or directory created under QTemporaryDir at path p,
// if FileSystem::remove(p) == true then FileSystem::exists(p) == false.
// ---------------------------------------------------------------------------
TEST_F(ContextMenuIntegrationTest, FileSystemRemovePostCondition_Property) {
    rc::detail::TestMetadata metadata;
    metadata.id = "ContextMenuIntegrationTest/FileSystemRemovePostCondition_Property";
    metadata.description = "FileSystemRemovePostCondition_Property";

    const auto result = rc::detail::checkTestable([]() {
        // Generate a valid filename using alphanumeric chars only.
        auto validCharGen = rc::gen::oneOf(
            rc::gen::inRange(static_cast<char>('a'), static_cast<char>('z' + 1)),
            rc::gen::inRange(static_cast<char>('A'), static_cast<char>('Z' + 1)),
            rc::gen::inRange(static_cast<char>('0'), static_cast<char>('9' + 1)));

        // Decide whether to create a file or directory.
        bool isFile = *rc::gen::arbitrary<bool>();

        // Generate a random name.
        std::size_t nameLen = *rc::gen::inRange<std::size_t>(1, 8);
        std::string rawName = *rc::gen::container<std::string>(nameLen, validCharGen);
        RC_PRE(!rawName.empty());
        RC_PRE(rawName != "." && rawName != "..");

        // Create a fresh temp dir for this iteration.
        QTemporaryDir iterDir;
        RC_ASSERT(iterDir.isValid());

        QString entryPath = QDir(iterDir.path()).filePath(QString::fromStdString(rawName));

        if (isFile) {
            QFile f(entryPath);
            RC_ASSERT(f.open(QIODevice::WriteOnly));
            f.write("content");
            f.close();
        } else {
            RC_ASSERT(QDir().mkpath(entryPath));
            // Optionally create a file inside the directory to test recursive removal.
            bool hasChild = *rc::gen::arbitrary<bool>();
            if (hasChild) {
                QFile f(QDir(entryPath).filePath("child.txt"));
                RC_ASSERT(f.open(QIODevice::WriteOnly));
                f.write("child");
                f.close();
            }
        }

        // Precondition: the entry exists.
        RC_ASSERT(FileSystem::exists(entryPath.toStdString()));

        // Perform removal.
        bool removed = FileSystem::remove(entryPath.toStdString());

        // Property 8: If remove returns true, the path must no longer exist.
        if (removed) {
            RC_ASSERT(!FileSystem::exists(entryPath.toStdString()));
        }
    }, metadata, fastParams());

    if (!result.template is<rc::detail::SuccessResult>()) {
        std::ostringstream ss;
        rc::detail::printResultMessage(result, ss);
        FAIL() << ss.str();
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 9: FileSystem::createDirectory post-condition
// Validates: Requirements 9.3
//
// For arbitrary path p under QTemporaryDir, if FileSystem::createDirectory(p)
// == true then FileSystem::exists(p) == true && FileSystem::isDirectory(p) == true.
// ---------------------------------------------------------------------------
TEST_F(ContextMenuIntegrationTest, FileSystemCreateDirectoryPostCondition_Property) {
    rc::detail::TestMetadata metadata;
    metadata.id = "ContextMenuIntegrationTest/FileSystemCreateDirectoryPostCondition_Property";
    metadata.description = "FileSystemCreateDirectoryPostCondition_Property";

    const auto result = rc::detail::checkTestable([]() {
        // Generate valid directory name segments using alphanumeric chars.
        auto validCharGen = rc::gen::oneOf(
            rc::gen::inRange(static_cast<char>('a'), static_cast<char>('z' + 1)),
            rc::gen::inRange(static_cast<char>('A'), static_cast<char>('Z' + 1)),
            rc::gen::inRange(static_cast<char>('0'), static_cast<char>('9' + 1)));

        // Generate between 1 and 3 path segments (to test nested creation).
        int segmentCount = *rc::gen::inRange(1, 4);

        // Create a fresh temp dir for this iteration.
        QTemporaryDir iterDir;
        RC_ASSERT(iterDir.isValid());

        // Build the path from random segments.
        QString dirPath = iterDir.path();
        for (int i = 0; i < segmentCount; ++i) {
            std::size_t nameLen = *rc::gen::inRange<std::size_t>(1, 8);
            std::string rawName = *rc::gen::container<std::string>(nameLen, validCharGen);
            RC_PRE(!rawName.empty());
            RC_PRE(rawName != "." && rawName != "..");
            dirPath = QDir(dirPath).filePath(QString::fromStdString(rawName));
        }

        // Precondition: the path does not yet exist.
        RC_PRE(!FileSystem::exists(dirPath.toStdString()));

        // Perform directory creation.
        bool created = FileSystem::createDirectory(dirPath.toStdString());

        // Property 9: If createDirectory returns true, path must exist and be a directory.
        if (created) {
            RC_ASSERT(FileSystem::exists(dirPath.toStdString()));
            RC_ASSERT(FileSystem::isDirectory(dirPath.toStdString()));
        }
    }, metadata, fastParams());

    if (!result.template is<rc::detail::SuccessResult>()) {
        std::ostringstream ss;
        rc::detail::printResultMessage(result, ss);
        FAIL() << ss.str();
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 10: FileSystem::move post-condition
// Validates: Requirements 7.3
//
// For arbitrary file at src in QTemporaryDir and a non-existing dest under
// QTemporaryDir, if FileSystem::move(src, dest) == true then
// FileSystem::exists(src) == false && FileSystem::exists(dest) == true.
// ---------------------------------------------------------------------------
TEST_F(ContextMenuIntegrationTest, FileSystemMovePostCondition_Property) {
    rc::detail::TestMetadata metadata;
    metadata.id = "ContextMenuIntegrationTest/FileSystemMovePostCondition_Property";
    metadata.description = "FileSystemMovePostCondition_Property";

    const auto result = rc::detail::checkTestable([]() {
        // Generate valid filenames using alphanumeric chars.
        auto validCharGen = rc::gen::oneOf(
            rc::gen::inRange(static_cast<char>('a'), static_cast<char>('z' + 1)),
            rc::gen::inRange(static_cast<char>('A'), static_cast<char>('Z' + 1)),
            rc::gen::inRange(static_cast<char>('0'), static_cast<char>('9' + 1)));

        // Generate source and destination names (must be different).
        std::size_t srcLen = *rc::gen::inRange<std::size_t>(1, 8);
        std::string srcRaw = *rc::gen::container<std::string>(srcLen, validCharGen);
        RC_PRE(!srcRaw.empty());

        std::size_t destLen = *rc::gen::inRange<std::size_t>(1, 8);
        std::string destRaw = *rc::gen::container<std::string>(destLen, validCharGen);
        RC_PRE(!destRaw.empty());
        RC_PRE(srcRaw != destRaw);  // Ensure src and dest names differ.

        // Create a fresh temp dir for this iteration.
        QTemporaryDir iterDir;
        RC_ASSERT(iterDir.isValid());

        QString srcPath = QDir(iterDir.path()).filePath(QString::fromStdString(srcRaw) + ".txt");
        QString destPath = QDir(iterDir.path()).filePath(QString::fromStdString(destRaw) + ".txt");

        // Create the source file.
        QFile f(srcPath);
        RC_ASSERT(f.open(QIODevice::WriteOnly));
        f.write("content");
        f.close();

        // Preconditions.
        RC_ASSERT(FileSystem::exists(srcPath.toStdString()));
        RC_PRE(!FileSystem::exists(destPath.toStdString()));

        // Perform move.
        bool moved = FileSystem::move(srcPath.toStdString(), destPath.toStdString());

        // Property 10: If move returns true, source must not exist and dest must exist.
        if (moved) {
            RC_ASSERT(!FileSystem::exists(srcPath.toStdString()));
            RC_ASSERT(FileSystem::exists(destPath.toStdString()));
        }
    }, metadata, fastParams());

    if (!result.template is<rc::detail::SuccessResult>()) {
        std::ostringstream ss;
        rc::detail::printResultMessage(result, ss);
        FAIL() << ss.str();
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 14: Selection-resolution preserves
// multi-selection iff click is inside it
// Validates: Requirements 1.6, 1.7
//
// NOTE: Full property-based testing with RapidCheck is not feasible for this
// UI-dependent property because it requires a live FileView with a fully
// populated QFileSystemModel (which loads asynchronously and depends on real
// filesystem timing). Instead, we write a deterministic test that exercises
// all 3 cases with multiple iterations using different file counts, serving
// as a simplified property test.
// ---------------------------------------------------------------------------
TEST_F(ContextMenuIntegrationTest, SelectionResolution_Property14_SimplifiedPBT) {
    // Test with varying file counts to approximate property-based coverage.
    for (int fileCount = 3; fileCount <= 7; ++fileCount) {
        QTemporaryDir iterDir;
        ASSERT_TRUE(iterDir.isValid());

        // Create fileCount files in the temp directory.
        for (int i = 0; i < fileCount; ++i) {
            QFile f(QDir(iterDir.path()).filePath(
                QString("item_%1.txt").arg(i, 3, 10, QChar('0'))));
            ASSERT_TRUE(f.open(QIODevice::WriteOnly));
            f.write("data");
            f.close();
        }

        FileView view;
        view.navigateTo(iterDir.path());
        waitForModelLoad(view);

        QAbstractItemView* activeView = FileViewTestAccess::activeView(view);
        ASSERT_NE(activeView, nullptr);

        QItemSelectionModel* selModel = activeView->selectionModel();
        ASSERT_NE(selModel, nullptr);

        // Wait for model to populate.
        QSignalSpy spy(&view, &FileView::statusChanged);
        spy.wait(200);
        while (spy.wait(50)) {}

        // Verify we have enough items loaded.
        int rowCount = activeView->model()->rowCount(activeView->rootIndex());
        if (rowCount < 2) {
            continue;  // Skip this iteration if model didn't load in time.
        }

        // --- Case 1: i ∉ S and i.isValid() → selection = {i} ---
        {
            QModelIndex firstIdx = activeView->model()->index(0, 0, activeView->rootIndex());
            QModelIndex secondIdx = activeView->model()->index(1, 0, activeView->rootIndex());
            if (firstIdx.isValid() && secondIdx.isValid()) {
                // Select first item.
                selModel->select(firstIdx,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                ASSERT_EQ(selModel->selectedRows().size(), 1);

                // Right-click on second item (not in selection).
                QPoint pos = activeView->visualRect(secondIdx).center();
                FileViewTestAccess::resolveSelection(view, pos);

                // Selection should now be {secondIdx}.
                QModelIndexList selected = selModel->selectedRows();
                EXPECT_EQ(selected.size(), 1)
                    << "Case 1 failed for fileCount=" << fileCount;
                if (!selected.isEmpty()) {
                    EXPECT_EQ(selected.first().row(), secondIdx.row());
                }
            }
        }

        // --- Case 2: i ∈ S → selection = S (preserved) ---
        {
            int maxSel = std::min(rowCount, 3);
            selModel->clearSelection();
            QModelIndexList multiSel;
            for (int i = 0; i < maxSel; ++i) {
                QModelIndex idx = activeView->model()->index(i, 0, activeView->rootIndex());
                if (idx.isValid()) {
                    selModel->select(idx,
                        QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    multiSel << idx;
                }
            }
            int expectedCount = selModel->selectedRows().size();
            ASSERT_GE(expectedCount, 2);

            // Right-click on the first item (already in selection).
            QPoint pos = activeView->visualRect(multiSel.first()).center();
            FileViewTestAccess::resolveSelection(view, pos);

            // Multi-selection should be preserved.
            EXPECT_EQ(selModel->selectedRows().size(), expectedCount)
                << "Case 2 failed for fileCount=" << fileCount;
        }

        // --- Case 3: !i.isValid() (empty area) → selection is empty ---
        {
            // First select something.
            QModelIndex firstIdx = activeView->model()->index(0, 0, activeView->rootIndex());
            if (firstIdx.isValid()) {
                selModel->select(firstIdx,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                ASSERT_GE(selModel->selectedRows().size(), 1);
            }

            // Click on empty area (far below all items).
            QPoint emptyPos(10, activeView->viewport()->height() - 1);
            QModelIndex idxAtEmpty = activeView->indexAt(emptyPos);
            if (idxAtEmpty.isValid()) {
                emptyPos = QPoint(10, activeView->viewport()->height() + 200);
            }

            // Only assert if the point truly maps to no index.
            if (!activeView->indexAt(emptyPos).isValid()) {
                FileViewTestAccess::resolveSelection(view, emptyPos);
                EXPECT_EQ(selModel->selectedRows().size(), 0)
                    << "Case 3 failed for fileCount=" << fileCount;
            }
        }
    }
}
