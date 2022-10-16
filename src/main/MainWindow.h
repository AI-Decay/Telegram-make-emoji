#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>

class QListView;
class ToWebmConvertor;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();
  QSize sizeHint() const override;

 private:
  QListView* files = nullptr;
  ToWebmConvertor* convertor = nullptr;
  QHash<QUuid, bool> session;
};
#endif  // MAINWINDOW_H
