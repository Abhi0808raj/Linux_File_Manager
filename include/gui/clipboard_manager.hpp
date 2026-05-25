#pragma once
#include <QList>
#include <QString>

enum class ClipboardOperation { Copy, Cut };

class ClipboardManager {
public:
    ClipboardManager() = default;

    // Replace contents. No-op if `paths` is empty (Req 12.11).
    void set(const QList<QString>& paths, ClipboardOperation op);

    // Empty the clipboard.
    void clear();

    // Accessors.
    QList<QString>     paths() const;
    ClipboardOperation operation() const;
    bool               isEmpty() const;

private:
    QList<QString>     paths_;
    ClipboardOperation op_ = ClipboardOperation::Copy;  // defined-but-unused when empty
};
