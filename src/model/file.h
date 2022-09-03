#ifndef FILE_H
#define FILE_H
#include <QString>

class File {
public:
  File(QString filName);
  QString getFileName() const;

private:
  const QString _fileName;
};

#endif // FILE_H
