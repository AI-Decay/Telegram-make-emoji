#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QListView;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  QSize sizeHint() const override;

private:
  QListView *files;
};
#endif // MAINWINDOW_H
