#pragma once
#include <QMainWindow>
#include "gui/clipboard_manager.hpp"
class QPushButton;
class QTabBar;
class QStackedWidget;
class QComboBox;
class QAction;
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
  FileView *fileView_ = nullptr;
  QTabBar *tabs_;
  QStackedWidget *stack_;
  QComboBox *viewMode_;
  QComboBox *sort_;
  QAction *closeTab_;
  ClipboardManager clipboard_;
  void addTab(const QString &path);
  void activateTab(int index);
  void closeTab(int index);
  void syncControls();
  QLineEdit *pathEntry_;
  QLineEdit *search_;
  QLabel *searchState_;
  QPushButton *clearSearch_;
  void updateSearchStatus(int dirs, int files);
  bool eventFilter(QObject *watched, QEvent *event) override;
  QLabel *title_;
  QLabel *status_;
  QLabel *pathError_;
  QListWidget *places_;
  void updateLocation(const QString &path);
};
