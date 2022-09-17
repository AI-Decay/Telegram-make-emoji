#include "videoToGifConverter.h"
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
}

namespace {

struct StreamingParams {
  StreamingParams(const StreamingParams&) = delete;
  StreamingParams(const char* output_extension,
                  const char* muxer_opt_key,
                  const char* muxer_opt_value,
                  const char* video_codec,
                  const char* codec_priv_key,
                  const char* codec_priv_value)
      : output_extension(output_extension),
        muxer_opt_key(muxer_opt_key),
        muxer_opt_value(muxer_opt_value),
        video_codec(video_codec),
        codec_priv_key(codec_priv_key),
        codec_priv_value(codec_priv_value){

        };
  const char* output_extension;
  const char* muxer_opt_key;
  const char* muxer_opt_value;
  const char* video_codec;
  const char* codec_priv_key;
  const char* codec_priv_value;
};

struct StreamingContext {
  AVFormatContext* avfc;
  AVCodec* video_avc;
  AVCodec* audio_avc;
  AVStream* video_avs;
  AVStream* audio_avs;
  AVCodecContext* video_avcc;
  AVCodecContext* audio_avcc;
  int video_index;
  int audio_index;
  char* filename;
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
    } else if (sc->avfc->streams[i]->codecpar->codec_type ==
               AVMEDIA_TYPE_AUDIO) {
      sc->audio_avs = sc->avfc->streams[i];
      sc->audio_index = i;

      if (fill_stream_info(sc->audio_avs, &sc->audio_avc, &sc->audio_avcc)) {
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

  sc->video_avc =
      const_cast<AVCodec*>(avcodec_find_encoder_by_name(sp.video_codec));
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
  if (sp.codec_priv_key && sp.codec_priv_value)
    av_opt_set(sc->video_avcc->priv_data, sp.codec_priv_key,
               sp.codec_priv_value, 0);

  sc->video_avcc->height = decoder_ctx->height;
  sc->video_avcc->width = decoder_ctx->width;
  sc->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
  if (sc->video_avc->pix_fmts)
    sc->video_avcc->pix_fmt = sc->video_avc->pix_fmts[0];
  else
    sc->video_avcc->pix_fmt = decoder_ctx->pix_fmt;

  const auto sss = 10;
  sc->video_avcc->bit_rate = 2 * 1000 * sss;
  sc->video_avcc->rc_buffer_size = 4 * 1000 * sss;
  sc->video_avcc->rc_max_rate = 2 * 1000 * sss;
  sc->video_avcc->rc_min_rate = 2.5 * 1000 * sss;

  sc->video_avcc->time_base = av_inv_q(input_framerate);
  sc->video_avs->time_base = sc->video_avcc->time_base;

  if (avcodec_open2(sc->video_avcc, sc->video_avc, nullptr) < 0) {
    qDebug() << "could not open the codec";
    return -1;
  }
  avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
  return 0;
}

int prepare_copy(AVFormatContext* avfc,
                 AVStream** avs,
                 AVCodecParameters* decoder_par) {
  *avs = avformat_new_stream(avfc, nullptr);
  avcodec_parameters_copy((*avs)->codecpar, decoder_par);
  return 0;
}

int encode_video(std::shared_ptr<StreamingContext> decoder,
                 std::shared_ptr<StreamingContext> encoder,
                 AVFrame* input_frame) {
  if (input_frame)
    input_frame->pict_type = AV_PICTURE_TYPE_NONE;

  AVPacket* output_packet = av_packet_alloc();
  if (!output_packet) {
    qDebug() << "could not allocate memory for output packet";
    return -1;
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
                    AVPacket* input_packet,
                    AVFrame* input_frame) {
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
      if (encode_video(decoder, encoder, input_frame))
        return -1;
    }
    av_frame_unref(input_frame);
  }
  return 0;
}

}  // namespace

VideoToGifConverter::VideoToGifConverter(QObject* parent) : QObject(parent) {}

void VideoToGifConverter::push(QString output, std::vector<VideoProp> input) {
  for (auto item : input) {
    std::thread(&VideoToGifConverter::convert, this, std::move(item), output)
        .detach();
  }
}

int VideoToGifConverter::convert(VideoProp input, QString output) {
  const auto outputExt = ".webm";
  StreamingParams sp{outputExt,    nullptr, "h264",
                     "libvpx-vp9", nullptr, nullptr};

  auto inputStd = input.path.toStdString();
  auto outputStd =
      (output + '/' + QUuid::createUuid().toString(QUuid::StringFormat::Id128) +
       outputExt)
          .toStdString();

  auto decoder = std::make_shared<StreamingContext>();
  decoder->filename = inputStd.data();

  auto encoder = std::make_shared<StreamingContext>();
  encoder->filename = outputStd.data();

  if (open_media(decoder->filename, &decoder->avfc))
    return -1;
  if (prepare_decoder(decoder))
    return -1;

  avformat_alloc_output_context2(&encoder->avfc, nullptr, nullptr,
                                 encoder->filename);
  if (!encoder->avfc) {
    qDebug() << "could not allocate memory for output format";
    return -1;
  }

  AVRational input_framerate =
      av_guess_frame_rate(decoder->avfc, decoder->video_avs, nullptr);
  prepare_video_encoder(encoder, decoder->video_avcc, input_framerate, sp);

  if (encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER)
    encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(encoder->avfc->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&encoder->avfc->pb, encoder->filename, AVIO_FLAG_WRITE) < 0) {
      qDebug() << "could not open the output file";
      return -1;
    }
  }

  AVDictionary* muxer_opts = nullptr;

  if (sp.muxer_opt_key && sp.muxer_opt_value) {
    av_dict_set(&muxer_opts, sp.muxer_opt_key, sp.muxer_opt_value, 0);
  }

  if (avformat_write_header(encoder->avfc, &muxer_opts) < 0) {
    qDebug() << "an error occurred when opening output file";
    return -1;
  }

  AVFrame* input_frame = av_frame_alloc();
  if (!input_frame) {
    qDebug() << "failed to allocated memory for AVFrame";
    return -1;
  }

  AVPacket* input_packet = av_packet_alloc();
  if (!input_packet) {
    qDebug() << "failed to allocated memory for AVPacket";
    return -1;
  }
  auto** streams = decoder->avfc->streams;

  const auto fps =
      static_cast<double>(
          streams[input_packet->stream_index]->avg_frame_rate.num) /
      streams[input_packet->stream_index]->avg_frame_rate.den;
  const size_t beginFrame = input.beginPosMs * fps;
  const size_t endFrame = input.endPosMs * fps;
  const auto totalFrames = endFrame - beginFrame;

  size_t count = 0;
  int progress = 0;
  while (av_read_frame(decoder->avfc, input_packet) >= 0) {
    if (streams[input_packet->stream_index]->codecpar->codec_type ==
            AVMEDIA_TYPE_VIDEO &&
        count <= endFrame) {
      if (count >= beginFrame) {
        if (transcode_video(decoder, encoder, input_packet, input_frame)) {
          return -1;
        }
      }
      av_packet_unref(input_packet);
      ++count;
      if (const auto pg = count * 100 / totalFrames; pg != progress) {
        progress = pg;
        emit updateProgress(input.uuid, progress);
      }
    }
  }

  if (encode_video(decoder, encoder, nullptr))
    return -1;

  av_write_trailer(encoder->avfc);

  if (muxer_opts != nullptr) {
    av_dict_free(&muxer_opts);
  }

  if (input_frame != nullptr) {
    av_frame_free(&input_frame);
  }

  if (input_packet != nullptr) {
    av_packet_free(&input_packet);
  }

  avformat_close_input(&decoder->avfc);

  avformat_free_context(decoder->avfc);
  avformat_free_context(encoder->avfc);

  avcodec_free_context(&decoder->video_avcc);
  avcodec_free_context(&decoder->audio_avcc);

  return 0;
}
