#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTreeWidgetItem>

// Forward declaration of the auto-generated UI class
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileTableView_doubleClicked(const QModelIndex &index);
    void on_navigationTreeWidget_itemClicked(QTreeWidgetItem *item, int column);
    void on_actionUp_triggered();
    void on_actionBack_triggered();
    void on_actionForward_triggered();

private:
    void setupModels();
    void setupConnections();
    void navigateToPath(const QString& path);

    Ui::MainWindow *ui;
    QFileSystemModel *m_fileSystemModel;
    QList<QString> m_history;
    int m_historyIndex = -1;
};