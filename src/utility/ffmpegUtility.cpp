#include "ffmpegUtility.h"
#include <QDebug>
extern "C" {
#include <libavformat/avformat.h>
}

namespace {}  // namespace

int64_t getVideoDurationMs(QString fileName) {
  auto fileNameStd = fileName.toStdString();

  AVFormatContext* context = nullptr;
  if (avformat_open_input(&context, fileNameStd.c_str(), nullptr, nullptr) !=
      0) {
    qDebug() << "failed to open input file " << fileName;
    return -1;
  }

  avformat_find_stream_info(context, nullptr);

  const auto duration = context->duration;

  avformat_close_input(&context);
  avformat_free_context(context);

  return duration / static_cast<double>(AV_TIME_BASE) * 1000;
}
