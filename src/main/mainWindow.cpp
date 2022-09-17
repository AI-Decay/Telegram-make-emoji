#include "mainWindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QMovie>
#include <QPushButton>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "converter/videoToGifConverter.h"
#include "model/fileDelegate.h"
#include "model/filesListModel.h"
#include "utility/styleSheetUtility.h"

namespace {
constexpr auto windowStyle = ":/styles/MainWindowStyles.css";
constexpr QSize WindowSizeHint(800, 600);
constexpr auto testGif = ":/images/test.gif";
constexpr auto convertButtonText = "Convert";
constexpr auto selectPathButtonText = "Select path";
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setStyleSheet(loadStyleSheet(windowStyle));

  convertor = new VideoToGifConverter(this);
  auto* central = new QWidget();

  auto* mainLayout = new QHBoxLayout();
  auto* gifLayout = new QVBoxLayout();
  auto* operationLayout = new QVBoxLayout();

  auto* operationWidget = new QWidget();
  auto* convertButton = new QPushButton(convertButtonText, operationWidget);
  auto* outPathLabel = new QLabel(operationWidget);
  auto* selectPathButton =
      new QPushButton(selectPathButtonText, operationWidget);

  QObject::connect(
      selectPathButton, &QPushButton::clicked, outPathLabel,
      [outPathLabel, this]() {
        outPathLabel->setText(QFileDialog::getExistingDirectory(this));
      });

  QObject::connect(
      convertButton, &QPushButton::clicked, outPathLabel,
      [outPathLabel, this]() {
        std::vector<VideoProp> vector;
        const auto selected = files->selectionModel()->selectedIndexes();
        for (auto item : selected) {
          vector.push_back({item.data(FilesListModel::Roles::QUuid).toUuid(),
                            item.data(Qt::DisplayRole).toString(), 0, 3});
        }

        convertor->push(outPathLabel->text(), std::move(vector));
      });

  convertButton->setMinimumSize(200, 50);
  outPathLabel->setMinimumSize(200, 50);
  selectPathButton->setMinimumSize(200, 50);
  operationWidget->setMinimumSize(400, 300);
  operationLayout->addWidget(operationWidget);
  operationLayout->addWidget(outPathLabel);
  operationLayout->addWidget(selectPathButton);

  files = new QListView(this);
  files->setModel(new FilesListModel());
  files->setSelectionMode(QAbstractItemView::ExtendedSelection);
  files->setDragEnabled(true);
  files->viewport()->setAcceptDrops(true);
  files->setDropIndicatorShown(true);
  files->setDragDropMode(QAbstractItemView::DropOnly);
  files->setItemDelegate(new FileDelegate(files));

  central->setLayout(mainLayout);
  setCentralWidget(central);

  auto* labelGif = new QLabel(this);
  auto* gif = new QMovie(testGif);
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

QSize MainWindow::sizeHint() const {
  return WindowSizeHint;
}
