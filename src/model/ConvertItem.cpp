#include "ConvertItem.h"

namespace {
const auto DefaultEndPos = 3000;
}

ConvertItem::ConvertItem(QString fileName, int64_t durationMs)
    : _fileName(fileName), _durationMs(durationMs) {
  _endPosMs = std::min(static_cast<int64_t>(DefaultEndPos), durationMs);
}

QString ConvertItem::getFileName() const {
  return _fileName;
}

int ConvertItem::getProgress() const {
  return _progress;
}

void ConvertItem::setProgress(int value) {
  _progress = value;
}

int64_t ConvertItem::getDurationMs() const {
  return _durationMs;
}

void ConvertItem::setDurationMs(int64_t value) {
  _durationMs = value;
}

int64_t ConvertItem::getBeginPosMs() const {
  return _beginPosMs;
}

void ConvertItem::setBeginPosMs(int64_t value) {
  _beginPosMs = value;
}

int64_t ConvertItem::getEndPosMs() const {
  return _endPosMs;
}

void ConvertItem::setEndPosMs(int64_t value) {
  _endPosMs = value;
}
