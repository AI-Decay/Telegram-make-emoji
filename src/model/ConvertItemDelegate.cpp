#include "ConvertItemDelegate.h"

#include <QApplication>
#include <QPainter>

#include "ConvertItemListModel.h"

namespace {
constexpr auto minHeight = 30;
}

ConvertItemDelegate::ConvertItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void ConvertItemDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
  // if (option.state & QStyle::State_HasFocus)

  QStyleOptionViewItem drawOption(option);
  drawOption.displayAlignment = Qt::AlignCenter;

  QStyleOptionProgressBar progressBarOption;
  progressBarOption.state |= QStyle::State_Enabled;
  progressBarOption.direction = QApplication::layoutDirection();
  progressBarOption.rect = option.rect;
  progressBarOption.fontMetrics = QFontMetrics(QApplication::font());
  progressBarOption.minimum = 0;
  progressBarOption.maximum = 100;
  progressBarOption.textVisible = false;
  progressBarOption.progress =
      index.data(ConvertItemListModel::Roles::Progress).toInt();

  QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption,
                                     painter);

  QStyledItemDelegate::paint(painter, drawOption, index);
}

QSize ConvertItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  const auto size = QStyledItemDelegate::sizeHint(option, index);
  return {size.width(), std::max(size.height(), minHeight)};
}
