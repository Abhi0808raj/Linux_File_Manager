#pragma once
#include <QIdentityProxyModel>
#include <QSet>
#include <QString>

class CutStateProxyModel : public QIdentityProxyModel {
    Q_OBJECT
public:
    explicit CutStateProxyModel(QObject* parent = nullptr);

    void setCutPaths(const QSet<QString>& paths);   // emits dataChanged
    void clearCutPaths();                            // emits dataChanged
    bool isCut(const QString& absolutePath) const;

    QVariant data(const QModelIndex& index, int role) const override;

private:
    QSet<QString> cutPaths_;
};
