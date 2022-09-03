#include "filesListModel.h"
#include "file.h"
#include <QDebug>
#include <QMimeData>
#include <QUrl>

FilesListModel::FilesListModel() {}

int FilesListModel::rowCount(const QModelIndex &parent) const {
  return list.size();
}

int FilesListModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant FilesListModel::data(const QModelIndex &index, int role) const {
  if (role == Qt::DisplayRole) {
    return list.at(index.row()).getFileName();
  }

  return QVariant();
}

Qt::DropActions FilesListModel::supportedDropActions() const {
  return Qt::CopyAction | Qt::MoveAction;
}

bool FilesListModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                  int row, int column,
                                  const QModelIndex &parent) {
  Q_UNUSED(row);
  Q_UNUSED(column);
  Q_UNUSED(action);

  const auto allUrls = data->urls();
  const auto localFilesCount =
      std::count_if(allUrls.begin(), allUrls.end(),
                    [](const QUrl &u) { return u.isLocalFile(); });

  if (localFilesCount > 0) {
    beginInsertRows(parent, list.size(), list.size() + localFilesCount);

    for (const auto &url : allUrls) {
      if (url.isLocalFile()) {
        list.push_back(File(url.toLocalFile()));
      }
    }

    endInsertRows();
  }

  return localFilesCount > 0;
}

bool FilesListModel::canDropMimeData(const QMimeData *data,
                                     Qt::DropAction action, int row, int column,
                                     const QModelIndex &parent) const {
  return true;
}

Qt::ItemFlags FilesListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return Qt::ItemIsDropEnabled;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
}
