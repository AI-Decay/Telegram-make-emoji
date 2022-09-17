#ifndef FILE_H
#define FILE_H
#include <QString>

class File {
 public:
  File(QString filName);
  QString getFileName() const noexcept;
  int getProgress() const noexcept;
  void setProgress(int value) noexcept;

 private:
  int progress = 0;
  const QString _fileName;
};

#endif  // FILE_H
