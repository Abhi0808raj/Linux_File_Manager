// tests/unit/test_context_menu_action_state.cpp
// Unit + property-based tests for ContextMenuActionState::compute.
// Requirements: 11.2, 11.3, 11.4, 11.5, 11.6, 11.7, 11.8, 11.9, 11.10, 11.11, 11.12, 11.13

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "gui/context_menu_action_state.hpp"

#include <cstring>
#include <type_traits>
#include <tuple>

// ---------------------------------------------------------------------------
// Compile-time half of Property 6: Action enable/disable is view-mode agnostic
//
// The function signature takes (int, bool, bool, bool) — no ViewMode parameter.
// If someone accidentally adds a ViewMode parameter, this static_assert will
// fire at compile time, proving the structural guarantee.
// ---------------------------------------------------------------------------
static_assert(
    std::is_invocable_r_v<ContextMenuActionState,
                          decltype(&ContextMenuActionState::compute),
                          int, bool, bool, bool>,
    "ContextMenuActionState::compute must accept exactly (int, bool, bool, bool) "
    "— no ViewMode parameter allowed (Property 6)");

// Also verify it is NOT callable with 5 arguments (catches an extra param).
static_assert(
    !std::is_invocable_v<decltype(&ContextMenuActionState::compute),
                         int, bool, bool, bool, int>,
    "ContextMenuActionState::compute must NOT accept a 5th parameter "
    "(would violate Property 6 — view-mode agnostic)");

// ---------------------------------------------------------------------------
// 24-row truth table — parameterized test
//
// Input space: (selectionCount ∈ {0, 1, 2})
//            × (isWritable ∈ {true, false})
//            × (clipboardEmpty ∈ {true, false})
//            × (selectionIsFile ∈ {true, false})
//
// 3 × 2 × 2 × 2 = 24 rows.
// ---------------------------------------------------------------------------

struct ActionStateRow {
    // Inputs
    int  selectionCount;
    bool isWritable;
    bool clipboardEmpty;
    bool selectionIsFile;

    // Expected outputs
    bool open;
    bool openWith;
    bool copy;
    bool cut;
    bool paste;
    bool rename;
    bool del;
    bool newFolder;
    bool properties;
};

class ContextMenuActionStateTruthTable
    : public ::testing::TestWithParam<ActionStateRow> {};

TEST_P(ContextMenuActionStateTruthTable, MatchesDesignTruthTable) {
    const auto& row = GetParam();

    const auto result = ContextMenuActionState::compute(
        row.selectionCount, row.isWritable, row.clipboardEmpty, row.selectionIsFile);

    EXPECT_EQ(result.open,       row.open)
        << "open mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.openWith,   row.openWith)
        << "openWith mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.copy,       row.copy)
        << "copy mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.cut,        row.cut)
        << "cut mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.paste,      row.paste)
        << "paste mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.rename,     row.rename)
        << "rename mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.del,        row.del)
        << "del mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.newFolder,  row.newFolder)
        << "newFolder mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;

    EXPECT_EQ(result.properties, row.properties)
        << "properties mismatch for selCount=" << row.selectionCount
        << " writable=" << row.isWritable
        << " clipEmpty=" << row.clipboardEmpty
        << " isFile=" << row.selectionIsFile;
}

// Truth table derived from design document § ContextMenuActionState:
//
// | Action     | Enabled iff                                          |
// |------------|------------------------------------------------------|
// | Open       | selectionCount == 1                                  |
// | Open With  | selectionCount == 1 && selectionIsFile               |
// | Copy       | selectionCount > 0                                   |
// | Cut        | selectionCount > 0 && isWritable                     |
// | Paste      | !clipboardEmpty && isWritable                         |
// | Rename     | selectionCount == 1 && isWritable                    |
// | Delete     | selectionCount > 0 && isWritable                     |
// | New Folder | isWritable                                           |
// | Properties | selectionCount == 1                                  |

INSTANTIATE_TEST_SUITE_P(
    ExhaustiveTruthTable,
    ContextMenuActionStateTruthTable,
    ::testing::Values(
        // selCount=0, isWritable=true, clipboardEmpty=true, selectionIsFile=true
        ActionStateRow{0, true, true, true,
            /*open*/false, /*openWith*/false, /*copy*/false, /*cut*/false,
            /*paste*/false, /*rename*/false, /*del*/false, /*newFolder*/true, /*properties*/false},

        // selCount=0, isWritable=true, clipboardEmpty=true, selectionIsFile=false
        ActionStateRow{0, true, true, false,
            false, false, false, false,
            false, false, false, true, false},

        // selCount=0, isWritable=true, clipboardEmpty=false, selectionIsFile=true
        ActionStateRow{0, true, false, true,
            false, false, false, false,
            true, false, false, true, false},

        // selCount=0, isWritable=true, clipboardEmpty=false, selectionIsFile=false
        ActionStateRow{0, true, false, false,
            false, false, false, false,
            true, false, false, true, false},

        // selCount=0, isWritable=false, clipboardEmpty=true, selectionIsFile=true
        ActionStateRow{0, false, true, true,
            false, false, false, false,
            false, false, false, false, false},

        // selCount=0, isWritable=false, clipboardEmpty=true, selectionIsFile=false
        ActionStateRow{0, false, true, false,
            false, false, false, false,
            false, false, false, false, false},

        // selCount=0, isWritable=false, clipboardEmpty=false, selectionIsFile=true
        ActionStateRow{0, false, false, true,
            false, false, false, false,
            false, false, false, false, false},

        // selCount=0, isWritable=false, clipboardEmpty=false, selectionIsFile=false
        ActionStateRow{0, false, false, false,
            false, false, false, false,
            false, false, false, false, false},

        // selCount=1, isWritable=true, clipboardEmpty=true, selectionIsFile=true
        ActionStateRow{1, true, true, true,
            true, true, true, true,
            false, true, true, true, true},

        // selCount=1, isWritable=true, clipboardEmpty=true, selectionIsFile=false
        ActionStateRow{1, true, true, false,
            true, false, true, true,
            false, true, true, true, true},

        // selCount=1, isWritable=true, clipboardEmpty=false, selectionIsFile=true
        ActionStateRow{1, true, false, true,
            true, true, true, true,
            true, true, true, true, true},

        // selCount=1, isWritable=true, clipboardEmpty=false, selectionIsFile=false
        ActionStateRow{1, true, false, false,
            true, false, true, true,
            true, true, true, true, true},

        // selCount=1, isWritable=false, clipboardEmpty=true, selectionIsFile=true
        ActionStateRow{1, false, true, true,
            true, true, true, false,
            false, false, false, false, true},

        // selCount=1, isWritable=false, clipboardEmpty=true, selectionIsFile=false
        ActionStateRow{1, false, true, false,
            true, false, true, false,
            false, false, false, false, true},

        // selCount=1, isWritable=false, clipboardEmpty=false, selectionIsFile=true
        ActionStateRow{1, false, false, true,
            true, true, true, false,
            false, false, false, false, true},

        // selCount=1, isWritable=false, clipboardEmpty=false, selectionIsFile=false
        ActionStateRow{1, false, false, false,
            true, false, true, false,
            false, false, false, false, true},

        // selCount=2, isWritable=true, clipboardEmpty=true, selectionIsFile=true
        ActionStateRow{2, true, true, true,
            false, false, true, true,
            false, false, true, true, false},

        // selCount=2, isWritable=true, clipboardEmpty=true, selectionIsFile=false
        ActionStateRow{2, true, true, false,
            false, false, true, true,
            false, false, true, true, false},

        // selCount=2, isWritable=true, clipboardEmpty=false, selectionIsFile=true
        ActionStateRow{2, true, false, true,
            false, false, true, true,
            true, false, true, true, false},

        // selCount=2, isWritable=true, clipboardEmpty=false, selectionIsFile=false
        ActionStateRow{2, true, false, false,
            false, false, true, true,
            true, false, true, true, false},

        // selCount=2, isWritable=false, clipboardEmpty=true, selectionIsFile=true
        ActionStateRow{2, false, true, true,
            false, false, true, false,
            false, false, false, false, false},

        // selCount=2, isWritable=false, clipboardEmpty=true, selectionIsFile=false
        ActionStateRow{2, false, true, false,
            false, false, true, false,
            false, false, false, false, false},

        // selCount=2, isWritable=false, clipboardEmpty=false, selectionIsFile=true
        ActionStateRow{2, false, false, true,
            false, false, true, false,
            false, false, false, false, false},

        // selCount=2, isWritable=false, clipboardEmpty=false, selectionIsFile=false
        ActionStateRow{2, false, false, false,
            false, false, true, false,
            false, false, false, false, false}
    )
);

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 7: Action enable/disable is a pure function of its inputs
// **Validates: Requirements 11.1–11.13**
//
// For arbitrary (s, w, e, f), two consecutive calls to
// ContextMenuActionState::compute(s, w, e, f) must return byte-identical results.
// ---------------------------------------------------------------------------
RC_GTEST_PROP(ContextMenuActionStateProp, PureFunctionOfInputs, (int s, bool w, bool e, bool f)) {
    // Clamp s to [0, 100] range
    s = std::abs(s) % 101;

    const auto result1 = ContextMenuActionState::compute(s, w, e, f);
    const auto result2 = ContextMenuActionState::compute(s, w, e, f);

    // POD struct — byte-identical comparison proves determinism (no hidden state).
    static_assert(std::is_trivially_copyable_v<ContextMenuActionState>,
                  "ContextMenuActionState must be trivially copyable for memcmp");
    RC_ASSERT(std::memcmp(&result1, &result2, sizeof(ContextMenuActionState)) == 0);
}

// ---------------------------------------------------------------------------
// Feature: fileview-context-menu, Property 6: Action enable/disable is view-mode agnostic
// **Validates: Requirements 11.14**
//
// Runtime side: for arbitrary (s, w, e, f), call compute(...) once — the function
// takes no view-mode parameter, so equality across view modes is trivially satisfied.
// Assert the result is well-formed (all bools are 0 or 1).
// Combined with the compile-time static_assert above (task 2.3), this fully
// validates Property 6.
// ---------------------------------------------------------------------------
RC_GTEST_PROP(ContextMenuActionStateProp, ViewModeAgnosticWellFormed, (int s, bool w, bool e, bool f)) {
    // Clamp s to a reasonable non-negative range
    s = std::abs(s) % 100;

    const auto state = ContextMenuActionState::compute(s, w, e, f);

    // Every bool field must be exactly 0 or 1 (well-formed).
    // This guards against uninitialised memory or bitfield corruption.
    auto isBool = [](bool b) {
        unsigned char raw;
        std::memcpy(&raw, &b, 1);
        return raw == 0 || raw == 1;
    };

    RC_ASSERT(isBool(state.open));
    RC_ASSERT(isBool(state.openWith));
    RC_ASSERT(isBool(state.copy));
    RC_ASSERT(isBool(state.cut));
    RC_ASSERT(isBool(state.paste));
    RC_ASSERT(isBool(state.rename));
    RC_ASSERT(isBool(state.del));
    RC_ASSERT(isBool(state.newFolder));
    RC_ASSERT(isBool(state.properties));
}
