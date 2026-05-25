#include "gui/cut_state_proxy_model.hpp"

#include <QBrush>
#include <QColor>
#include <QFileSystemModel>

CutStateProxyModel::CutStateProxyModel(QObject* parent)
    : QIdentityProxyModel(parent)
{
}

void CutStateProxyModel::setCutPaths(const QSet<QString>& paths)
{
    cutPaths_ = paths;
    if (rowCount() > 0 && columnCount() > 0) {
        emit dataChanged(index(0, 0),
                         index(rowCount() - 1, columnCount() - 1),
                         {Qt::ForegroundRole});
    }
}

void CutStateProxyModel::clearCutPaths()
{
    cutPaths_.clear();
    if (rowCount() > 0 && columnCount() > 0) {
        emit dataChanged(index(0, 0),
                         index(rowCount() - 1, columnCount() - 1),
                         {Qt::ForegroundRole});
    }
}

bool CutStateProxyModel::isCut(const QString& absolutePath) const
{
    return cutPaths_.contains(absolutePath);
}

QVariant CutStateProxyModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::ForegroundRole) {
        const QModelIndex sourceIndex = mapToSource(index);
        auto* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (fsModel) {
            const QString filePath = fsModel->filePath(sourceIndex);
            if (isCut(filePath)) {
                // Get the base foreground color and apply 50% alpha
                QVariant baseFg = QIdentityProxyModel::data(index, Qt::ForegroundRole);
                QColor color;
                if (baseFg.isValid()) {
                    color = baseFg.value<QBrush>().color();
                } else {
                    color = QColor(Qt::black);
                }
                color.setAlpha(128);  // 50% alpha
                return QBrush(color);
            }
        }
    }
    return QIdentityProxyModel::data(index, role);
}
