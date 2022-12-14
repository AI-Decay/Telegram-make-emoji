#ifndef CONVERTITEMLISTMODEL_H
#define CONVERTITEMLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QUuid>

#include "utility/MultiIndex.h"

class ConvertItem;

class ConvertItemListModel : public QAbstractListModel {
  Q_OBJECT
 public:
  ConvertItemListModel();
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index,
               const QVariant& value,
               int role = Qt::EditRole) override;
  bool removeRows(int row,
                  int count,
                  const QModelIndex& parent = QModelIndex()) override;
  void updateProgress(QUuid uuid, int value);
  QModelIndex getIndexForUuid(const QUuid uuid);
  Qt::DropActions supportedDropActions() const override;
  bool dropMimeData(const QMimeData* data,
                    Qt::DropAction action,
                    int row,
                    int column,
                    const QModelIndex& parent) override;
  bool canDropMimeData(const QMimeData* data,
                       Qt::DropAction action,
                       int row,
                       int column,
                       const QModelIndex& parent) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  enum Roles { Uuid = Qt::UserRole + 1, Progress, Duration, BeginPos, EndPos };

 private:
  MultiIndex<ConvertItem> items;
};

#endif  // CONVERTITEMLISTMODEL_H
