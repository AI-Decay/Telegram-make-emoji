#ifndef STYLESHEETUTILITY_H
#define STYLESHEETUTILITY_H

#include <QFile>

QString loadStyleSheet(const QString &fileName) {
  QFile file(fileName);
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());

  return styleSheet;
}

#endif // STYLESHEETUTILITY_H
