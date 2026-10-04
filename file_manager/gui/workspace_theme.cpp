#include "gui/workspace_theme.hpp"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QStyleFactory>

namespace {
bool validTokens(const QJsonObject& value) {
    const auto colors = value["colors"].toObject();
    for (const char* role : {"surface", "surface-container", "surface-container-high",
         "on-surface", "on-surface-variant", "outline", "outline-variant", "primary",
         "on-primary", "primary-hover", "primary-pressed", "primary-container",
         "on-primary-container", "error"}) {
        if (!QColor(colors[QLatin1String(role)].toObject()["light"].toString()).isValid())
            return false;
    }
    const auto typography = value["typography"].toObject();
    if (typography["family"].toString().trimmed().isEmpty()) return false;
    const auto styles = typography["styles"].toObject();
    for (const char* role : {"Body", "Title"}) {
        const auto style = styles[QLatin1String(role)].toArray();
        if (style.isEmpty() || !style[0].isDouble() || style[0].toInt() <= 0) return false;
    }
    const auto radii = value["radius"].toObject();
    for (const char* name : {"small", "medium", "large", "panel"}) {
        const auto radius = radii[QLatin1String(name)];
        if (!radius.isDouble() || radius.toInt(-1) < 0) return false;
    }
    const auto density = value["density"].toObject()["comfortable"].toObject();
    for (const char* name : {"rowHeight", "controlHeight", "navHeight"}) {
        const auto metric = density[QLatin1String(name)];
        if (!metric.isDouble() || metric.toInt() <= 2) return false;
    }
    return true;
}

const QJsonObject& tokens() {
    static const QJsonObject value = [] {
        QFile file(":/workspace-tokens.json");
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open workspace design tokens";
            return QJsonObject{};
        }
        const auto document = QJsonDocument::fromJson(file.readAll());
        if (!document.isObject() || !validTokens(document.object())) {
            qWarning() << "Invalid workspace design tokens; retaining application theme";
            return QJsonObject{};
        }
        return document.object();
    }();
    return value;
}

QString color(const char* role) {
    return tokens()["colors"].toObject()[QLatin1String(role)].toObject()["light"].toString();
}
}

int workspaceMetric(const char* name) {
    const QString key = QString::fromLatin1(name);
    const int fallback = key == "rowHeight" ? 48 : key == "navHeight" ? 44 : 40;
    return tokens()["density"].toObject()["comfortable"].toObject()[key].toInt(fallback);
}

void applyWorkspaceTheme(QApplication& application) {
    if (tokens().isEmpty()) return;
    const auto typography = tokens()["typography"].toObject();
    const auto styles = typography["styles"].toObject();
    QFont font = application.font();
    const QString requestedFamily = typography["family"].toString();
    if (QFontDatabase::families().contains(requestedFamily, Qt::CaseInsensitive))
        font.setFamily(requestedFamily);
    font.setPixelSize(styles["Body"].toArray()[0].toInt());

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(color("surface")));
    palette.setColor(QPalette::Base, QColor(color("surface")));
    palette.setColor(QPalette::AlternateBase, QColor(color("surface-container")));
    palette.setColor(QPalette::Button, QColor(color("surface-container")));
    palette.setColor(QPalette::WindowText, QColor(color("on-surface")));
    palette.setColor(QPalette::Text, QColor(color("on-surface")));
    palette.setColor(QPalette::ButtonText, QColor(color("on-surface")));
    palette.setColor(QPalette::Highlight, QColor(color("primary-container")));
    palette.setColor(QPalette::HighlightedText, QColor(color("on-primary-container")));
    palette.setColor(QPalette::Link, QColor(color("primary")));
    palette.setColor(QPalette::PlaceholderText, QColor(color("on-surface-variant")));
    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        palette.setColor(QPalette::Disabled, role, QColor(color("outline")));

    QFile file(":/stylesheet.qss");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open workspace stylesheet";
        return;
    }
    QString sheet = QString::fromUtf8(file.readAll());
    const auto colors = tokens()["colors"].toObject();
    for (auto it = colors.begin(); it != colors.end(); ++it)
        sheet.replace("@" + it.key() + "@", it.value().toObject()["light"].toString());
    const auto radii = tokens()["radius"].toObject();
    for (auto it = radii.begin(); it != radii.end(); ++it)
        sheet.replace("@radius-" + it.key() + "@", QString::number(it.value().toInt()));
    for (const char* metric : {"rowHeight", "controlHeight", "navHeight"})
        sheet.replace("@" + QString::fromLatin1(metric) + "@", QString::number(workspaceMetric(metric)));
    sheet.replace("@controlInnerHeight@", QString::number(workspaceMetric("controlHeight") - 2));
    sheet.replace("@bodySize@", QString::number(styles["Body"].toArray()[0].toInt()));
    sheet.replace("@titleSize@", QString::number(styles["Title"].toArray()[0].toInt()));
    if (sheet.contains('@')) {
        qWarning() << "Unresolved workspace stylesheet tokens; retaining application theme";
        return;
    }
    application.setStyle(QStyleFactory::create("Fusion"));
    application.setFont(font);
    application.setPalette(palette);
    application.setStyleSheet(sheet);
}
