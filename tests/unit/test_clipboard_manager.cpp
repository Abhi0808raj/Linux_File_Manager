// tests/unit/test_clipboard_manager.cpp
// Unit + property-based tests for ClipboardManager.
// Requirements: 12.1, 12.4
//
// This file holds construction-invariant unit tests (task 1.3) and will
// later receive property tests for Properties 1, 2, 3, and 11 (tasks 1.4–1.7).

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <QList>
#include <QString>

#include "gui/clipboard_manager.hpp"

// ---------------------------------------------------------------------------
// Test: Default-constructed ClipboardManager reports isEmpty() == true
//
// A freshly constructed ClipboardManager has never been set, so it must
// report itself as empty. This is the construction-time invariant that
// ensures Paste is disabled until the user explicitly copies or cuts.
//
// Validates: Requirements 12.1, 12.4
// ---------------------------------------------------------------------------
TEST(ClipboardManagerTest, DefaultConstructedIsEmpty) {
    ClipboardManager cb;
    EXPECT_TRUE(cb.isEmpty());
}

// ---------------------------------------------------------------------------
// Test: Default-constructed ClipboardManager has empty paths list
//
// Validates: Requirements 12.1, 12.4
// ---------------------------------------------------------------------------
TEST(ClipboardManagerTest, DefaultConstructedHasEmptyPaths) {
    ClipboardManager cb;
    EXPECT_TRUE(cb.paths().isEmpty());
}

// ---------------------------------------------------------------------------
// Test: clear() after set returns to empty
//
// After storing paths via set(), calling clear() must bring the clipboard
// back to the empty state — isEmpty() == true and paths() is empty.
//
// Validates: Requirements 12.1, 12.4
// ---------------------------------------------------------------------------
TEST(ClipboardManagerTest, ClearAfterSetReturnsToEmpty) {
    ClipboardManager cb;

    QList<QString> paths;
    paths << QStringLiteral("/home/user/file.txt")
          << QStringLiteral("/home/user/photo.png");

    cb.set(paths, ClipboardOperation::Copy);
    ASSERT_FALSE(cb.isEmpty()) << "Precondition: clipboard should be non-empty after set";

    cb.clear();
    EXPECT_TRUE(cb.isEmpty());
    EXPECT_TRUE(cb.paths().isEmpty());
}

// ---------------------------------------------------------------------------
// Test: clear() after set with Cut operation also returns to empty
//
// Validates: Requirements 12.1, 12.4
// ---------------------------------------------------------------------------
TEST(ClipboardManagerTest, ClearAfterCutReturnsToEmpty) {
    ClipboardManager cb;

    QList<QString> paths;
    paths << QStringLiteral("/tmp/document.pdf");

    cb.set(paths, ClipboardOperation::Cut);
    ASSERT_FALSE(cb.isEmpty());

    cb.clear();
    EXPECT_TRUE(cb.isEmpty());
    EXPECT_TRUE(cb.paths().isEmpty());
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 1: Clipboard Copy round-trip
//
// For any non-empty QList<QString> P, after set(P, Copy) the clipboard
// reports paths() == P, operation() == Copy, and isEmpty() == false.
//
// **Validates: Requirements 4.2, 12.8**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(ClipboardManagerProp, CopyRoundTrip, (const std::vector<std::string>& raw)) {
    RC_PRE(!raw.empty());
    QList<QString> P;
    for (const auto& s : raw) P << QString::fromStdString(s);

    ClipboardManager cb;
    cb.set(P, ClipboardOperation::Copy);

    RC_ASSERT(cb.paths() == P);
    RC_ASSERT(cb.operation() == ClipboardOperation::Copy);
    RC_ASSERT(!cb.isEmpty());
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 2: Clipboard Cut round-trip
//
// For any non-empty QList<QString> P, after set(P, Cut) the clipboard
// reports paths() == P, operation() == Cut, and isEmpty() == false.
//
// **Validates: Requirements 5.2, 12.9**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(ClipboardManagerProp, CutRoundTrip, (const std::vector<std::string>& raw)) {
    RC_PRE(!raw.empty());
    QList<QString> P;
    for (const auto& s : raw) P << QString::fromStdString(s);

    ClipboardManager cb;
    cb.set(P, ClipboardOperation::Cut);

    RC_ASSERT(cb.paths() == P);
    RC_ASSERT(cb.operation() == ClipboardOperation::Cut);
    RC_ASSERT(!cb.isEmpty());
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 3: Clipboard replace invariant
//
// For any two non-empty path lists P1, P2 and operations op1, op2, after
// set(P1, op1); set(P2, op2) the clipboard reports paths() == P2 and
// operation() == op2.
//
// **Validates: Requirements 4.3, 5.3, 12.2, 12.3**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(ClipboardManagerProp, ReplaceInvariant, ()) {
    const auto raw1 = *rc::gen::nonEmpty<std::vector<std::string>>();
    const auto raw2 = *rc::gen::nonEmpty<std::vector<std::string>>();
    const auto op1 = *rc::gen::element(ClipboardOperation::Copy, ClipboardOperation::Cut);
    const auto op2 = *rc::gen::element(ClipboardOperation::Copy, ClipboardOperation::Cut);

    QList<QString> P1;
    for (const auto& s : raw1) P1 << QString::fromStdString(s);
    QList<QString> P2;
    for (const auto& s : raw2) P2 << QString::fromStdString(s);

    ClipboardManager cb;
    cb.set(P1, op1);
    cb.set(P2, op2);

    RC_ASSERT(cb.paths() == P2);
    RC_ASSERT(cb.operation() == op2);
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 11: Empty-list set is a no-op
//
// For any prior state (P, op) and any op2, after set(emptyList, op2) the
// clipboard's paths(), operation(), and isEmpty() are unchanged.
//
// **Validates: Requirements 12.11**
// ---------------------------------------------------------------------------
RC_GTEST_PROP(ClipboardManagerProp, EmptyListSetIsNoOp,
              (const std::vector<std::string>& raw, bool useCutOp, bool useCutOp2)) {
    RC_PRE(!raw.empty());

    QList<QString> P;
    for (const auto& s : raw) P << QString::fromStdString(s);

    ClipboardOperation op = useCutOp ? ClipboardOperation::Cut : ClipboardOperation::Copy;
    ClipboardOperation op2 = useCutOp2 ? ClipboardOperation::Cut : ClipboardOperation::Copy;

    ClipboardManager cb;
    cb.set(P, op);

    // Capture state before the empty-list set call.
    QList<QString> pathsBefore = cb.paths();
    ClipboardOperation opBefore = cb.operation();
    bool emptyBefore = cb.isEmpty();

    // Attempt to set with an empty list — should be a no-op.
    cb.set(QList<QString>{}, op2);

    RC_ASSERT(cb.paths() == pathsBefore);
    RC_ASSERT(cb.operation() == opBefore);
    RC_ASSERT(cb.isEmpty() == emptyBefore);
}
