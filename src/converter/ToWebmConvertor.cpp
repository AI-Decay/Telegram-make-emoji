#include "ToWebmConvertor.h"

#include <QDebug>
#include <QDir>
#include <QString>
#include <QUuid>
#include <future>
#include <memory>
extern "C" {
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
}

namespace {

constexpr auto maxDurationMs = 3000;
constexpr auto maxFileSizeByte = 100000;
constexpr auto maxWidth = 100;
constexpr auto maxHeight = 100;
#ifdef DebugAVPacket
void dumpAVPacket(AVPacket* pk) {
  if (!pk) return;

  qDebug() << "dts: " << pk->dts << "duration: " << pk->duration
           << "flags: " << pk->flags << "pos: " << pk->pos << "pts: " << pk->pts
           << "side_data_elems: " << pk->side_data_elems << "size: " << pk->size
           << "time_base.den: " << pk->time_base.den
           << "time_base.num: " << pk->time_base.num;
}
#endif

struct StreamingParams {
  std::string output_extension;
  std::string muxer_opt_key;
  std::string muxer_opt_value;
  std::string video_codec;
  std::string codec_priv_key;
  std::string codec_priv_value;
};

struct StreamingContext {
  AVFormatContext* avfc = nullptr;
  AVCodec* video_avc = nullptr;
  AVStream* video_avs = nullptr;
  AVCodecContext* video_avcc = nullptr;
  int video_index = 0;
  std::string filename;
};

struct StreamingContextDeleter {
  void operator()(StreamingContext* context) {
    if (context) {
      auto* avfc = &context->avfc;
      auto* avcc = &context->video_avcc;
      if (avfc) avformat_close_input(avfc);
      if (avcc) avcodec_free_context(avcc);
      if (context->avfc) avformat_free_context(context->avfc);
    }
  }
};

struct AVFrameDeleter {
  void operator()(AVFrame* frame) {
    if (frame) av_frame_free(&frame);
  }
};

struct AVPacketDeleter {
  void operator()(AVPacket* packet) {
    if (packet) av_packet_free(&packet);
  }
};

struct SwsContextDeleter {
  void operator()(SwsContext* context) {
    if (context) sws_freeContext(context);
  }
};

struct AVDictionaryDeleter {
  void operator()(AVDictionary* dictionary) {
    if (dictionary) av_dict_free(&dictionary);
  }
};

QString getResponse(int code) {
  char error[AV_ERROR_MAX_STRING_SIZE];
  av_make_error_string(error, AV_ERROR_MAX_STRING_SIZE, code);
  return QString(error);
}

int fill_stream_info(AVStream* avs, AVCodec** avc, AVCodecContext** avcc) {
  *avc = const_cast<AVCodec*>(avcodec_find_decoder(avs->codecpar->codec_id));
  if (!*avc) {
    qDebug() << "failed to find the codec";
    return -1;
  }

  *avcc = avcodec_alloc_context3(*avc);
  if (!*avcc) {
    qDebug() << "failed to alloc memory for codec context";
    return -1;
  }

  if (avcodec_parameters_to_context(*avcc, avs->codecpar) < 0) {
    qDebug() << "failed to fill codec context";
    return -1;
  }

  if (avcodec_open2(*avcc, *avc, nullptr) < 0) {
    qDebug() << "failed to open codec";
    return -1;
  }
  return 0;
}

int open_media(const char* in_filename, AVFormatContext** avfc) {
  *avfc = avformat_alloc_context();
  if (!*avfc) {
    qDebug() << "failed to alloc memory for format";
    return -1;
  }

  if (avformat_open_input(avfc, in_filename, nullptr, nullptr) != 0) {
    qDebug() << "failed to open input file " << in_filename;
    return -1;
  }

  if (avformat_find_stream_info(*avfc, nullptr) < 0) {
    qDebug() << "failed to get stream info";
    return -1;
  }

  // av_dump_format(*avfc, 0, in_filename, 0);
  return 0;
}

int prepare_decoder(std::shared_ptr<StreamingContext> sc) {
  for (int i = 0; i < sc->avfc->nb_streams; i++) {
    if (sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      sc->video_avs = sc->avfc->streams[i];
      sc->video_index = i;

      if (fill_stream_info(sc->video_avs, &sc->video_avc, &sc->video_avcc)) {
        return -1;
      }
    } else {
      qDebug() << "skipping streams other than audio and video";
    }
  }

  return 0;
}

int prepare_video_encoder(std::shared_ptr<StreamingContext> sc,
                          AVCodecContext* decoder_ctx,
                          AVRational input_framerate,
                          const StreamingParams& sp) {
  sc->video_avs = avformat_new_stream(sc->avfc, nullptr);

  sc->video_avc = const_cast<AVCodec*>(
      avcodec_find_encoder_by_name(sp.video_codec.c_str()));
  if (!sc->video_avc) {
    qDebug() << "could not find the proper codec";
    return -1;
  }

  sc->video_avcc = avcodec_alloc_context3(sc->video_avc);
  if (!sc->video_avcc) {
    qDebug() << "could not allocated memory for codec context";
    return -1;
  }

  av_opt_set(sc->video_avcc->priv_data, "preset", "fast", 0);
  if (!sp.codec_priv_key.empty() && !sp.codec_priv_value.empty())
    av_opt_set(sc->video_avcc->priv_data, sp.codec_priv_key.c_str(),
               sp.codec_priv_value.c_str(), 0);

  sc->video_avcc->height = maxHeight;
  sc->video_avcc->width = maxWidth;
  sc->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
  if (sc->video_avc->pix_fmts)
    sc->video_avcc->pix_fmt = sc->video_avc->pix_fmts[0];
  else
    sc->video_avcc->pix_fmt = decoder_ctx->pix_fmt;

  constexpr int64_t maxBitrate = maxFileSizeByte / (maxDurationMs / 1000.0) - 1;

  sc->video_avcc->bit_rate = maxBitrate;
  sc->video_avcc->rc_buffer_size = maxFileSizeByte;
  sc->video_avcc->rc_max_rate = maxBitrate;
  sc->video_avcc->rc_min_rate = maxBitrate;
  sc->video_avcc->time_base = av_inv_q(input_framerate);
  sc->video_avs->time_base = sc->video_avcc->time_base;

  if (avcodec_open2(sc->video_avcc, sc->video_avc, nullptr) < 0) {
    qDebug() << "could not open the codec";
    return -1;
  }
  avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);

  return 0;
}

int encode_video(std::shared_ptr<StreamingContext> decoder,
                 std::shared_ptr<StreamingContext> encoder,
                 AVFrame* input_frame, SwsContext* scale) {
  if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

  AVPacket* output_packet = av_packet_alloc();
  if (!output_packet) {
    qDebug() << "could not allocate memory for output packet";
    return -1;
  }

  if (input_frame && scale) {
    sws_scale(scale, input_frame->data, input_frame->linesize, 0,
              decoder->video_avcc->height, input_frame->data,
              input_frame->linesize);
  }

  int response = avcodec_send_frame(encoder->video_avcc, input_frame);

  while (response >= 0) {
    response = avcodec_receive_packet(encoder->video_avcc, output_packet);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      qDebug() << "Error while receiving packet from encoder: "
               << getResponse(response);
      return -1;
    }

    output_packet->stream_index = decoder->video_index;
    output_packet->duration = encoder->video_avs->time_base.den /
                              encoder->video_avs->time_base.num /
                              decoder->video_avs->avg_frame_rate.num *
                              decoder->video_avs->avg_frame_rate.den;

    av_packet_rescale_ts(output_packet, decoder->video_avs->time_base,
                         encoder->video_avs->time_base);

    response = av_interleaved_write_frame(encoder->avfc, output_packet);
    if (response != 0) {
      qDebug() << "Error while receiving packet from decoder: " << response
               << getResponse(response);
      return -1;
    }
  }
  av_packet_unref(output_packet);
  av_packet_free(&output_packet);
  return 0;
}

int transcode_video(std::shared_ptr<StreamingContext> decoder,
                    std::shared_ptr<StreamingContext> encoder,
                    AVPacket* input_packet, AVFrame* input_frame,
                    SwsContext* scale) {
  int response = avcodec_send_packet(decoder->video_avcc, input_packet);
  if (response < 0) {
    qDebug() << "Error while sending packet to decoder: "
             << getResponse(response);
    return response;
  }

  while (response >= 0) {
    response = avcodec_receive_frame(decoder->video_avcc, input_frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      qDebug() << "Error while receiving frame from decoder: "
               << getResponse(response);
      return response;
    }

    if (response >= 0) {
      if (encode_video(decoder, encoder, input_frame, scale)) return -1;
    }
    av_frame_unref(input_frame);
  }

  return 0;
}

}  // namespace

ToWebmConvertor::ToWebmConvertor(QObject* parent) : QObject(parent) {}

void ToWebmConvertor::push(QString output, std::vector<VideoProp> input) {
  for (auto item : input) {
    std::thread(&ToWebmConvertor::convert, this, std::move(item), output)
        .detach();
  }
}

int ToWebmConvertor::convert(VideoProp input, QString output) {
  if (input.beginPosMs >= input.endPosMs) {
    qDebug() << "incorrect pos, begin >= end";
    return -1;
  }

  StreamingParams sp;
  sp.output_extension = ".webm";
  sp.video_codec = "libvpx-vp9";

  auto inputStd = input.path.toStdString();
  auto outputStd =
      (output + '/' + QUuid::createUuid().toString(QUuid::StringFormat::Id128))
          .toStdString() +
      sp.output_extension;

  auto decoder = std::shared_ptr<StreamingContext>(new StreamingContext,
                                                   StreamingContextDeleter{});
  auto encoder = std::shared_ptr<StreamingContext>(new StreamingContext,
                                                   StreamingContextDeleter{});

  encoder->filename = std::move(outputStd);
  decoder->filename = std::move(inputStd);

  if (open_media(decoder->filename.c_str(), &decoder->avfc)) return -1;
  if (prepare_decoder(decoder)) return -1;

  avformat_alloc_output_context2(&encoder->avfc, nullptr, nullptr,
                                 encoder->filename.c_str());
  if (!encoder->avfc) {
    qDebug() << "could not allocate memory for output format";
    return -1;
  }

  AVRational input_framerate =
      av_guess_frame_rate(decoder->avfc, decoder->video_avs, nullptr);
  prepare_video_encoder(encoder, decoder->video_avcc, input_framerate, sp);

  if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
    encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  // INVESTIGATE
  if (!(encoder->avfc->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&encoder->avfc->pb, encoder->filename.c_str(),
                  AVIO_FLAG_WRITE) < 0) {
      qDebug() << "could not open the output file";
      return -1;
    }
  }

  AVDictionary* muxer_opts = nullptr;

  if (!sp.muxer_opt_key.empty() && !sp.muxer_opt_value.empty()) {
    av_dict_set(&muxer_opts, sp.muxer_opt_key.c_str(),
                sp.muxer_opt_value.c_str(), 0);
  }

  if (avformat_write_header(encoder->avfc, &muxer_opts) < 0) {
    qDebug() << "an error occurred when opening output file";
    return -1;
  }

  auto inputFrame = std::unique_ptr<AVFrame, AVFrameDeleter>(av_frame_alloc());
  if (!inputFrame) {
    qDebug() << "failed to allocated memory for AVFrame";
    return -1;
  }

  auto inputPacket =
      std::unique_ptr<AVPacket, AVPacketDeleter>(av_packet_alloc());
  if (!inputPacket) {
    qDebug() << "failed to allocated memory for AVPacket";
    return -1;
  }

  auto scaleContext =
      std::unique_ptr<SwsContext, SwsContextDeleter>(sws_getContext(
          decoder->video_avcc->width, decoder->video_avcc->height,
          decoder->video_avcc->pix_fmt, maxWidth, maxHeight,
          encoder->video_avcc->pix_fmt, SWS_SPLINE, nullptr, nullptr, nullptr));

  auto** streams = decoder->avfc->streams;

  const auto fps = static_cast<double>(
                       streams[inputPacket->stream_index]->avg_frame_rate.num) /
                   streams[inputPacket->stream_index]->avg_frame_rate.den;
  const size_t beginFrame = input.beginPosMs * fps / 1000;
  const size_t endFrame = input.endPosMs * fps / 1000;
  const auto totalFrames = endFrame - beginFrame;

  size_t count = 0;
  int progress = 0;

  int64_t startTime =
      av_rescale_q(input.beginPosMs * AV_TIME_BASE / 1000, {1, AV_TIME_BASE},
                   decoder->video_avs->time_base);

  av_seek_frame(decoder->avfc, inputPacket->stream_index, startTime, 0);
  avcodec_flush_buffers(decoder->video_avcc);

  // Note: Important to calculate manually and set pts, duration, dts because we
  // did av_seek_frame
  const int64_t frameDuration =
      decoder->video_avs->time_base.den /
      (input_framerate.num / static_cast<double>(input_framerate.den));

  while (count < totalFrames &&
         av_read_frame(decoder->avfc, inputPacket.get()) >= 0) {
    if (streams[inputPacket->stream_index]->codecpar->codec_type ==
        AVMEDIA_TYPE_VIDEO) {
      const int64_t frameTime = count * frameDuration;
      inputPacket->pts = frameTime / decoder->video_avs->time_base.num;
      inputPacket->duration = frameDuration;
      inputPacket->dts = inputPacket->pts;

      if (transcode_video(decoder, encoder, inputPacket.get(), inputFrame.get(),
                          scaleContext.get())) {
        return -1;
      }
      if (const auto pg = (count * 100) / totalFrames; pg != progress) {
        progress = pg;
        emit updateProgress(input.uuid, progress);
      }
      ++count;
    }
    av_packet_unref(inputPacket.get());
  }

  if (encode_video(decoder, encoder, nullptr, nullptr)) {
    return -1;
  }

  emit updateProgress(input.uuid, 100);

  av_write_trailer(encoder->avfc);

  return 0;
}
