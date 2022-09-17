#ifndef FILESLISTMODEL_H
#define FILESLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include "utility/multiIndex.h"

class File;

class FilesListModel : public QAbstractListModel {
 public:
  FilesListModel();
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
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

  enum Roles { QUuid = Qt::UserRole + 1 };

 private:
  MultiIndex<File> items;
};

#endif  // FILESLISTMODEL_H
