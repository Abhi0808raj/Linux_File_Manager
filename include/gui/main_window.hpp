#pragma once
#include <QMainWindow>
class FileView;
class QLineEdit;
class QLabel;
class QListWidget;
class QListWidgetItem;
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

private:
  FileView *fileView_;
  QLineEdit *pathEntry_;
  QLabel *title_;
  QLabel *status_;
  QLabel *pathError_;
  QListWidget *places_;
  void updateLocation(const QString &path);
};
