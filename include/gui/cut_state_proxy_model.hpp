#pragma once
#include <QSortFilterProxyModel>
#include <QSet>
#include <QString>

class CutStateProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit CutStateProxyModel(QObject* parent = nullptr);

    void setCutPaths(const QSet<QString>& paths);   // emits dataChanged
    void clearCutPaths();                            // emits dataChanged
    bool isCut(const QString& absolutePath) const;

    QVariant data(const QModelIndex& index, int role) const override;
    void setFolderSearch(const QString& path, const QString& query);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override;

private:
    QSet<QString> cutPaths_;
    QString searchPath_, query_;
};
