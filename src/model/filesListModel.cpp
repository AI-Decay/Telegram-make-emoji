#include "filesListModel.h"
#include <QDebug>
#include <QMimeData>
#include <QUrl>
#include <QUuid>
#include "file.h"

FilesListModel::FilesListModel() {}

int FilesListModel::rowCount(const QModelIndex& parent) const {
  return items.size();
}

int FilesListModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

QVariant FilesListModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole) {
    return items[index.row()].getFileName();
  } else if (role == Roles::Uuid) {
    return items.getUuid(index.row());
  } else if (role == Roles::Progress) {
    return items[index.row()].getProgress();
  }

  return QVariant();
}

bool FilesListModel::setData(const QModelIndex& index,
                             const QVariant& value,
                             int role) {
  if (role == Roles::Progress) {
    items[index.row()].setProgress(value.toInt());
    emit dataChanged(index, index, {role});
    return true;
  } else {
    return QAbstractListModel::setData(index, value, role);
  }
}

void FilesListModel::updateProgress(QUuid uuid, int value) {
  items[uuid].setProgress(value);
}

QModelIndex FilesListModel::getIndexForUuid(const QUuid uuid) {
  return index(items.getKey(uuid));
}

Qt::DropActions FilesListModel::supportedDropActions() const {
  return Qt::CopyAction | Qt::MoveAction;
}

bool FilesListModel::dropMimeData(const QMimeData* data,
                                  Qt::DropAction action,
                                  int row,
                                  int column,
                                  const QModelIndex& parent) {
  Q_UNUSED(row);
  Q_UNUSED(column);
  Q_UNUSED(action);

  const auto allUrls = data->urls();
  const auto localFilesCount =
      std::count_if(allUrls.begin(), allUrls.end(),
                    [](const QUrl& u) { return u.isLocalFile(); });

  if (localFilesCount > 0) {
    beginInsertRows(parent, items.size(), items.size() + localFilesCount);

    for (const auto& url : allUrls) {
      if (url.isLocalFile()) {
        items.push_back(File(url.toLocalFile()));
      }
    }

    endInsertRows();
  }

  return localFilesCount > 0;
}

bool FilesListModel::canDropMimeData(const QMimeData* data,
                                     Qt::DropAction action,
                                     int row,
                                     int column,
                                     const QModelIndex& parent) const {
  return true;
}

Qt::ItemFlags FilesListModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) {
    return Qt::ItemIsDropEnabled;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
}
