#ifndef CONVERTITEM_H
#define CONVERTITEM_H
#include <QString>

class ConvertItem {
 public:
  ConvertItem(QString filName, int64_t durationMs);
  QString getFileName() const;
  int getProgress() const;
  void setProgress(int value);
  int64_t getDurationMs() const;
  void setDurationMs(int64_t value);
  int64_t getBeginPosMs() const;
  void setBeginPosMs(int64_t value);
  int64_t getEndPosMs() const;
  void setEndPosMs(int64_t value);

 private:
  int64_t _beginPosMs = 0;
  int64_t _endPosMs = 0;
  int64_t _durationMs = 0;
  int _progress = 0;
  const QString _fileName;
};

#endif  // CONVERTITEM_H
