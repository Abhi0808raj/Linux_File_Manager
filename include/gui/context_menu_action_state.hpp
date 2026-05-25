// include/gui/context_menu_action_state.hpp
#pragma once

struct ContextMenuActionState {
    bool open       = false;
    bool openWith   = false;
    bool copy       = false;
    bool cut        = false;
    bool paste      = false;
    bool rename     = false;
    bool del        = false;   // 'delete' is reserved
    bool newFolder  = false;
    bool properties = false;

    // Pure function of inputs — no Qt, no globals, no ordering effects.
    // Validates Requirement 11 (all sub-clauses) and Properties 6 & 7.
    static ContextMenuActionState compute(int  selectionCount,
                                          bool isWritable,
                                          bool clipboardEmpty,
                                          bool selectionIsFile);
};
