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
        const auto count = files->model()->rowCount();
        std::vector<VideoProp> items;
        items.reserve(count);
        QModelIndex index;
        for (int i = 0; i < count; ++i) {
          index = files->model()->index(i, 0);
          items.push_back({index.data(FilesListModel::Roles::Uuid).toUuid(),
                           index.data(Qt::DisplayRole).toString(), 600000,
                           603000});
        }

        convertor->push(outPathLabel->text(), std::move(items));
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

  QObject::connect(
      convertor, &VideoToGifConverter::updateProgress, files,
      [this](QUuid uuid, int value) {
        if (auto* model = qobject_cast<FilesListModel*>(files->model())) {
          const auto index = model->getIndexForUuid(uuid);
          model->setData(index, value, FilesListModel::Roles::Progress);
        }
      });

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
