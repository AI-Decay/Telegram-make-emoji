#include "file.h"

File::File(QString fileName) : _fileName(fileName) {}

QString File::getFileName() const noexcept {
  return _fileName;
}

int File::getProgress() const noexcept {
  return progress;
}

void File::setProgress(int value) noexcept {
  progress = value;
}
