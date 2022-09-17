#include "fileDelegate.h"
#include <QApplication>

FileDelegate::FileDelegate(QObject* parent) : QItemDelegate(parent) {}

void FileDelegate::paint(QPainter* painter,
                         const QStyleOptionViewItem& option,
                         const QModelIndex& index) const {
  // Set up a QStyleOptionProgressBar to precisely mimic the
  // environment of a progress bar.
  QStyleOptionProgressBar progressBarOption;
  progressBarOption.state |= QStyle::State_Enabled;
  progressBarOption.direction = QApplication::layoutDirection();
  progressBarOption.rect = option.rect;
  progressBarOption.fontMetrics = QFontMetrics(QApplication::font());
  progressBarOption.minimum = 0;
  progressBarOption.maximum = 100;
  progressBarOption.textAlignment = Qt::AlignCenter;
  progressBarOption.textVisible = true;

  // Set the progress and text values of the style option.
  int progress = 25;
  progressBarOption.progress = progress < 0 ? 0 : progress;
  progressBarOption.text = index.data(Qt::DisplayRole).toString();

  // Draw the progress bar onto the view.
  QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption,
                                     painter);
}
