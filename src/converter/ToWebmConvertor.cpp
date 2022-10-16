#include "ToWebmConvertor.h"

#include <QDebug>
#include <QDir>
#include <QString>
#include <QUuid>
#include <exception>
#include <future>
#include <memory>
extern "C" {
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
}

namespace {

constexpr auto MaxDurationMs = 3000;
constexpr auto MaxFileSizeByte = 100000;
constexpr auto Width = 100;
constexpr auto Height = 100;
constexpr auto OutputExtension = ".webm";
constexpr auto VideoCodec = "libvpx-vp9";

constexpr auto OpenOutputFileException = "Failed to opening output file";
constexpr auto WriteHeaderFileException = "Failed to write header output file";
constexpr auto AllocateAVFrameException =
    "Failed to allocated memory for AVFrame";
constexpr auto AllocateAVPacketException =
    "Failed to allocated memory for AVPacket";
constexpr auto AllocateOutputFormatException =
    "Failed to allocated memory for output format";
constexpr auto AllocateAVFormatContextException =
    "Failed to allocated memory for AVFormat context";
constexpr auto AllocateCodecContextException =
    "Failed to allocated memory for codec context";
constexpr auto FindCodecException = "Failed to find the proper codec";
constexpr auto OpenCodecException = "Failed to open the codec";
constexpr auto FillCodecContextException = "Failed to fill codec context";
constexpr auto OpenInputFileException = "Failed to open input file";
constexpr auto FindStreamInfoException = "Failed to get find info";
constexpr auto ReceivingPacketEncoderException =
    "Failed to receiving packet from encoder";
constexpr auto ReceivingPacketDecoderException =
    "Failed to receiving packet from decoder";
constexpr auto SendongPacketDecoderException =
    "Error while sending packet to decoder";
constexpr auto ReceivingFrameDecoderException =
    "Error while receiving frame from decoder";

struct StreamingParams {
  std::string outputExtension;
  std::string muxerOptKey;
  std::string muxerOptValue;
  std::string videoCodec;
  std::string codecPrivKey;
  std::string codecPrivValue;
};

struct StreamingContext {
  AVFormatContext* formatContext = nullptr;
  AVCodec* codec = nullptr;
  AVStream* stream = nullptr;
  AVCodecContext* codecContext = nullptr;
  int video_index = 0;
  std::string filename;
};

struct StreamingContextDeleter {
  void operator()(StreamingContext* context) {
    if (context) {
      auto* avfc = &context->formatContext;
      auto* avcc = &context->codecContext;
      if (avfc)
        avformat_close_input(avfc);
      if (avcc)
        avcodec_free_context(avcc);
      if (context->formatContext)
        avformat_free_context(context->formatContext);
    }
  }
};

struct AVFrameDeleter {
  void operator()(AVFrame* frame) {
    if (frame)
      av_frame_free(&frame);
  }
};

struct AVPacketDeleter {
  void operator()(AVPacket* packet) {
    if (packet)
      av_packet_free(&packet);
  }
};

struct SwsContextDeleter {
  void operator()(SwsContext* context) {
    if (context)
      sws_freeContext(context);
  }
};

struct AVDictionaryDeleter {
  void operator()(AVDictionary* dictionary) {
    if (dictionary)
      av_dict_free(&dictionary);
  }
};

QString getResponse(int code) {
  char error[AV_ERROR_MAX_STRING_SIZE];
  av_make_error_string(error, AV_ERROR_MAX_STRING_SIZE, code);
  return QString(error);
}

using ContextPtr = std::unique_ptr<StreamingContext, StreamingContextDeleter>;

class VideoTranscoder : public QObject {
  Q_OBJECT
  using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
  using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
  using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

 signals:
  void updateProgress(QUuid taskId, int progress);

 public:
  VideoTranscoder(ContextPtr encoder, ContextPtr decoder)
      : _encoder(std::move(encoder)), _decoder(std::move(decoder)) {}

  void process(const VideoProp& input) {
    AVDictionary* muxer_opts = nullptr;

    /*if (!sp.muxer_opt_key.empty() && !sp.muxer_opt_value.empty()) {
      av_dict_set(&muxer_opts, sp.muxer_opt_key.c_str(),
                  sp.muxer_opt_value.c_str(), 0);
    }*/

    open_media();
    prepare_decoder();
    prepare_video_encoder();

    if (avformat_write_header(_encoder->formatContext, &muxer_opts) < 0) {
      throw std::exception(WriteHeaderFileException);
    }

    auto inputFrame = AVFramePtr(av_frame_alloc());
    if (!inputFrame) {
      throw std::exception(AllocateAVFrameException);
    }

    auto inputPacket = AVPacketPtr(av_packet_alloc());
    if (!inputPacket) {
      throw std::exception(AllocateAVPacketException);
    }

    auto scaleContext = SwsContextPtr(sws_getContext(
        _decoder->codecContext->width, _decoder->codecContext->height,
        _decoder->codecContext->pix_fmt, Width, Height,
        _encoder->codecContext->pix_fmt, SWS_SPLINE, nullptr, nullptr,
        nullptr));

    auto** streams = _decoder->formatContext->streams;

    _fps = streams[inputPacket->stream_index]->avg_frame_rate;

    //   static_cast<double>(
    // //     streams[inputPacket->stream_index]->avg_frame_rate.num) /
    // streams[inputPacket->stream_index]->avg_frame_rate.den;

    const int64_t beginFrame = input.beginPosMs * _fps.num / (1000 * _fps.den);
    const int64_t endFrame = input.endPosMs * _fps.num / (1000 * _fps.den);
    const int64_t totalFrames = endFrame - beginFrame;

    int64_t count = 0;
    int progress = 0;

    int64_t startTime =
        av_rescale_q(input.beginPosMs * AV_TIME_BASE / 1000, {1, AV_TIME_BASE},
                     _decoder->stream->time_base);

    if (startTime > 0) {
      av_seek_frame(_decoder->formatContext, inputPacket->stream_index,
                    startTime, 0);
      avcodec_flush_buffers(_decoder->codecContext);
    }

    while (count < totalFrames &&
           av_read_frame(_decoder->formatContext, inputPacket.get()) >= 0) {
      const auto codec_type =
          streams[inputPacket->stream_index]->codecpar->codec_type;

      if (codec_type == AVMEDIA_TYPE_VIDEO) {
        transcode_video(inputPacket.get(), inputFrame.get(),
                        scaleContext.get());

        if (const auto pg = (count * 100) / totalFrames; pg != progress) {
          progress = pg;
          emit updateProgress(input.uuid, progress);
        }

        ++count;
      }
      av_packet_unref(inputPacket.get());
    }

    encode_video(nullptr, nullptr);
    av_write_trailer(_encoder->formatContext);

    emit updateProgress(input.uuid, 100);
  }

 private:
  void prepare_decoder() {
    for (int i = 0; i < _decoder->formatContext->nb_streams; i++) {
      if (_decoder->formatContext->streams[i]->codecpar->codec_type ==
          AVMEDIA_TYPE_VIDEO) {
        _decoder->stream = _decoder->formatContext->streams[i];
        _decoder->video_index = i;

        fill_stream_info(_decoder->stream, &_decoder->codec,
                         &_decoder->codecContext);
      }
    }
  }

  void prepare_video_encoder() {
    avformat_alloc_output_context2(&_encoder->formatContext, nullptr, nullptr,
                                   _encoder->filename.c_str());
    if (!_encoder->formatContext) {
      throw std::exception(AllocateOutputFormatException);
    }

    AVRational input_framerate =
        av_guess_frame_rate(_decoder->formatContext, _decoder->stream, nullptr);

    _encoder->stream = avformat_new_stream(_encoder->formatContext, nullptr);

    _encoder->codec =
        const_cast<AVCodec*>(avcodec_find_encoder_by_name(VideoCodec));
    if (!_encoder->codec) {
      throw std::exception(FindCodecException);
    }

    _encoder->codecContext = avcodec_alloc_context3(_encoder->codec);
    if (!_encoder->codecContext) {
      throw std::exception(AllocateCodecContextException);
    }

    if (_encoder->codecContext->codec_id == AV_CODEC_ID_H264)
      av_opt_set(_encoder->codecContext->priv_data, "preset", "slow", 0);
    else
      av_opt_set(_encoder->codecContext->priv_data, "preset", "fast", 0);

    _encoder->codecContext->height = Height;
    _encoder->codecContext->width = Width;
    _encoder->codecContext->sample_aspect_ratio = {Width, Height};
    if (_encoder->codec->pix_fmts)
      _encoder->codecContext->pix_fmt = _encoder->codec->pix_fmts[0];
    else
      _encoder->codecContext->pix_fmt = _decoder->codecContext->pix_fmt;

    constexpr int64_t maxBitrate =
        MaxFileSizeByte / (MaxDurationMs / 1000.0) - 1;

    _encoder->codecContext->max_b_frames = _decoder->codecContext->max_b_frames;
    _encoder->codecContext->bit_rate =
        std::min(maxBitrate, _decoder->codecContext->bit_rate);
    _encoder->codecContext->rc_buffer_size = MaxFileSizeByte;
    _encoder->codecContext->rc_max_rate =
        std::min(maxBitrate, _decoder->codecContext->rc_max_rate);
    _encoder->codecContext->rc_min_rate =
        std::min(maxBitrate, _decoder->codecContext->rc_min_rate);
    _encoder->codecContext->time_base = av_inv_q(input_framerate);
    _encoder->stream->time_base = _encoder->codecContext->time_base;

    if (avcodec_open2(_encoder->codecContext, _encoder->codec, nullptr) < 0) {
      throw std::exception(OpenCodecException);
    }
    avcodec_parameters_from_context(_encoder->stream->codecpar,
                                    _encoder->codecContext);

    if (_encoder->formatContext->oformat->flags & AVFMT_GLOBALHEADER)
      _encoder->formatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (!(_encoder->formatContext->oformat->flags & AVFMT_NOFILE)) {
      if (avio_open(&_encoder->formatContext->pb, _encoder->filename.c_str(),
                    AVIO_FLAG_WRITE) < 0) {
        throw std::exception(OpenOutputFileException);
      }
    }
  }

  void fill_stream_info(AVStream* avs, AVCodec** avc, AVCodecContext** avcc) {
    *avc = const_cast<AVCodec*>(avcodec_find_decoder(avs->codecpar->codec_id));
    if (!*avc) {
      throw std::exception(FindCodecException);
    }

    *avcc = avcodec_alloc_context3(*avc);
    if (!*avcc) {
      throw std::exception(AllocateCodecContextException);
    }

    if (avcodec_parameters_to_context(*avcc, avs->codecpar) < 0) {
      throw std::exception(FillCodecContextException);
    }

    if (avcodec_open2(*avcc, *avc, nullptr) < 0) {
      throw std::exception(OpenCodecException);
    }
  }

  void open_media() {
    auto** avfc = &_decoder->formatContext;
    *avfc = avformat_alloc_context();
    if (!*avfc) {
      throw std::exception(AllocateAVFormatContextException);
    }

    if (avformat_open_input(avfc, _decoder->filename.c_str(), nullptr,
                            nullptr) != 0) {
      throw std::exception(OpenInputFileException);
    }

    if (avformat_find_stream_info(*avfc, nullptr) < 0) {
      throw std::exception(FindStreamInfoException);
    }
  }

  void encode_video(AVFrame* inputFrame, SwsContext* scale) {
    if (inputFrame) {
      inputFrame->pict_type = AV_PICTURE_TYPE_NONE;
    }

    AVPacket* output_packet = av_packet_alloc();
    if (!output_packet) {
      throw std::exception(AllocateAVPacketException);
    }

    AVFramePtr scaledFrame = nullptr;

    if (inputFrame && scale) {
      scaledFrame = AVFramePtr(av_frame_alloc());
      scaledFrame->format = inputFrame->format;
      scaledFrame->width = Width;
      scaledFrame->height = Height;
      // scaledFrame->channels = inputFrame->channels;
      // scaledFrame->channel_layout = inputFrame->channel_layout;
      scaledFrame->nb_samples = inputFrame->nb_samples;
      av_frame_get_buffer(scaledFrame.get(), 32);
      av_frame_copy_props(scaledFrame.get(), inputFrame);

      const auto pixFormat = static_cast<AVPixelFormat>(inputFrame->format);
      auto* out_buffer = static_cast<std::uint8_t*>(
          av_malloc(av_image_get_buffer_size(pixFormat, Width, Height, 32)));
      av_image_fill_arrays(scaledFrame->data, scaledFrame->linesize, out_buffer,
                           pixFormat, Width, Height, 32);

      sws_scale(scale, inputFrame->data, inputFrame->linesize, 0,
                _decoder->codecContext->height, scaledFrame->data,
                scaledFrame->linesize);
    }

    int response =
        avcodec_send_frame(_encoder->codecContext, scaledFrame.get());
    while (response >= 0) {
      response = avcodec_receive_packet(_encoder->codecContext, output_packet);
      if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        break;
      } else if (response < 0) {
        throw std::exception(ReceivingPacketDecoderException);
      }

      const auto frameDuration =
          _decoder->stream->time_base.den * _fps.den / _fps.num;
      const int64_t frameTime = current_frame * frameDuration;
      output_packet->pts = frameTime / _decoder->stream->time_base.num;
      output_packet->duration = frameDuration;
      output_packet->dts = output_packet->pts;
      output_packet->stream_index = _decoder->stream->index;
      av_packet_rescale_ts(output_packet, _decoder->stream->time_base,
                           _encoder->stream->time_base);

      ++current_frame;

      response =
          av_interleaved_write_frame(_encoder->formatContext, output_packet);
      if (response != 0) {
        throw std::exception(ReceivingPacketDecoderException);
      }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
  }

  void transcode_video(AVPacket* input_packet,
                       AVFrame* input_frame,
                       SwsContext* scale) {
    int response = avcodec_send_packet(_decoder->codecContext, input_packet);
    if (response < 0) {
      throw std::exception(SendongPacketDecoderException);
    }

    while (response >= 0) {
      response = avcodec_receive_frame(_decoder->codecContext, input_frame);
      if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        break;
      } else if (response < 0) {
        throw std::exception(ReceivingFrameDecoderException);
      }

      if (response >= 0) {
        encode_video(input_frame, scale);
      }
      av_frame_unref(input_frame);
    }
  }

 private:
  ContextPtr _encoder = nullptr;
  ContextPtr _decoder = nullptr;
  size_t current_frame = 0;
  AVRational _fps = {};
};

}  // namespace

ToWebmConvertor::ToWebmConvertor(QObject* parent) : QObject(parent) {}

void ToWebmConvertor::push(QString output, std::vector<VideoProp> input) {
  for (auto item : input) {
    std::thread(&ToWebmConvertor::convert, this, std::move(item), output)
        .detach();
  }
}

int ToWebmConvertor::convert(VideoProp input, QString output) {
  auto inputStd = input.path.toStdString();
  auto outputStd =
      (output + '/' + QUuid::createUuid().toString(QUuid::StringFormat::Id128))
          .toStdString() +
      OutputExtension;

  auto decoder = ContextPtr(new StreamingContext);
  auto encoder = ContextPtr(new StreamingContext);

  encoder->filename = std::move(outputStd);
  decoder->filename = std::move(inputStd);

  VideoTranscoder transcoder(std::move(encoder), std::move(decoder));
  QObject::connect(&transcoder, &VideoTranscoder::updateProgress, this,
                   &ToWebmConvertor::updateProgress);

  try {
    transcoder.process(input);
  } catch (std::exception& ex) {
    qDebug() << ex.what();
    emit updateProgress(input.uuid, -1);
    return -1;
  }

  return 0;
}
#include "ToWebmConvertor.moc"
