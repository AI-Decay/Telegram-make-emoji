#ifndef VIDEOTOGIFCONVERTER_H
#define VIDEOTOGIFCONVERTER_H
#include <QObject>
#include <vector>
class QString;

class VideoToGifConverter final : QObject {
  Q_OBJECT
 public:
  VideoToGifConverter();
  void push(QString args...);
  int convert(QString input, QString output);
 signals:
  void finishConvert(QString path);

 private:
  std::vector<QString> paths;
};

#endif  // VIDEOTOGIFCONVERTER_H
