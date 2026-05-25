// tests/unit/test_cut_state_proxy_model.cpp
// Unit + property-based tests for CutStateProxyModel.
// Requirements: 5.5, 5.6
//
// This file holds unit tests for the proxy model's cut-state tracking
// (task 4.3) and will later receive Property 12 (task 4.4).
//
// Testing approach (per task notes):
//   Since CutStateProxyModel::data() uses qobject_cast<QFileSystemModel*>
//   to obtain file paths, and we use QStandardItemModel here, the cast
//   returns nullptr and data() falls through to the base class. Therefore:
//   - We test isCut() directly to verify cut-path membership logic.
//   - We test setCutPaths/clearCutPaths emit dataChanged via QSignalSpy.
//   - The full proxy-with-QFileSystemModel path is covered by the
//     integration test (task 12).

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <QApplication>
#include <QSignalSpy>
#include <QStandardItemModel>

#include "gui/cut_state_proxy_model.hpp"

// ---------------------------------------------------------------------------
// QApplication fixture
// Qt widgets and proxy models require a QApplication to be alive.
// We share a single instance across all tests in this translation unit.
// ---------------------------------------------------------------------------
class CutStateProxyModelTest : public ::testing::Test {
protected:
    static QApplication* app_;

    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int   argc = 0;
            static char* argv[] = {nullptr};
            app_ = new QApplication(argc, argv);
        }
    }

    void SetUp() override {
        // Populate a QStandardItemModel with synthetic absolute paths.
        // The display text of each item is the path basename; we store
        // the full path in Qt::UserRole for easy retrieval in tests.
        sourceModel_ = new QStandardItemModel;

        const QStringList paths = {
            QStringLiteral("/home/user/Documents/report.pdf"),
            QStringLiteral("/home/user/Documents/notes.txt"),
            QStringLiteral("/home/user/Pictures/photo.png"),
            QStringLiteral("/tmp/scratch/temp_file.dat"),
            QStringLiteral("/var/log/syslog"),
        };

        for (const QString& path : paths) {
            auto* item = new QStandardItem(path.section('/', -1)); // basename
            item->setData(path, Qt::UserRole); // store full path
            sourceModel_->appendRow(item);
        }

        proxy_ = new CutStateProxyModel;
        proxy_->setSourceModel(sourceModel_);
    }

    void TearDown() override {
        delete proxy_;
        proxy_ = nullptr;
        delete sourceModel_;
        sourceModel_ = nullptr;
    }

    QStandardItemModel*  sourceModel_ = nullptr;
    CutStateProxyModel*  proxy_       = nullptr;
};

QApplication* CutStateProxyModelTest::app_ = nullptr;

// ---------------------------------------------------------------------------
// Test: isCut returns false for all paths when no cut paths are set
//
// Validates: Requirements 5.5, 5.6
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, IsCutReturnsFalseWhenNoCutPathsSet) {
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/notes.txt")));
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/tmp/scratch/temp_file.dat")));
}

// ---------------------------------------------------------------------------
// Test: isCut returns true only for paths in the cut set
//
// After setCutPaths({A, B}), isCut(A) and isCut(B) should be true,
// while isCut(C) (not in the set) should be false.
//
// Validates: Requirements 5.5
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, IsCutReturnsTrueOnlyForCutPaths) {
    const QSet<QString> cutSet = {
        QStringLiteral("/home/user/Documents/report.pdf"),
        QStringLiteral("/tmp/scratch/temp_file.dat"),
    };

    proxy_->setCutPaths(cutSet);

    EXPECT_TRUE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
    EXPECT_TRUE(proxy_->isCut(QStringLiteral("/tmp/scratch/temp_file.dat")));
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/notes.txt")));
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Pictures/photo.png")));
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/var/log/syslog")));
}

// ---------------------------------------------------------------------------
// Test: clearCutPaths removes all cut state
//
// After setCutPaths followed by clearCutPaths, isCut should return false
// for all previously-cut paths.
//
// Validates: Requirements 5.6
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, ClearCutPathsRemovesAllCutState) {
    const QSet<QString> cutSet = {
        QStringLiteral("/home/user/Documents/report.pdf"),
        QStringLiteral("/home/user/Documents/notes.txt"),
    };

    proxy_->setCutPaths(cutSet);
    ASSERT_TRUE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));

    proxy_->clearCutPaths();

    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/notes.txt")));
}

// ---------------------------------------------------------------------------
// Test: setCutPaths replaces previous cut set entirely
//
// Calling setCutPaths with a new set should discard the old set.
// Paths in the old set but not the new set should no longer be cut.
//
// Validates: Requirements 5.5, 5.6
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, SetCutPathsReplacesPreviousSet) {
    const QSet<QString> firstSet = {
        QStringLiteral("/home/user/Documents/report.pdf"),
    };
    const QSet<QString> secondSet = {
        QStringLiteral("/home/user/Pictures/photo.png"),
    };

    proxy_->setCutPaths(firstSet);
    ASSERT_TRUE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
    ASSERT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Pictures/photo.png")));

    proxy_->setCutPaths(secondSet);
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
    EXPECT_TRUE(proxy_->isCut(QStringLiteral("/home/user/Pictures/photo.png")));
}

// ---------------------------------------------------------------------------
// Test: setCutPaths emits dataChanged signal
//
// When the proxy has rows, setCutPaths must emit dataChanged so that
// attached views repaint the affected rows.
//
// Validates: Requirements 5.5
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, SetCutPathsEmitsDataChanged) {
    QSignalSpy spy(proxy_, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(spy.isValid());

    const QSet<QString> cutSet = {
        QStringLiteral("/home/user/Documents/report.pdf"),
    };

    proxy_->setCutPaths(cutSet);

    EXPECT_GE(spy.count(), 1)
        << "setCutPaths should emit dataChanged at least once";

    // Verify the signal covers the full model range (top-left to bottom-right)
    const auto args = spy.takeFirst();
    const QModelIndex topLeft = args.at(0).value<QModelIndex>();
    const QModelIndex bottomRight = args.at(1).value<QModelIndex>();

    EXPECT_EQ(topLeft.row(), 0);
    EXPECT_EQ(topLeft.column(), 0);
    EXPECT_EQ(bottomRight.row(), proxy_->rowCount() - 1);
    EXPECT_EQ(bottomRight.column(), proxy_->columnCount() - 1);
}

// ---------------------------------------------------------------------------
// Test: clearCutPaths emits dataChanged signal
//
// When the proxy has rows, clearCutPaths must emit dataChanged so that
// attached views repaint (removing the dimmed indicator).
//
// Validates: Requirements 5.6
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, ClearCutPathsEmitsDataChanged) {
    // First set some cut paths so there's state to clear
    proxy_->setCutPaths({QStringLiteral("/home/user/Documents/report.pdf")});

    QSignalSpy spy(proxy_, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(spy.isValid());

    proxy_->clearCutPaths();

    EXPECT_GE(spy.count(), 1)
        << "clearCutPaths should emit dataChanged at least once";

    // Verify the signal covers the full model range
    const auto args = spy.takeFirst();
    const QModelIndex topLeft = args.at(0).value<QModelIndex>();
    const QModelIndex bottomRight = args.at(1).value<QModelIndex>();

    EXPECT_EQ(topLeft.row(), 0);
    EXPECT_EQ(topLeft.column(), 0);
    EXPECT_EQ(bottomRight.row(), proxy_->rowCount() - 1);
    EXPECT_EQ(bottomRight.column(), proxy_->columnCount() - 1);
}

// ---------------------------------------------------------------------------
// Test: setCutPaths with empty set clears all cut state
//
// Setting an empty set should behave like clearCutPaths — no path is cut.
//
// Validates: Requirements 5.6
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, SetCutPathsWithEmptySetClearsState) {
    proxy_->setCutPaths({QStringLiteral("/home/user/Documents/report.pdf")});
    ASSERT_TRUE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));

    proxy_->setCutPaths({});

    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
}

// ---------------------------------------------------------------------------
// Test: dataChanged signal includes Qt::ForegroundRole in roles vector
//
// The dataChanged emission should specify Qt::ForegroundRole so views
// know which role changed (optimization hint for Qt's view framework).
//
// Validates: Requirements 5.5
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, DataChangedIncludesForegroundRole) {
    QSignalSpy spy(proxy_, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(spy.isValid());

    proxy_->setCutPaths({QStringLiteral("/tmp/scratch/temp_file.dat")});

    ASSERT_GE(spy.count(), 1);

    // Third argument of dataChanged is QVector<int> roles
    const auto args = spy.takeFirst();
    ASSERT_GE(args.size(), 3);
    const QList<int> roles = args.at(2).value<QList<int>>();

    EXPECT_TRUE(roles.contains(Qt::ForegroundRole))
        << "dataChanged should specify Qt::ForegroundRole in the roles list";
}

// ---------------------------------------------------------------------------
// Test: isCut is case-sensitive (paths are absolute filesystem paths)
//
// On Linux, filesystem paths are case-sensitive. Verify that isCut
// respects this.
//
// Validates: Requirements 5.5
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, IsCutIsCaseSensitive) {
    proxy_->setCutPaths({QStringLiteral("/home/user/Documents/Report.pdf")});

    // Exact match (different case from our model paths)
    EXPECT_TRUE(proxy_->isCut(QStringLiteral("/home/user/Documents/Report.pdf")));
    // Different case — should NOT match
    EXPECT_FALSE(proxy_->isCut(QStringLiteral("/home/user/Documents/report.pdf")));
}

// ---------------------------------------------------------------------------
// Test: no dataChanged emitted when model is empty
//
// If the proxy has no rows (empty source model), setCutPaths/clearCutPaths
// should not emit dataChanged (the implementation guards on rowCount > 0).
//
// Validates: Requirements 5.5
// ---------------------------------------------------------------------------
TEST_F(CutStateProxyModelTest, NoDataChangedWhenModelIsEmpty) {
    // Create a proxy with an empty source model
    QStandardItemModel emptyModel;
    CutStateProxyModel emptyProxy;
    emptyProxy.setSourceModel(&emptyModel);

    QSignalSpy spy(&emptyProxy, &QAbstractItemModel::dataChanged);
    ASSERT_TRUE(spy.isValid());

    emptyProxy.setCutPaths({QStringLiteral("/some/path")});
    EXPECT_EQ(spy.count(), 0)
        << "No dataChanged should be emitted when the model has no rows";

    emptyProxy.clearCutPaths();
    EXPECT_EQ(spy.count(), 0)
        << "No dataChanged should be emitted when the model has no rows";
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 12: Cut visual indicator round-trip
//
// For arbitrary path sets P and P' (both possibly empty), after
// setCutPaths(P); setCutPaths(P'), isCut(p) == (p ∈ P') for every p
// in the model universe.
//
// Validates: Requirements 5.5, 5.6
// ---------------------------------------------------------------------------

// Fixed universe of test paths used by the property test generator.
static const QStringList kUniversePaths = {
    QStringLiteral("/home/user/Documents/report.pdf"),
    QStringLiteral("/home/user/Documents/notes.txt"),
    QStringLiteral("/home/user/Pictures/photo.png"),
    QStringLiteral("/tmp/scratch/temp_file.dat"),
    QStringLiteral("/var/log/syslog"),
};

// We need a test fixture that provides QApplication for the property test.
// RC_GTEST_FIXTURE_PROP requires a ::testing::Test-derived fixture.
class CutStateProxyModelPropTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QCoreApplication::instance()) {
            static int   argc = 0;
            static char* argv[] = {nullptr};
            static QApplication app(argc, argv);
        }
    }
};

RC_GTEST_FIXTURE_PROP(CutStateProxyModelPropTest,
                      CutVisualIndicatorRoundTrip,
                      ()) {
    // Generate two arbitrary subsets of the universe (P and P').
    // Each path is independently included or excluded via a boolean mask.
    auto maskP  = *rc::gen::container<std::vector<bool>>(
        kUniversePaths.size(), rc::gen::arbitrary<bool>());
    auto maskPP = *rc::gen::container<std::vector<bool>>(
        kUniversePaths.size(), rc::gen::arbitrary<bool>());

    // Build QSet<QString> from the masks.
    QSet<QString> P;
    for (int i = 0; i < kUniversePaths.size(); ++i) {
        if (maskP[static_cast<size_t>(i)])
            P.insert(kUniversePaths[i]);
    }

    QSet<QString> PP; // P'
    for (int i = 0; i < kUniversePaths.size(); ++i) {
        if (maskPP[static_cast<size_t>(i)])
            PP.insert(kUniversePaths[i]);
    }

    // Set up a source model and proxy (fresh per iteration).
    QStandardItemModel sourceModel;
    for (const QString& path : kUniversePaths) {
        auto* item = new QStandardItem(path.section('/', -1));
        item->setData(path, Qt::UserRole);
        sourceModel.appendRow(item);
    }

    CutStateProxyModel proxy;
    proxy.setSourceModel(&sourceModel);

    // Apply P then P' (the second call should fully replace the first).
    proxy.setCutPaths(P);
    proxy.setCutPaths(PP);

    // Assert: for every path in the universe, isCut(p) == (p ∈ P').
    for (const QString& p : kUniversePaths) {
        RC_ASSERT(proxy.isCut(p) == PP.contains(p));
    }
}
