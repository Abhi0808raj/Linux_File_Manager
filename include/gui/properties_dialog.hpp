// include/gui/properties_dialog.hpp
#pragma once
#include <QDialog>
#include <QString>

// Holds the rendered field values displayed in the dialog.
// Exposed for test access via PropertiesDialogTestAccess.
struct PropertiesView {
    QString path;         // Full absolute path
    QString type;         // "File" or "Directory"
    QString size;         // 1024-based human-readable (e.g. "4.2 KB") or "—" for directories
    QString modified;     // yyyy-MM-dd HH:mm:ss
    QString permissions;  // 4-digit octal (e.g. "0755")
};

class PropertiesDialog : public QDialog {
    Q_OBJECT
public:
    explicit PropertiesDialog(const QString& absolutePath, QWidget* parent = nullptr);
    ~PropertiesDialog() override = default;

    // Static convenience: constructs, sets WA_DeleteOnClose, calls show().
    // Returns true if metadata for the path could be read (Req 10.6).
    static bool showFor(const QString& absolutePath, QWidget* parent);

private:
    bool valid_ = false;
    PropertiesView view_;

    friend class PropertiesDialogTestAccess;
};
