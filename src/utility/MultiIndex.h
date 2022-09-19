#ifndef MULTIINDEX_H
#define MULTIINDEX_H

#include <QHash>
#include <QList>
#include <QUuid>
#include <mutex>

template <class T>
class MultiIndex {
  using Iterator = typename QList<T>::iterator;
  using ConstIterator = typename QList<T>::const_iterator;

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

  int getKey(QUuid uuid) { return itemsMap[uuid]; }

  Iterator begin() const { return itemsList.begin(); }
  Iterator end() const { return itemsList.end(); }

  ConstIterator cbegin() const noexcept { return itemsList.cbegin(); }
  ConstIterator cend() const noexcept { return itemsList.cend(); }

  void erace(size_t index) {
    itemsMap.erase(itemsMap.find(uuidList[index]));
    uuidList.erase(std::next(uuidList.begin(), index));
    itemsList.erase(std::next(itemsList.begin(), index));
  };

 private:
  QHash<QUuid, int> itemsMap;
  QList<T> itemsList;
  QList<QUuid> uuidList;
};

#endif  // MULTIINDEX_H
