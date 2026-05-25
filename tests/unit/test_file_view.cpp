// tests/unit/test_file_view.cpp
// Unit tests for FileView construction invariants.
// Requirements: 1.4, 1.5, 8.1, 8.2

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <QApplication>
#include <QDir>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QSignalSpy>
#include <QTableView>
#include <QTemporaryDir>

#include "gui/file_view.hpp"

// ---------------------------------------------------------------------------
// FileViewTestAccess — white-box read accessor for FileView's private
// navigation state.  FileView declares this class as a friend (see
// include/gui/file_view.hpp), so we can reach into history_ and
// historyIndex_ to verify post-conditions that the public API doesn't
// expose directly.
//
// Used by property-based tests to assert invariants such as
//   history_.size() == historyIndex_ + 1  (Property 3, Req 3.3)
// without leaking those internals into FileView's public surface.
// ---------------------------------------------------------------------------
class FileViewTestAccess {
public:
    static int historyIndex(const FileView& v) { return v.historyIndex_; }
    static int historySize(const FileView& v)  { return v.history_.size(); }
    static QString historyAt(const FileView& v, int i) { return v.history_[i]; }
};

// ---------------------------------------------------------------------------
// QApplication fixture
// Qt widgets require a QApplication to be alive during construction.
// We share a single instance across all tests in this translation unit.
// ---------------------------------------------------------------------------
class FileViewTest : public ::testing::Test {
protected:
    // QApplication is created once per process; we store it statically so
    // that it outlives every individual test.
    static QApplication* app_;

    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int    argc = 0;
            static char*  argv[] = {nullptr};
            app_ = new QApplication(argc, argv);
        }
    }

    // Each test gets a freshly constructed FileView on the stack.
    FileView view_;
};

QApplication* FileViewTest::app_ = nullptr;

// ---------------------------------------------------------------------------
// Test: default view mode is TableMode (Req 1.4)
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, DefaultViewModeIsTableMode) {
    EXPECT_EQ(view_.currentViewMode(), ViewMode::Table);
}

// ---------------------------------------------------------------------------
// Test: currentPath() returns empty string before first navigation (Req 1.5)
//   historyIndex_ is -1 on construction; currentPath() must therefore
//   return an empty QString.
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, CurrentPathIsEmptyBeforeFirstNavigation) {
    EXPECT_TRUE(view_.currentPath().isEmpty());
}

// ---------------------------------------------------------------------------
// Test: tableView_ column widths and sort order match design spec (Req 8.1)
//   Access via QWidget::findChild<QTableView*>().
//   Expected widths: Name=200, Size=80, Type=80, Date Modified=150.
//   Expected sort: column 0, Qt::AscendingOrder.
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, TableViewColumnWidthsMatchDesignSpec) {
    auto* tableView = view_.findChild<QTableView*>();
    ASSERT_NE(tableView, nullptr) << "No QTableView child found in FileView";

    // Column widths — test the columns that were explicitly configured.
    // Column 3 (Date Modified) is stretched via setStretchLastSection(true),
    // so we only test columns 0–2 whose widths are fixed by setColumnWidth().
    EXPECT_EQ(tableView->columnWidth(0), 200) << "Name column width should be 200";
    EXPECT_EQ(tableView->columnWidth(1), 80)  << "Size column width should be 80";
    EXPECT_EQ(tableView->columnWidth(2), 80)  << "Type column width should be 80";
}

TEST_F(FileViewTest, TableViewSortOrderMatchesDesignSpec) {
    auto* tableView = view_.findChild<QTableView*>();
    ASSERT_NE(tableView, nullptr) << "No QTableView child found in FileView";

    auto* header = tableView->horizontalHeader();
    ASSERT_NE(header, nullptr);

    EXPECT_EQ(header->sortIndicatorSection(), 0)
        << "Sort indicator should be on column 0 (Name)";
    EXPECT_EQ(header->sortIndicatorOrder(), Qt::AscendingOrder)
        << "Default sort order should be ascending";
}

// ---------------------------------------------------------------------------
// Test: model filter flags include QDir::AllEntries | QDir::NoDotAndDotDot
//       | QDir::Hidden  (Req 8.2)
//   Access via QWidget::findChild<QFileSystemModel*>().
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, ModelFilterFlagsMatchDesignSpec) {
    auto* model = view_.findChild<QFileSystemModel*>();
    ASSERT_NE(model, nullptr) << "No QFileSystemModel child found in FileView";

    const QDir::Filters expectedFlags =
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden;

    EXPECT_EQ(model->filter(), expectedFlags)
        << "QFileSystemModel filter flags do not match design spec";
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 7: pathChanged signal carries the current path
//
// For any valid directory path p (chosen from a set of QTemporaryDir-created
// subdirectories), navigateTo(p) SHALL emit pathChanged exactly once with
// argument p.
//
// Validates: Requirements 4.1
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, PathChangedSignalCarriesCurrentPath, ()) {
    // Ensure a QApplication exists — required for Qt widget construction.
    // The FileViewTest fixture creates one if needed; if this property test
    // runs first we create one here instead (static, so constructed once).
    if (!QCoreApplication::instance()) {
        static int    argc = 0;
        static char*  argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // Build a small set of QTemporaryDir-created subdirectories to draw from.
    // tempDir must outlive the FileView (its QFileSystemModel watches paths),
    // so it is declared before the view (C++ destroys locals in LIFO order).
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    RC_ASSERT(base.mkdir("alpha"));
    RC_ASSERT(base.mkdir("beta"));
    RC_ASSERT(base.mkdir("gamma"));

    const std::vector<QString> validPaths = {
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
    };
    const QString p = *rc::gen::elementOf(validPaths);

    // Fresh FileView per property iteration — avoids accumulated history
    // from prior iterations affecting the spy's signal count.
    FileView view;
    QSignalSpy spy(&view, &FileView::pathChanged);

    view.navigateTo(p);

    // pathChanged must fire exactly once …
    RC_ASSERT(spy.count() == 1);
    // … and its sole argument must be the path we navigated to.
    RC_ASSERT(spy.takeFirst().at(0).toString() == p);
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 3: Forward history truncation on new navigation
//
// For any navigation sequence that places historyIndex_ before the end of
// history_ (achieved by navigating N >= 2 entries then calling goBack one
// or more times), calling navigateTo(newPath) SHALL result in:
//
//     history_.size() == historyIndex_ + 1
//
// i.e. all forward entries are discarded and newPath is appended.
//
// White-box access to history_ / historyIndex_ is granted via
// FileViewTestAccess (declared as friend in file_view.hpp).
//
// Validates: Requirements 3.3
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, ForwardHistoryTruncatedOnNewNavigation, ()) {
    // RC_GTEST_PROP expands to TEST (no fixture), so ensure a QApplication
    // exists before constructing any QWidget.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Generate the sequence shape:
    //   - N: number of initial navigateTo calls, in [2, 6]  (history needs
    //        at least 2 entries so that goBack can move us off the end).
    //   - backSteps: how many goBack() calls to issue, in [1, N-1]  (must be
    //        at least 1 to put historyIndex_ strictly below history_.size()-1,
    //        and at most N-1 because goBack() at index 0 is a no-op).
    // -----------------------------------------------------------------------
    const int N = *rc::gen::inRange(2, 7);
    const int backSteps = *rc::gen::inRange(1, N);  // [1, N-1]

    // -----------------------------------------------------------------------
    // Build N+1 real subdirectories under a fresh QTemporaryDir.  The first
    // N feed the initial navigateTo calls; the last one is the "fresh"
    // newPath used to trigger forward-history truncation.
    //
    // Declare tempDir before testView so testView's QFileSystemModel
    // watchers are torn down BEFORE the temp directories are deleted
    // (C++ local destruction order is reverse declaration order).
    // -----------------------------------------------------------------------
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    std::vector<QString> paths;
    paths.reserve(N + 1);
    for (int i = 0; i <= N; ++i) {
        const QString name = QString("dir%1").arg(i);
        RC_ASSERT(base.mkdir(name));
        paths.push_back(tempDir.path() + "/" + name);
    }

    // -----------------------------------------------------------------------
    // Apply the sequence to a fresh FileView.
    // -----------------------------------------------------------------------
    FileView view;

    // Step 1: Populate history with N entries.
    //   After this loop: history_.size() == N, historyIndex_ == N-1.
    for (int i = 0; i < N; ++i) {
        view.navigateTo(paths[i]);
    }
    RC_ASSERT(FileViewTestAccess::historySize(view) == N);
    RC_ASSERT(FileViewTestAccess::historyIndex(view) == N - 1);

    // Step 2: goBack() backSteps times.
    //   After this loop: historyIndex_ == N - 1 - backSteps, which is
    //   strictly less than history_.size() - 1 because backSteps >= 1.
    for (int i = 0; i < backSteps; ++i) {
        view.goBack();
    }
    const int idxAfterBack = FileViewTestAccess::historyIndex(view);
    RC_ASSERT(idxAfterBack == N - 1 - backSteps);
    RC_ASSERT(idxAfterBack < FileViewTestAccess::historySize(view) - 1);

    // Step 3: navigateTo a fresh path (paths[N], not yet in history).
    const QString& newPath = paths[N];
    view.navigateTo(newPath);

    // -----------------------------------------------------------------------
    // Property assertion:
    //
    //   history_.size() == historyIndex_ + 1
    //
    // The forward entries from idxAfterBack+1 onwards must have been
    // truncated, and newPath appended at the end.
    // -----------------------------------------------------------------------
    const int finalSize  = FileViewTestAccess::historySize(view);
    const int finalIndex = FileViewTestAccess::historyIndex(view);

    RC_ASSERT(finalSize == finalIndex + 1);
    // Sanity: the newly appended entry must be newPath.
    RC_ASSERT(FileViewTestAccess::historyAt(view, finalIndex) == newPath);
    // Equivalent observation through the public API.
    RC_ASSERT(view.currentPath() == newPath);
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 1: History index invariant
//
// For any sequence of navigateTo / goBack / goForward / goUp calls on valid
// paths, historyIndex_ SHALL always stay in the range [-1, history_.size()-1].
//
// Direct verification via FileViewTestAccess (friend class declared in
// include/gui/file_view.hpp) — no public-API inference required.
//
// Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5
// ---------------------------------------------------------------------------

// Enum encoding the 4 operations we want to generate sequences of.
enum class NavOp { NavigateTo, GoBack, GoForward, GoUp };

RC_GTEST_PROP(FileViewPropertyTests, HistoryIndexInvariant, ()) {
    // Ensure a QApplication exists — required for Qt widget construction.
    // The static object is created once per process (safe under RapidCheck's
    // per-iteration calls because the lambda captures a local reference).
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Set up: create a small real directory tree so navigateTo() validation
    // passes.  We use a QTemporaryDir scoped to the property iteration; each
    // RapidCheck iteration gets its own fresh tree to avoid state leakage.
    // Declare tempDir before 'view' so 'view' (and its QFileSystemModel
    // watchers) is destroyed before tempDir (C++ LIFO destruction order).
    // -----------------------------------------------------------------------
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    // Create a few sub-directories to give navigateTo() variety.
    QDir base(tempDir.path());
    base.mkdir("alpha");
    base.mkdir("beta");
    base.mkdir("gamma");

    // Pool of valid paths the generator can pick from.
    const std::vector<QString> validPaths = {
        tempDir.path(),
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
        QDir::tempPath(),
        QDir::homePath(),
    };

    // -----------------------------------------------------------------------
    // Generate a random sequence of operations (length 1–20).
    // First generate the count, then generate that many elements.
    // -----------------------------------------------------------------------
    const std::size_t opCount = *rc::gen::inRange<std::size_t>(1, 21);
    const auto ops = *rc::gen::container<std::vector<NavOp>>(
        opCount,
        rc::gen::element(
            NavOp::NavigateTo,
            NavOp::GoBack,
            NavOp::GoForward,
            NavOp::GoUp
        )
    );

    // -----------------------------------------------------------------------
    // Apply the sequence to a fresh FileView and assert the invariant after
    // every single step using direct read access to historyIndex_/history_.
    // -----------------------------------------------------------------------
    FileView view;

    // Pre-condition before any operation: empty history, index -1.
    RC_ASSERT(FileViewTestAccess::historyIndex(view) == -1);
    RC_ASSERT(FileViewTestAccess::historySize(view) == 0);

    for (const NavOp op : ops) {
        switch (op) {
            case NavOp::NavigateTo: {
                // Pick a random valid path from the pool.
                const QString path = *rc::gen::elementOf(validPaths);
                view.navigateTo(path);
                break;
            }
            case NavOp::GoBack:
                view.goBack();
                break;
            case NavOp::GoForward:
                view.goForward();
                break;
            case NavOp::GoUp:
                view.goUp();
                break;
        }

        // -----------------------------------------------------------------
        // Direct invariant check (Property 1):
        //   historyIndex_ ∈ [-1, history_.size() - 1]
        //
        // Equivalently:
        //   historyIndex_ >= -1
        //   historyIndex_ <  history_.size()    (size 0 ⇒ historyIndex_ == -1)
        // -----------------------------------------------------------------
        const int idx  = FileViewTestAccess::historyIndex(view);
        const int size = FileViewTestAccess::historySize(view);

        RC_ASSERT(idx >= -1);
        RC_ASSERT(idx <  size);          // when size==0 this requires idx<0, i.e. idx==-1
        RC_ASSERT(idx <= size - 1);      // explicit upper-bound form from the spec
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 2: Back/Forward navigation round-trip
//
// For any valid directory path p reached when history already has at least
// two entries, calling goBack() followed by goForward() SHALL return
// currentPath() to p.
//
// Sequence per iteration:
//   1. Create QTemporaryDir with several real subdirectories.
//   2. navigateTo a "previous" path so history has >= 2 entries.
//   3. navigateTo p (which becomes currentPath()).
//   4. Capture currentPath() — must equal p.
//   5. Call goBack().
//   6. Call goForward().
//   7. Assert currentPath() == p.
//
// Validates: Requirements 3.4, 3.5
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, BackForwardRoundTripReturnsToP, ()) {
    // RC_GTEST_PROP expands to a free TEST (no fixture), so make sure a
    // QApplication exists before we construct any QWidget.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Step 1: build a real temporary directory tree.  tempDir must outlive
    // the FileView (its QFileSystemModel watches paths), so it is declared
    // before the view (C++ destroys locals in reverse order).
    // -----------------------------------------------------------------------
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    RC_ASSERT(base.mkdir("alpha"));
    RC_ASSERT(base.mkdir("beta"));
    RC_ASSERT(base.mkdir("gamma"));
    RC_ASSERT(base.mkdir("delta"));

    const std::vector<QString> validPaths = {
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
        tempDir.path() + "/delta",
    };

    // Pick two distinct paths: 'previous' (anchors history) and p (target).
    // Indices are independent — the case prev == p is allowed and still
    // yields a valid history of size >= 2 (each navigateTo appends a new
    // entry regardless of value), so the round-trip property still applies.
    const QString previous =
        *rc::gen::elementOf(validPaths);
    const QString p =
        *rc::gen::elementOf(validPaths);

    // -----------------------------------------------------------------------
    // Step 2 & 3: navigate to 'previous' then to p.  After this, history
    // contains [previous, p] and historyIndex_ == 1.
    // -----------------------------------------------------------------------
    FileView view;

    view.navigateTo(previous);
    view.navigateTo(p);

    // -----------------------------------------------------------------------
    // Step 4: confirm we landed on p before exercising the round-trip.
    // -----------------------------------------------------------------------
    const QString captured = view.currentPath();
    RC_ASSERT(captured == p);

    // Sanity: history must have at least 2 entries for goBack to be a real
    // navigation rather than a no-op (the precondition of Property 2).
    RC_ASSERT(FileViewTestAccess::historySize(view) >= 2);

    // -----------------------------------------------------------------------
    // Step 5 & 6: round-trip — goBack() then goForward().
    // -----------------------------------------------------------------------
    view.goBack();
    view.goForward();

    // -----------------------------------------------------------------------
    // Step 7: currentPath() must be back to p.
    // -----------------------------------------------------------------------
    RC_ASSERT(view.currentPath() == p);
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 6: No-op guard at history bounds
//
// (a) goBack() on empty history (size 0, index -1) AND on a single-entry
//     history (size 1, index 0) SHALL leave currentPath(), historyIndex_,
//     and history_.size() unchanged.
//
// (b) goForward() called with historyIndex_ already at history_.size() - 1
//     SHALL leave currentPath(), historyIndex_, and history_.size()
//     unchanged regardless of how many times it is invoked.
//
// White-box state read via FileViewTestAccess (friend class of FileView).
//
// Validates: Requirements 3.7, 3.8
// ---------------------------------------------------------------------------

// Sub-property (a): goBack at the lower bound (empty or single-entry history)
//                   is a no-op.
RC_GTEST_PROP(FileViewPropertyTests, GoBackAtLowerBoundIsNoOp, ()) {
    // Ensure a QApplication exists — required for Qt widget construction.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Generate the case under test:
    //   entries == 0 → empty history (no navigation performed).
    //   entries == 1 → single-entry history (one navigateTo call).
    // -----------------------------------------------------------------------
    const int entries = *rc::gen::inRange(0, 2);  // {0, 1}

    // Build a small set of valid paths for the single-entry case.
    // tempDir must outlive the FileView (its QFileSystemModel watches paths),
    // so it is declared first; C++ destroys locals in LIFO order.
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    RC_ASSERT(base.mkdir("alpha"));
    RC_ASSERT(base.mkdir("beta"));
    RC_ASSERT(base.mkdir("gamma"));

    const std::vector<QString> validPaths = {
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
    };

    FileView view;

    // Optionally seed the history with a single entry.
    QString chosenPath;
    if (entries == 1) {
        chosenPath = *rc::gen::elementOf(validPaths);
        view.navigateTo(chosenPath);
    }

    // -----------------------------------------------------------------------
    // Capture state BEFORE the goBack() call.
    //   - entries == 0 ⇒ size 0, index -1, currentPath() empty.
    //   - entries == 1 ⇒ size 1, index 0,  currentPath() == chosenPath.
    // -----------------------------------------------------------------------
    const QString pathBefore  = view.currentPath();
    const int     indexBefore = FileViewTestAccess::historyIndex(view);
    const int     sizeBefore  = FileViewTestAccess::historySize(view);

    // Sanity-check the precondition we set up.
    if (entries == 0) {
        RC_ASSERT(sizeBefore  == 0);
        RC_ASSERT(indexBefore == -1);
        RC_ASSERT(pathBefore.isEmpty());
    } else {
        RC_ASSERT(sizeBefore  == 1);
        RC_ASSERT(indexBefore == 0);
        RC_ASSERT(pathBefore  == chosenPath);
    }

    // -----------------------------------------------------------------------
    // Trigger: goBack() at the lower bound.
    // -----------------------------------------------------------------------
    view.goBack();

    // -----------------------------------------------------------------------
    // Property assertion: nothing changed.
    // -----------------------------------------------------------------------
    RC_ASSERT(view.currentPath() == pathBefore);
    RC_ASSERT(FileViewTestAccess::historyIndex(view) == indexBefore);
    RC_ASSERT(FileViewTestAccess::historySize(view)  == sizeBefore);
}

// Sub-property (b): goForward at the upper bound (historyIndex_ already at
//                   history_.size() - 1) is a no-op, even when invoked
//                   repeatedly.
RC_GTEST_PROP(FileViewPropertyTests, GoForwardAtUpperBoundIsNoOp, ()) {
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Generate:
    //   - N: number of initial navigateTo calls in [1, 6].  After these
    //        calls, historyIndex_ == N - 1 == history_.size() - 1, i.e. we
    //        are at the upper bound.
    //   - K: number of goForward() calls to make at the upper bound, in
    //        [1, 5].  All must be no-ops.
    // -----------------------------------------------------------------------
    const int N = *rc::gen::inRange(1, 7);
    const int K = *rc::gen::inRange(1, 6);

    // Create N real subdirectories under a fresh QTemporaryDir.
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    std::vector<QString> paths;
    paths.reserve(N);
    for (int i = 0; i < N; ++i) {
        const QString name = QString("dir%1").arg(i);
        RC_ASSERT(base.mkdir(name));
        paths.push_back(tempDir.path() + "/" + name);
    }

    FileView view;

    // Populate history with N entries.  After this loop:
    //   history_.size() == N, historyIndex_ == N - 1.
    for (int i = 0; i < N; ++i) {
        view.navigateTo(paths[i]);
    }
    RC_ASSERT(FileViewTestAccess::historySize(view)  == N);
    RC_ASSERT(FileViewTestAccess::historyIndex(view) == N - 1);

    // Capture state BEFORE the goForward() calls.
    const QString pathBefore  = view.currentPath();
    const int     indexBefore = FileViewTestAccess::historyIndex(view);
    const int     sizeBefore  = FileViewTestAccess::historySize(view);

    // -----------------------------------------------------------------------
    // Trigger: K consecutive goForward() calls at the upper bound.
    // After each call we assert that nothing has changed.
    // -----------------------------------------------------------------------
    for (int i = 0; i < K; ++i) {
        view.goForward();

        RC_ASSERT(view.currentPath() == pathBefore);
        RC_ASSERT(FileViewTestAccess::historyIndex(view) == indexBefore);
        RC_ASSERT(FileViewTestAccess::historySize(view)  == sizeBefore);
    }
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 4: Capability signals accurately reflect history state
//
// For an arbitrary sequence of navigateTo / goBack / goForward / goUp calls
// on valid paths from a QTemporaryDir-built pool, the EFFECTIVE LAST VALUE
// reported by each of the three capability signals SHALL equal the expected
// predicate against FileView's live private state:
//
//   canGoBackChanged    effective last == (historyIndex_ > 0)
//   canGoForwardChanged effective last == (historyIndex_ < history_.size() - 1)
//   canGoUpChanged      effective last == (currentPath() != QDir::rootPath()
//                                          && !currentPath().isEmpty())
//
// "Effective last value" =
//     the last emitted boolean from the QSignalSpy if it received any
//     emissions, otherwise the construction-time default — which is false
//     for all three signals because FileView starts with empty history
//     (idx=-1, size=0, currentPath()=="").
//
// White-box read access to historyIndex_ / history_.size() is granted via
// FileViewTestAccess (declared as friend in include/gui/file_view.hpp).
//
// Validates: Requirements 3.9, 3.10, 3.11, 4.5, 4.6, 4.7
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, CapabilitySignalsReflectHistoryState, ()) {
    // RC_GTEST_PROP expands to TEST (no fixture); ensure a QApplication
    // exists before constructing any QWidget.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Build a real directory tree to give navigateTo() variety.  tempDir
    // must outlive 'view' (its QFileSystemModel holds watchers on these
    // paths), so it is declared first — C++ destroys locals in reverse
    // declaration order.
    // -----------------------------------------------------------------------
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    base.mkdir("alpha");
    base.mkdir("beta");
    base.mkdir("gamma");

    // Pool of valid paths the generator can pick from.  None of these are
    // expected to equal QDir::rootPath() on a typical Linux/macOS host, but
    // goUp() can still reach the filesystem root from any of them, which
    // exercises the canGoUp transition to false.
    const std::vector<QString> validPaths = {
        tempDir.path(),
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
        QDir::tempPath(),
        QDir::homePath(),
    };

    // -----------------------------------------------------------------------
    // Generate a random navigation sequence (length 1–20). Reuses the
    // file-scope NavOp enum declared above for Property 1.
    // -----------------------------------------------------------------------
    const std::size_t opCount = *rc::gen::inRange<std::size_t>(1, 21);
    const auto ops = *rc::gen::container<std::vector<NavOp>>(
        opCount,
        rc::gen::element(
            NavOp::NavigateTo,
            NavOp::GoBack,
            NavOp::GoForward,
            NavOp::GoUp
        )
    );

    // -----------------------------------------------------------------------
    // Fresh FileView per iteration.  Attach the three QSignalSpys BEFORE
    // running any navigation so every capability signal emission during
    // the sequence is recorded.
    // -----------------------------------------------------------------------
    FileView view;
    QSignalSpy backSpy   (&view, &FileView::canGoBackChanged);
    QSignalSpy forwardSpy(&view, &FileView::canGoForwardChanged);
    QSignalSpy upSpy     (&view, &FileView::canGoUpChanged);

    for (const NavOp op : ops) {
        switch (op) {
            case NavOp::NavigateTo: {
                const QString path = *rc::gen::elementOf(validPaths);
                view.navigateTo(path);
                break;
            }
            case NavOp::GoBack:    view.goBack();    break;
            case NavOp::GoForward: view.goForward(); break;
            case NavOp::GoUp:      view.goUp();      break;
        }
    }

    // -----------------------------------------------------------------------
    // Compute the "effective last value" for each spy.  When a spy has no
    // emissions the construction-time default applies (false for all three
    // capabilities, because empty history => idx=-1, size=0, path empty).
    // -----------------------------------------------------------------------
    auto effectiveLast = [](const QSignalSpy& spy) -> bool {
        if (spy.isEmpty()) return false;
        return spy.last().at(0).toBool();
    };

    const bool effCanGoBack    = effectiveLast(backSpy);
    const bool effCanGoForward = effectiveLast(forwardSpy);
    const bool effCanGoUp      = effectiveLast(upSpy);

    // -----------------------------------------------------------------------
    // Expected predicates evaluated against FileView's live private state.
    // -----------------------------------------------------------------------
    const int     idx  = FileViewTestAccess::historyIndex(view);
    const int     size = FileViewTestAccess::historySize(view);
    const QString cur  = view.currentPath();

    const bool expectedCanGoBack    = idx > 0;
    const bool expectedCanGoForward = idx < size - 1;
    const bool expectedCanGoUp      = !cur.isEmpty() && cur != QDir::rootPath();

    RC_ASSERT(effCanGoBack    == expectedCanGoBack);
    RC_ASSERT(effCanGoForward == expectedCanGoForward);
    RC_ASSERT(effCanGoUp      == expectedCanGoUp);
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 5: View mode persistence independent of current path
//
// For any view mode m in {ViewMode::Table, ViewMode::List} and any valid
// directory path p drawn from a QTemporaryDir-built pool, the sequence
//
//     navigateTo(p);
//     setViewMode(m);
//
// SHALL leave the FileView with both
//
//     currentViewMode() == m   AND   currentPath() == p
//
// holding simultaneously.  In other words, switching the view mode must
// not perturb the navigated path, and navigating must not perturb the
// view mode that the caller subsequently sets.
//
// Validates: Requirements 2.1, 2.2, 2.3, 2.4, 2.5
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, ViewModePersistsIndependentOfCurrentPath, ()) {
    // RC_GTEST_PROP expands to TEST (no fixture); make sure a QApplication
    // exists before constructing any QWidget.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Build a real directory pool under a fresh QTemporaryDir.  tempDir
    // must outlive the FileView (its QFileSystemModel watches paths), so
    // it is declared before the view (C++ destroys locals in LIFO order).
    // -----------------------------------------------------------------------
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    RC_ASSERT(base.mkdir("alpha"));
    RC_ASSERT(base.mkdir("beta"));
    RC_ASSERT(base.mkdir("gamma"));
    RC_ASSERT(base.mkdir("delta"));

    const std::vector<QString> validPaths = {
        tempDir.path(),
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
        tempDir.path() + "/delta",
    };

    // -----------------------------------------------------------------------
    // Generate the inputs:
    //   - p: an arbitrary valid path from the pool.
    //   - m: an arbitrary ViewMode from {Table, List}.
    // -----------------------------------------------------------------------
    const QString p = *rc::gen::elementOf(validPaths);
    const ViewMode m = *rc::gen::element(ViewMode::Table, ViewMode::List);

    // -----------------------------------------------------------------------
    // Apply the sequence to a fresh FileView.
    // -----------------------------------------------------------------------
    FileView view;

    view.navigateTo(p);
    view.setViewMode(m);

    // -----------------------------------------------------------------------
    // Property assertion: both invariants hold simultaneously.
    // -----------------------------------------------------------------------
    RC_ASSERT(view.currentViewMode() == m);
    RC_ASSERT(view.currentPath()     == p);
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 9: Sort column round-trip
//
// For any column index c ∈ [0, 3] and any Qt::SortOrder o ∈
// {Qt::AscendingOrder, Qt::DescendingOrder}, calling
// view.setSortColumn(c, o) on a freshly constructed FileView SHALL result
// in the underlying QTableView's horizontal header reporting:
//
//   header->sortIndicatorSection() == c
//   header->sortIndicatorOrder()   == o
//
// We reach the QTableView through the public widget hierarchy via
// view.findChild<QTableView*>() — no white-box access is required, since
// the table is a Qt child of FileView.
//
// Validates: Requirements 8.3
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, SortColumnRoundTrip, ()) {
    // RC_GTEST_PROP expands to TEST (no fixture); ensure a QApplication
    // exists before constructing any QWidget.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Generate inputs:
    //   - c: column index in [0, 3]  (rc::gen::inRange upper bound is exclusive)
    //   - o: Qt::SortOrder picked from the two valid values
    // -----------------------------------------------------------------------
    const int c = *rc::gen::inRange(0, 4);
    const Qt::SortOrder o =
        *rc::gen::element(Qt::AscendingOrder, Qt::DescendingOrder);

    // Step 1: Construct fresh FileView.
    FileView view;

    // Step 2: Apply the sort.
    view.setSortColumn(c, o);

    // Step 3: Reach the QTableView through the public Qt child hierarchy.
    auto* tableView = view.findChild<QTableView*>();
    RC_ASSERT(tableView != nullptr);

    auto* header = tableView->horizontalHeader();
    RC_ASSERT(header != nullptr);

    // Step 4: Round-trip assertion — section + order match what we set.
    RC_ASSERT(header->sortIndicatorSection() == c);
    RC_ASSERT(header->sortIndicatorOrder()   == o);
}

// ---------------------------------------------------------------------------
// Feature: fileview-refactor, Property 8: Refresh idempotence
//
// For any valid directory path p drawn from a QTemporaryDir-built pool and
// any natural number n in [1, 5], the sequence
//
//     FileView view;             // freshly constructed
//     view.navigateTo(p);        // currentPath() must equal p
//     for (i = 0; i < n; ++i) {
//         view.refresh();
//         assert(view.currentPath() == p);
//     }
//
// SHALL hold.  In other words refresh() is idempotent with respect to
// currentPath(): a single refresh produces the same observable path as
// any number of subsequent refreshes — none of them perturb the navigated
// state.
//
// Validates: Requirements 6.1
// ---------------------------------------------------------------------------
RC_GTEST_PROP(FileViewPropertyTests, RefreshIsIdempotentForCurrentPath, ()) {
    // RC_GTEST_PROP expands to a free TEST (no fixture); ensure a
    // QApplication exists before constructing any QWidget.
    if (!QCoreApplication::instance()) {
        static int   argc = 0;
        static char* argv[] = {nullptr};
        static QApplication localApp(argc, argv);
        Q_UNUSED(localApp);
    }

    // -----------------------------------------------------------------------
    // Build a real directory pool under a fresh QTemporaryDir.  tempDir
    // must outlive the FileView (its QFileSystemModel watches paths), so
    // it is declared before the view (C++ destroys locals in LIFO order).
    // -----------------------------------------------------------------------
    QTemporaryDir tempDir;
    RC_PRE(tempDir.isValid());

    QDir base(tempDir.path());
    RC_ASSERT(base.mkdir("alpha"));
    RC_ASSERT(base.mkdir("beta"));
    RC_ASSERT(base.mkdir("gamma"));
    RC_ASSERT(base.mkdir("delta"));

    const std::vector<QString> validPaths = {
        tempDir.path(),
        tempDir.path() + "/alpha",
        tempDir.path() + "/beta",
        tempDir.path() + "/gamma",
        tempDir.path() + "/delta",
    };

    // -----------------------------------------------------------------------
    // Generate the inputs:
    //   - p: an arbitrary valid path from the pool.
    //   - n: number of refresh() calls to issue, in [1, 5]
    //        (rc::gen::inRange upper bound is exclusive ⇒ inRange(1, 6)).
    // -----------------------------------------------------------------------
    const QString p = *rc::gen::elementOf(validPaths);
    const int     n = *rc::gen::inRange(1, 6);

    // -----------------------------------------------------------------------
    // Step 1 & 2: fresh FileView, navigate to p.
    // -----------------------------------------------------------------------
    FileView view;
    view.navigateTo(p);

    // -----------------------------------------------------------------------
    // Step 3: capture currentPath() — must equal p before we start
    // exercising refresh().
    // -----------------------------------------------------------------------
    const QString captured = view.currentPath();
    RC_ASSERT(captured == p);

    // -----------------------------------------------------------------------
    // Step 4: refresh() n times; currentPath() must remain == p after each
    // individual call (not just after the last one).
    // -----------------------------------------------------------------------
    for (int i = 0; i < n; ++i) {
        view.refresh();
        RC_ASSERT(view.currentPath() == p);
    }
}

// ===========================================================================
// Example-based unit tests — edge cases not covered by property tests
// (Task 7.4)
//
// Validates: Requirements 5.1, 5.2, 3.7, 3.8
// ===========================================================================

// ---------------------------------------------------------------------------
// Test: double-click on a directory triggers navigateTo (Req 5.1)
//
// Setup:
//   1. Create a QTemporaryDir with one subdirectory ("subdir").
//   2. navigateTo(tempDir.path()) so the FileView is rooted at the temp dir.
//   3. Resolve the subdirectory's QModelIndex via QFileSystemModel::index(path).
//   4. Invoke the private slot onItemActivated via QMetaObject::invokeMethod
//      (slots are invokable through the meta-object system regardless of
//      access; the slot is registered because it appears under the
//      `private slots:` section of file_view.hpp).
//
// Assertion:
//   currentPath() updates to the subdirectory's path.
//
// Note: we deliberately avoid simulating a real Qt mouse double-click via
//       QTest::mouseDClick because that requires the QTableView to be shown,
//       its viewport to be laid out, and the QFileSystemModel to have
//       finished its asynchronous population — none of which is reliable
//       in a headless unit-test environment.  Invoking the slot directly
//       exercises exactly the behaviour we care about (Req 5.1: directory
//       double-click → navigateTo).
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, DoubleClickOnDirectoryNavigatesIntoIt) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    QDir base(tempDir.path());
    ASSERT_TRUE(base.mkdir("subdir"));
    const QString subdirPath = tempDir.path() + "/subdir";

    FileView view;
    view.navigateTo(tempDir.path());
    ASSERT_EQ(view.currentPath(), tempDir.path());

    auto* model = view.findChild<QFileSystemModel*>();
    ASSERT_NE(model, nullptr);

    // QFileSystemModel::index(path) returns a valid index synchronously
    // for any path that exists on disk.
    const QModelIndex subdirIdx = model->index(subdirPath);
    ASSERT_TRUE(subdirIdx.isValid()) << "Could not resolve QModelIndex for " << subdirPath.toStdString();
    ASSERT_TRUE(model->isDir(subdirIdx));

    // Invoke the private slot directly.  QMetaObject::invokeMethod bypasses
    // C++ access control because slots are part of the meta-object system.
    const bool invoked = QMetaObject::invokeMethod(
        &view,
        "onItemActivated",
        Qt::DirectConnection,
        Q_ARG(QModelIndex, subdirIdx));
    ASSERT_TRUE(invoked) << "QMetaObject::invokeMethod failed to invoke onItemActivated";

    EXPECT_EQ(view.currentPath(), subdirPath);
}

// ---------------------------------------------------------------------------
// Test: goBack() on empty history is a no-op (Req 3.7 edge case)
//
// On a freshly constructed FileView, history_ is empty and historyIndex_
// is -1.  Calling goBack() must NOT change currentPath() (it stays empty)
// and must NOT emit pathChanged or any capability-changed signals (the
// values cannot flip because no transition occurred).
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, GoBackOnEmptyHistoryIsNoOp) {
    FileView view;

    QSignalSpy pathSpy(&view, &FileView::pathChanged);
    QSignalSpy backSpy(&view, &FileView::canGoBackChanged);
    QSignalSpy fwdSpy (&view, &FileView::canGoForwardChanged);
    QSignalSpy upSpy  (&view, &FileView::canGoUpChanged);

    ASSERT_TRUE(view.currentPath().isEmpty());

    view.goBack();

    EXPECT_TRUE(view.currentPath().isEmpty());
    EXPECT_EQ(pathSpy.count(), 0);
    EXPECT_EQ(backSpy.count(), 0);
    EXPECT_EQ(fwdSpy.count(),  0);
    EXPECT_EQ(upSpy.count(),   0);
}

// ---------------------------------------------------------------------------
// Test: navigateTo with a non-existent path is silently ignored (Req 3)
//
// After navigating to a real path, attempting to navigate to a
// non-existent path must leave currentPath() unchanged and must not
// emit pathChanged a second time.
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, NavigateToNonExistentPathIsSilentlyIgnored) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    FileView view;
    view.navigateTo(tempDir.path());
    ASSERT_EQ(view.currentPath(), tempDir.path());

    // Spy attached AFTER the initial navigation so the count reflects only
    // emissions caused by the bogus navigateTo call below.
    QSignalSpy pathSpy(&view, &FileView::pathChanged);

    const QString bogusPath = "/nonexistent/path/12345_does_not_exist_xyz";
    ASSERT_FALSE(QDir(bogusPath).exists());

    view.navigateTo(bogusPath);

    EXPECT_EQ(view.currentPath(), tempDir.path()) << "currentPath() must be unchanged";
    EXPECT_EQ(pathSpy.count(), 0)                << "pathChanged must not fire for invalid path";
}

// ---------------------------------------------------------------------------
// Test: goUp() at the filesystem root is a no-op (Req 3.8 edge case)
//
// On Linux QDir::rootPath() returns "/", whose cdUp() returns false.
// FileView::goUp() therefore returns early without modifying state.
// ---------------------------------------------------------------------------
TEST_F(FileViewTest, GoUpAtFilesystemRootIsNoOp) {
    FileView view;

    const QString rootPath = QDir::rootPath();
    view.navigateTo(rootPath);
    ASSERT_EQ(view.currentPath(), rootPath);

    // Spy attached AFTER the initial navigation so we can assert that
    // goUp() does not emit pathChanged.
    QSignalSpy pathSpy(&view, &FileView::pathChanged);

    view.goUp();

    EXPECT_EQ(view.currentPath(), rootPath) << "currentPath() must be unchanged at root";
    EXPECT_EQ(pathSpy.count(), 0)           << "pathChanged must not fire at filesystem root";
}
