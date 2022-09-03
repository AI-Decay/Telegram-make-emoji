#include "file.h"

File::File(QString fileName) : _fileName(fileName) {}

QString File::getFileName() const { return _fileName; }
