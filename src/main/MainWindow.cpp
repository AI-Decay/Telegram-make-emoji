#include "MainWindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <algorithm>

#include "converter/ToWebmConvertor.h"
#include "custom/InputSliderWidget.h"
#include "model/ConvertItemDelegate.h"
#include "model/ConvertItemListModel.h"
#include "utility/StyleSheetUtility.h"

namespace {
constexpr auto DefaultEndPos = 3000;
constexpr auto WindowStyle = ":/styles/MainWindowStyles.css";
constexpr QSize WindowSizeHint(800, 400);
constexpr QSize ButtonMinSize(200, 50);
constexpr QSize OperationWidgetMinSize(300, 300);
constexpr QSize FilesListMinSize(500, 300);
constexpr auto ConvertButtonText = "Convert";
constexpr auto SelectPathButtonText = "Select output path";
constexpr auto OutPathLabelObjectName = "OutPathLabel";
constexpr auto Organization = "AiDecay";
constexpr auto Application = "TgGifToEmoji";
constexpr auto OutputPathKey = "OutputPath";
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setStyleSheet(loadStyleSheet(WindowStyle));
  ;
  const auto outputPath =
      QSettings(Organization, Application)
          .value(OutputPathKey, QStandardPaths::writableLocation(
                                    QStandardPaths::DesktopLocation))
          .toString();

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

  outPathLabel->setText(outputPath);
  convertButton->setMinimumSize(ButtonMinSize);
  outPathLabel->setMinimumSize(ButtonMinSize);
  selectPathButton->setMinimumSize(ButtonMinSize);
  operationWidget->setMinimumSize(OperationWidgetMinSize);

  operationLayout->addWidget(convertButton);
  operationLayout->addStretch(1);
  operationLayout->addWidget(operationWidget);
  operationLayout->addWidget(inputWidget);
  operationLayout->addWidget(outPathLabel);
  operationLayout->addWidget(selectPathButton);

  operationLayout->setAlignment(Qt::AlignmentFlag::AlignTop);

  files = new QListView(this);
  files->setModel(new ConvertItemListModel());
  files->setSelectionMode(QAbstractItemView::SingleSelection);
  files->setDragEnabled(true);
  files->viewport()->setAcceptDrops(true);
  files->setDropIndicatorShown(true);
  files->setDragDropMode(QAbstractItemView::DropOnly);
  files->setItemDelegate(new ConvertItemDelegate(files));
  files->setMinimumSize(FilesListMinSize);

  QObject::connect(
      selectPathButton, &QPushButton::clicked, outPathLabel,
      [outPathLabel, this]() {
        const auto path = QFileDialog::getExistingDirectory(
            this, QString(), outPathLabel->text());
        QSettings(Organization, Application).setValue(OutputPathKey, path);
        outPathLabel->setText(path);
      });

  QObject::connect(
      convertButton, &QPushButton::clicked, outPathLabel,
      [convertButton, inputWidget, outPathLabel, this]() {
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
          const auto uuid =
              index.data(ConvertItemListModel::Roles::Uuid).toUuid();
          session.insert(uuid, false);
          items.push_back({
              uuid,
              index.data(Qt::DisplayRole).toString(),
              begin,
              end,
          });
        }

        convertor->push(outPathLabel->text(), std::move(items));
        convertButton->setEnabled(false);
      });

  QObject::connect(
      convertor, &ToWebmConvertor::updateProgress, files,
      [convertButton, this](QUuid uuid, int value) {
        if (auto* model = qobject_cast<ConvertItemListModel*>(files->model())) {
          const auto index = model->getIndexForUuid(uuid);
          model->setData(index, value, ConvertItemListModel::Roles::Progress);
          if (value == 100 || value == -1) {
            session[uuid] = true;
            const auto isAllReady =
                std::all_of(session.begin(), session.end(),
                            [](const auto pair) { return pair; });
            if (isAllReady) {
              model->removeRows(0, session.size());
              convertButton->setEnabled(true);
            }
          }
        }
      });

  QObject::connect(
      files->selectionModel(), &QItemSelectionModel::currentRowChanged, files,
      [this, inputWidget](const QModelIndex& current,
                          const QModelIndex& previous) {
        if (previous.isValid()) {
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
