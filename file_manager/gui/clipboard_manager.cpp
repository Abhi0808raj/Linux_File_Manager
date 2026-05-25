#include "gui/clipboard_manager.hpp"

void ClipboardManager::set(const QList<QString>& paths, ClipboardOperation op)
{
    if (paths.isEmpty())
        return;

    paths_ = paths;
    op_    = op;
}

void ClipboardManager::clear()
{
    paths_.clear();
}

QList<QString> ClipboardManager::paths() const
{
    return paths_;
}

ClipboardOperation ClipboardManager::operation() const
{
    return op_;
}

bool ClipboardManager::isEmpty() const
{
    return paths_.isEmpty();
}
