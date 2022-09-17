#ifndef FILEDELEGATE_H
#define FILEDELEGATE_H

#include <QItemDelegate>

class FileDelegate : public QItemDelegate {
  Q_OBJECT
 public:
  FileDelegate(QObject* parent = nullptr);
  void paint(QPainter* painter,
             const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
};

#endif  // FILEDELEGATE_H
