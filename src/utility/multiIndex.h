#ifndef MULTIINDEX_H
#define MULTIINDEX_H

#include <QHash>
#include <QUuid>

template <class T>
class MultiIndex {
 public:
  MultiIndex(){};
  void push_back(const T& item) {
    itemsList.push_back(item);
    uuidList.push_back(QUuid::createUuid());
    itemsMap.insert(uuidList.last(), itemsList.size() - 1);
  }

  size_t size() const noexcept { return itemsList.size(); }

  QUuid getUuid(size_t index) const { return uuidList[index]; }

  const T& operator[](size_t index) const { return itemsList[index]; }

  const T& operator[](QUuid uuid) const { return itemsMap[uuid]; }

  T& operator[](size_t index) { return itemsList[index]; }

  T& operator[](QUuid uuid) { return itemsList[itemsMap[uuid]]; }

  typename QList<T>::iterator begin() const { return itemsList.begin(); }

  typename QList<T>::iterator end() const { return itemsList.end(); }

  typename QList<T>::iterator cbegin() const noexcept {
    return itemsList.cbegin();
  }

  typename QList<T>::iterator cend() const noexcept { return itemsList.cend(); }

 private:
  QHash<QUuid, int> itemsMap;
  QList<T> itemsList;
  QList<QUuid> uuidList;
};

#endif  // MULTIINDEX_H
