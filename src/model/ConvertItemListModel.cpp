#include "ConvertItemListModel.h"

#include <QDebug>
#include <QMimeData>
#include <QUrl>
#include <QUuid>

#include "ConvertItem.h"
#include "utility/FFmpegUtility.h"

ConvertItemListModel::ConvertItemListModel() {}

int ConvertItemListModel::rowCount(const QModelIndex& parent) const {
  return items.size();
}

int ConvertItemListModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

QVariant ConvertItemListModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole) {
    return items[index.row()].getFileName();
  } else if (role == Roles::Uuid) {
    return items.getUuid(index.row());
  } else if (role == Roles::Progress) {
    return items[index.row()].getProgress();
  } else if (role == Roles::Duration) {
    return items[index.row()].getDurationMs();
  } else if (role == Roles::BeginPos) {
    return items[index.row()].getBeginPosMs();
  } else if (role == Roles::EndPos) {
    return items[index.row()].getEndPosMs();
  }

  return QVariant();
}

bool ConvertItemListModel::setData(const QModelIndex& index,
                                   const QVariant& value, int role) {
  if (role == Roles::Progress) {
    const auto progress = value.toInt();
    if (progress == 100)
      items.erace(index.row());
    else
      items[index.row()].setProgress(value.toInt());
    emit dataChanged(index, index, {role});
    return true;
  } else if (role == Roles::BeginPos) {
    items[index.row()].setBeginPosMs(value.toInt());
    return true;
  } else if (role == Roles::EndPos) {
    items[index.row()].setEndPosMs(value.toInt());
    return true;
  }

  return QAbstractListModel::setData(index, value, role);
}

void ConvertItemListModel::updateProgress(QUuid uuid, int value) {
  items[uuid].setProgress(value);
}

QModelIndex ConvertItemListModel::getIndexForUuid(const QUuid uuid) {
  return index(items.getKey(uuid));
}

Qt::DropActions ConvertItemListModel::supportedDropActions() const {
  return Qt::CopyAction | Qt::MoveAction;
}

bool ConvertItemListModel::dropMimeData(const QMimeData* data,
                                        Qt::DropAction action, int row,
                                        int column, const QModelIndex& parent) {
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
        const auto localFile = url.toLocalFile();
        const auto duration = getVideoDurationMs(localFile);
        if (duration > 0) {
          items.push_back(ConvertItem(localFile, duration));
        }
      }
    }

    endInsertRows();
  }

  return localFilesCount > 0;
}

bool ConvertItemListModel::canDropMimeData(const QMimeData* data,
                                           Qt::DropAction action, int row,
                                           int column,
                                           const QModelIndex& parent) const {
  return true;
}

Qt::ItemFlags ConvertItemListModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) {
    return Qt::ItemIsDropEnabled;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;
}
