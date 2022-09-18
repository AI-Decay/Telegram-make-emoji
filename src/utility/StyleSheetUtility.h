#ifndef STYLESHEETUTILITY_H
#define STYLESHEETUTILITY_H

#include <QFile>
#include <QString>

inline QString loadStyleSheet(const QString& fileName) {
  QFile file(fileName);
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());

  return styleSheet;
}

template <typename T>
inline T* createWidget(QWidget* parent = nullptr,
                       const QString objectName = "") {
  auto* widget = new T(parent);
  if (!objectName.isEmpty())
    widget->setObjectName(objectName);
  return widget;
}

#endif  // STYLESHEETUTILITY_H
