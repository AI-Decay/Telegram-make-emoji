#include "file.h"

File::File(QString fileName, int64_t durationMs)
    : _fileName(fileName), _durationMs(durationMs) {}

QString File::getFileName() const {
  return _fileName;
}

int File::getProgress() const {
  return _progress;
}

void File::setProgress(int value) {
  _progress = value;
}

int64_t File::getDurationMs() const {
  return _durationMs;
}

void File::setDurationMs(int64_t value) {
  _durationMs = value;
}

int64_t File::getBeginPosMs() const {
  return _beginPosMs;
}

void File::setBeginPosMs(int64_t value) {
  _beginPosMs = value;
}

int64_t File::getEndPosMs() const {
  return _endPosMs;
}

void File::setEndPosMs(int64_t value) {
  _endPosMs = value;
}
