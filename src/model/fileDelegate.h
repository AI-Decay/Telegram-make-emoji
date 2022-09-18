#ifndef FILEDELEGATE_H
#define FILEDELEGATE_H

#include <QStyledItemDelegate>

class FileDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  FileDelegate(QObject* parent = nullptr);
  void paint(QPainter* painter,
             const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const override;
};

#endif  // FILEDELEGATE_H
