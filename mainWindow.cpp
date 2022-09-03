#include "mainWindow.h"
#include "filesListModel.h"
#include "styleSheetUtility.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QMovie>
#include <QPushButton>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace {
constexpr auto windowStyle = ":/styles/MainWindowStyles.css";
constexpr QSize WindowSizeHint(800, 600);
constexpr auto testGif = ":/images/test.gif";
constexpr auto convertButtonText = "Convert";
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setStyleSheet(loadStyleSheet(windowStyle));

  auto *central = new QWidget();

  auto *mainLayout = new QHBoxLayout();
  auto *gifLayout = new QVBoxLayout();
  auto *operationLayout = new QVBoxLayout();

  auto *operationWidget = new QWidget();
  auto *convertButton = new QPushButton(convertButtonText, operationWidget);
  convertButton->setMinimumSize(200, 50);
  operationWidget->setMinimumSize(400, 300);
  operationLayout->addWidget(operationWidget);

  files = new QListView(this);
  files->setModel(new FilesListModel());
  files->setSelectionMode(QAbstractItemView::ExtendedSelection);
  files->setDragEnabled(true);
  files->viewport()->setAcceptDrops(true);
  files->setDropIndicatorShown(true);
  files->setDragDropMode(QAbstractItemView::DropOnly);

  central->setLayout(mainLayout);
  setCentralWidget(central);

  auto *labelGif = new QLabel(this);
  auto *gif = new QMovie(testGif);
  labelGif->setMovie(gif);
  gif->start();

  gifLayout->addWidget(labelGif);
  gifLayout->addWidget(files);
  mainLayout->addLayout(gifLayout);
  mainLayout->addLayout(operationLayout);

  resize(WindowSizeHint);
  move(screen()->geometry().center() - frameGeometry().center());
}

MainWindow::~MainWindow() {}

QSize MainWindow::sizeHint() const { return WindowSizeHint; }
