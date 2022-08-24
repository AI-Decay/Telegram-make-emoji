#include "mainWindow.h"
#include "styleSheetUtility.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMovie>
#include <QScreen>
#include <QVBoxLayout>

namespace {
constexpr auto windowStyle = ":/styles/MainWindowStyles.css";
constexpr QSize WindowSizeHint(800, 600);
constexpr auto testGif = ":/images/test.gif";
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setStyleSheet(loadStyleSheet(windowStyle));

  auto *central = new QWidget();

  auto *mainLayout = new QHBoxLayout();
  auto *gifLayout = new QVBoxLayout();
  auto *operationLayout = new QVBoxLayout();

  central->setLayout(mainLayout);
  setCentralWidget(central);

  auto *labelGif = new QLabel(this);
  auto *gif = new QMovie(testGif);
  labelGif->setMovie(gif);
  gif->start();

  gifLayout->addWidget(labelGif);
  mainLayout->addLayout(gifLayout);
  mainLayout->addLayout(operationLayout);

  resize(WindowSizeHint);
  move(screen()->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow() {}

QSize MainWindow::sizeHint() const { return WindowSizeHint; }
