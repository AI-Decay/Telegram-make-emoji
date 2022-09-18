#include "MainWindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QMovie>
#include <QPushButton>
#include <QScreen>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "converter/ToWebmConvertor.h"
#include "custom/InputSliderWidget.h"
#include "model/ConvertItemDelegate.h"
#include "model/ConvertItemListModel.h"
#include "utility/StyleSheetUtility.h"

namespace {
constexpr auto DefaultEndPos = 3000;
constexpr auto WindowStyle = ":/styles/MainWindowStyles.css";
constexpr QSize WindowSizeHint(800, 600);
constexpr auto testGif = ":/images/test.gif";
constexpr auto ConvertButtonText = "Convert";
constexpr auto SelectPathButtonText = "Select path";
constexpr auto OutPathLabelObjectName = "OutPathLabel";
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setStyleSheet(loadStyleSheet(WindowStyle));

  convertor = new ToWebmConvertor(this);
  auto* central = new QWidget();

  auto* mainLayout = new QHBoxLayout();
  auto* gifLayout = new QVBoxLayout();
  auto* operationLayout = new QVBoxLayout();
  auto* inputWidget = new InputSliderWidget();
  inputWidget->setEnabled(false);

  auto* operationWidget = new QWidget();
  auto* convertButton = new QPushButton(ConvertButtonText, operationWidget);
  auto* outPathLabel =
      createWidget<QLabel>(operationWidget, OutPathLabelObjectName);
  auto* selectPathButton =
      new QPushButton(SelectPathButtonText, operationWidget);

  QObject::connect(
      selectPathButton, &QPushButton::clicked, outPathLabel,
      [outPathLabel, this]() {
        outPathLabel->setText(QFileDialog::getExistingDirectory(this));
      });

  QObject::connect(
      convertButton, &QPushButton::clicked, outPathLabel,
      [inputWidget, outPathLabel, this]() {
        const auto count = files->model()->rowCount();
        std::vector<VideoProp> items;
        items.reserve(count);

        for (auto& index : files->selectionModel()->selectedRows()) {
          if (auto* model =
                  qobject_cast<ConvertItemListModel*>(files->model())) {
            const auto begin = inputWidget->getBegin();
            const auto end = inputWidget->getEnd();
            model->setData(index, begin, ConvertItemListModel::Roles::BeginPos);
            model->setData(index, end, ConvertItemListModel::Roles::EndPos);
          }
        }

        QModelIndex index;
        for (int i = 0; i < count; ++i) {
          index = files->model()->index(i, 0);
          const auto begin =
              index.data(ConvertItemListModel::Roles::BeginPos).toInt();
          auto end = index.data(ConvertItemListModel::Roles::EndPos).toInt();
          if (end == 0)
            end = std::min(
                index.data(ConvertItemListModel::Roles::Duration).toInt(),
                DefaultEndPos);
          items.push_back({
              index.data(ConvertItemListModel::Roles::Uuid).toUuid(),
              index.data(Qt::DisplayRole).toString(),
              begin,
              end,
          });
        }

        convertor->push(outPathLabel->text(), std::move(items));
      });

  convertButton->setMinimumSize(200, 50);
  outPathLabel->setMinimumSize(200, 40);
  selectPathButton->setMinimumSize(200, 50);
  operationWidget->setMinimumSize(400, 300);

  operationLayout->addWidget(convertButton);
  operationLayout->addWidget(operationWidget);
  operationLayout->addWidget(inputWidget);
  operationLayout->addWidget(outPathLabel);
  operationLayout->addWidget(selectPathButton);

  operationLayout->setAlignment(Qt::AlignVCenter);

  files = new QListView(this);
  files->setModel(new ConvertItemListModel());
  files->setSelectionMode(QAbstractItemView::SingleSelection);
  files->setDragEnabled(true);
  files->viewport()->setAcceptDrops(true);
  files->setDropIndicatorShown(true);
  files->setDragDropMode(QAbstractItemView::DropOnly);
  files->setItemDelegate(new ConvertItemDelegate(files));

  QObject::connect(
      convertor, &ToWebmConvertor::updateProgress, files,
      [this](QUuid uuid, int value) {
        if (auto* model = qobject_cast<ConvertItemListModel*>(files->model())) {
          const auto index = model->getIndexForUuid(uuid);
          model->setData(index, value, ConvertItemListModel::Roles::Progress);
        }
      });

  QObject::connect(
      files->selectionModel(), &QItemSelectionModel::currentRowChanged, files,
      [this, inputWidget](const QModelIndex& current,
                          const QModelIndex& previous) {
        if (previous.isValid()) {
          qDebug() << previous.row();
          if (auto* model =
                  qobject_cast<ConvertItemListModel*>(files->model())) {
            const auto begin = inputWidget->getBegin();
            const auto end = inputWidget->getEnd();
            model->setData(previous, begin,
                           ConvertItemListModel::Roles::BeginPos);
            model->setData(previous, end, ConvertItemListModel::Roles::EndPos);
          }
        }

        if (current.isValid()) {
          inputWidget->setEnabled(true);
          inputWidget->setLimit(
              current.data(ConvertItemListModel::Roles::Duration).toInt());
          if (auto* model =
                  qobject_cast<ConvertItemListModel*>(files->model())) {
            const auto begin =
                model->data(current, ConvertItemListModel::Roles::BeginPos)
                    .toInt();
            const auto end =
                model->data(current, ConvertItemListModel::Roles::EndPos)
                    .toInt();
            inputWidget->setBegin(begin);
            inputWidget->setEnd(end);
          }

        } else {
          inputWidget->setEnabled(false);
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

QSize MainWindow::sizeHint() const { return WindowSizeHint; }