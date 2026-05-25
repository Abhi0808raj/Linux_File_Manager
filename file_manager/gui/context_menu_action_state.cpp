#include "gui/context_menu_action_state.hpp"

ContextMenuActionState ContextMenuActionState::compute(int  selectionCount,
                                                       bool isWritable,
                                                       bool clipboardEmpty,
                                                       bool selectionIsFile)
{
    ContextMenuActionState state;

    state.open       = (selectionCount == 1);
    state.openWith   = (selectionCount == 1 && selectionIsFile);
    state.copy       = (selectionCount > 0);
    state.cut        = (selectionCount > 0 && isWritable);
    state.paste      = (!clipboardEmpty && isWritable);
    state.rename     = (selectionCount == 1 && isWritable);
    state.del        = (selectionCount > 0 && isWritable);
    state.newFolder  = isWritable;
    state.properties = (selectionCount == 1);

    return state;
}
