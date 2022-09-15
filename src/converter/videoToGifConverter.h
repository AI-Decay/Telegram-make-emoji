#ifndef VIDEOTOGIFCONVERTER_H
#define VIDEOTOGIFCONVERTER_H
#include <QObject>
#include <QUuid>
#include <tuple>
#include <vector>
class QString;

class QStringList;

struct VideoProp {
  QString path;
  int beginPosMs = 0;
  int endPosMs = 0;
};

class VideoToGifConverter final : QObject {
  Q_OBJECT
 public:
  VideoToGifConverter(QObject* parent = nullptr);
  void push(QString output, std::vector<VideoProp> input);
 signals:
  void finishConvert(QString path);
  void updateProgress(QUuid taskId, int progress);

 private:
  int convert(VideoProp input, QString output);
  std::vector<QString> paths;
};

#endif  // VIDEOTOGIFCONVERTER_H
