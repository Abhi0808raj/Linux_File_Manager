#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class FileView : public QWidget {
    Q_OBJECT

public:
    explicit FileView(QWidget* parent = nullptr);
    ~FileView();
};
