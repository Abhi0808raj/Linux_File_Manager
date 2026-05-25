#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileSystemModel>
#include <QTableView>
#include <QListView>
#include <QStackedWidget>

class FileView : public QWidget {
    Q_OBJECT

public:
    explicit FileView(QWidget* parent = nullptr);
    ~FileView();

private:
    QFileSystemModel* model_ = nullptr;
    QTableView* table_view_ = nullptr;
    QListView* list_view_ = nullptr;
    QStackedWidget* stacked_widget_ = nullptr;
};
