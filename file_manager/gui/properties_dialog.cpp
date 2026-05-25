// file_manager/gui/properties_dialog.cpp
#include "gui/properties_dialog.hpp"
#include "core/file_system.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <chrono>
#include <filesystem>

// ---------------------------------------------------------------------------
// Static helpers (internal linkage)
// ---------------------------------------------------------------------------

namespace {

/// Formats a byte count using 1024-based units with 1 decimal place.
/// Returns "—" for directories (indicated by passing std::nullopt).
QString formatSize(std::optional<uintmax_t> bytes)
{
    if (!bytes.has_value())
        return QStringLiteral("\u2014"); // em-dash for directories

    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    constexpr int unitCount = 5;

    double value = static_cast<double>(bytes.value());
    int unitIndex = 0;

    while (value >= 1024.0 && unitIndex < unitCount - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    return QString::number(value, 'f', 1) + " " + units[unitIndex];
}

/// Formats filesystem permissions as a 4-digit octal string (e.g. "0755").
QString formatPermissions(const QString& absolutePath)
{
    std::error_code ec;
    auto status = std::filesystem::status(absolutePath.toStdString(), ec);
    if (ec)
        return QString();

    auto perms = status.permissions();
    return QString::asprintf("%04o", static_cast<unsigned>(perms) & 07777);
}

/// Converts std::filesystem::file_time_type to QDateTime and formats as
/// "yyyy-MM-dd HH:mm:ss".
QString formatModifiedDate(std::optional<std::filesystem::file_time_type> fileTime)
{
    if (!fileTime.has_value())
        return QString();

    // Convert file_time_type to system_clock time_point (C++17-compatible).
    // file_clock and system_clock share the same epoch on most implementations;
    // use duration arithmetic to avoid the C++20-only clock_cast.
    auto fileTP = fileTime.value();
    auto fileDuration = fileTP.time_since_epoch();
    auto sctp = std::chrono::time_point<std::chrono::system_clock,
                    decltype(fileDuration)>(fileDuration);
    auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                     sctp.time_since_epoch())
                     .count();

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(epoch));
    return dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// PropertiesDialog implementation
// ---------------------------------------------------------------------------

PropertiesDialog::PropertiesDialog(const QString& absolutePath, QWidget* parent)
    : QDialog(parent)
{
    // Determine type
    bool isFile = FileSystem::isFile(absolutePath.toStdString());
    bool isDir  = FileSystem::isDirectory(absolutePath.toStdString());

    if (!isFile && !isDir) {
        // Path doesn't exist or is neither file nor directory
        valid_ = false;
        return;
    }

    valid_ = true;

    // Populate view_ struct
    QFileInfo fi(absolutePath);
    view_.path = absolutePath;
    view_.type = isFile ? QStringLiteral("File") : QStringLiteral("Directory");

    // Size: use FileSystem::fileSize for files, nullopt for directories
    if (isFile) {
        auto sizeOpt = FileSystem::fileSize(absolutePath.toStdString());
        if (!sizeOpt.has_value()) {
            valid_ = false;
            return;
        }
        view_.size = formatSize(sizeOpt);
    } else {
        view_.size = formatSize(std::nullopt); // "—" for directories
    }

    // Modified date
    auto timeOpt = FileSystem::lastWriteTime(absolutePath.toStdString());
    if (!timeOpt.has_value()) {
        valid_ = false;
        return;
    }
    view_.modified = formatModifiedDate(timeOpt);
    if (view_.modified.isEmpty()) {
        valid_ = false;
        return;
    }

    // Permissions
    view_.permissions = formatPermissions(absolutePath);
    if (view_.permissions.isEmpty()) {
        valid_ = false;
        return;
    }

    // --- Build the UI ---
    setWindowTitle(QStringLiteral("Properties \u2014 %1").arg(fi.fileName()));

    auto* mainLayout = new QVBoxLayout(this);
    auto* grid = new QGridLayout();

    auto addRow = [&](int row, const QString& label, const QString& value) {
        auto* lbl = new QLabel(label, this);
        QFont boldFont = lbl->font();
        boldFont.setBold(true);
        lbl->setFont(boldFont);

        auto* val = new QLabel(value, this);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);

        grid->addWidget(lbl, row, 0);
        grid->addWidget(val, row, 1);
    };

    addRow(0, QStringLiteral("Path:"),        view_.path);
    addRow(1, QStringLiteral("Type:"),        view_.type);
    addRow(2, QStringLiteral("Size:"),        view_.size);
    addRow(3, QStringLiteral("Modified:"),    view_.modified);
    addRow(4, QStringLiteral("Permissions:"), view_.permissions);

    mainLayout->addLayout(grid);
    mainLayout->addStretch();

    // Close button
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);

    setMinimumWidth(400);
}

// static
bool PropertiesDialog::showFor(const QString& absolutePath, QWidget* parent)
{
    auto* dlg = new PropertiesDialog(absolutePath, parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    if (!dlg->valid_) {
        delete dlg;
        return false;
    }

    dlg->show();
    return true;
}
