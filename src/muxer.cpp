/**
 * Author: Hidden Track
 * Date: 2020/06/10
 * Time: 14:24
 *
 * aac转码hls/dash
 *
 */
#include "muxer.h"
#include <chrono>

Muxer::Muxer() {
  frame = av_frame_alloc();
  in_sample_rate = 48000;
  in_channels = 2;
  in_channel_layout = av_get_default_channel_layout(in_channels);
  in_sample_fmt = AV_SAMPLE_FMT_FLTP;

  pSwrCtx = nullptr;
  aacbsfc = nullptr;

  packet.data = NULL;
  packet.size = 0;
}

Muxer::~Muxer() {
  I_LOG("Muxer uninit...");
   //av_write_trailer(ofmt_ctx);
   //av_free_packet(&packet);
   //if(frame) av_frame_free(&frame);
   av_bitstream_filter_close(aacbsfc);

   if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
    avio_closep(&ofmt_ctx->pb);
   avformat_free_context(ofmt_ctx);
}

void Muxer::initSwr(int audio_index) {
  if (NULL == pSwrCtx) {
    pSwrCtx = swr_alloc();
  }
#if LIBSWRESAMPLE_VERSION_MINOR >= 17  // 根据版本不同，选用适当函数
  av_opt_set_int(pSwrCtx, "ich", in_channels, 0);
  av_opt_set_int(pSwrCtx, "och",
                 ofmt_ctx->streams[audio_index]->codec->channels, 0);
  av_opt_set_int(pSwrCtx, "in_sample_rate", in_sample_rate, 0);
  av_opt_set_int(pSwrCtx, "out_sample_rate",
                 ofmt_ctx->streams[audio_index]->codec->sample_rate, 0);
  av_opt_set_sample_fmt(pSwrCtx, "in_sample_fmt", in_sample_fmt, 0);
  av_opt_set_sample_fmt(pSwrCtx, "out_sample_fmt",
                        ofmt_ctx->streams[audio_index]->codec->sample_fmt, 0);

#else
  pSwrCtx = swr_alloc_set_opts(
      NULL, ofmt_ctx->streams[audio_index]->codec->channel_layout,
      ofmt_ctx->streams[audio_index]->codec->sample_fmt,
      ofmt_ctx->streams[audio_index]->codec->sample_rate, in_channel_layout,
      in_sample_fmt, in_sample_rate, 0, NULL);
#endif

  // I_LOG("init swr ctx, channel_layout: {}, sample_fmt: {}, sample rate: {}",
  //       ofmt_ctx->streams[audio_index]->codec->channel_layout,
  //       av_get_sample_fmt_name(ofmt_ctx->streams[audio_index]->codec->sample_fmt),
  //       ofmt_ctx->streams[audio_index]->codec->sample_rate);
  // I_LOG("in_fmt, channel_layout: {}, sample_fmt: {}, sample rate: {}",
  //      ifmt_ctx->streams[audio_index]->codec->channel_layout,
  //      av_get_sample_fmt_name(ifmt_ctx->streams[audio_index]->codec->sample_fmt),
  //      ifmt_ctx->streams[audio_index]->codec->sample_rate);
  I_LOG("xxx, default channel_layout: {}, channels: {}, channel index: {}",
        av_get_default_channel_layout(2), av_get_channel_layout_nb_channels(3),
        av_get_channel_layout_channel_index(3, 2));

  swr_init(pSwrCtx);
}

void Muxer::setup_array(uint8_t *out[32], AVFrame *in_frame,
                        enum AVSampleFormat format, int samples) {
  if (av_sample_fmt_is_planar(format)) {
    int i;
    int plane_size = av_get_bytes_per_sample((format)) * samples;
    // format &= 0xFF;

    //从decoder出来的frame中的data数据不是连续分布的，所以不能这样写：
    in_frame->data[0] + i *plane_size;
    for (i = 0; i < in_frame->channels; i++) {
      out[i] = in_frame->data[i];
    }
  } else {
    out[0] = in_frame->data[0];
  }
}

int Muxer::TransSample(AVFrame *in_frame, AVFrame *out_frame, int audio_index) {
  int ret;
  int max_dst_nb_samples = 4096;
  // int64_t dst_nb_samples;
  int64_t src_nb_samples = in_frame->nb_samples;
  out_frame->pts = in_frame->pts;
  uint8_t *paudiobuf;
  int decode_size, input_size, len;
  if (pSwrCtx != NULL) {
    out_frame->nb_samples = av_rescale_rnd(
        swr_get_delay(pSwrCtx,
                      ofmt_ctx->streams[audio_index]->codec->sample_rate) +
            src_nb_samples,
        ofmt_ctx->streams[audio_index]->codec->sample_rate, in_sample_rate,
        AV_ROUND_UP);

    ret = av_samples_alloc(
        out_frame->data, &out_frame->linesize[0],
        ofmt_ctx->streams[audio_index]->codec->channels, out_frame->nb_samples,
        ofmt_ctx->streams[audio_index]->codec->sample_fmt, 0);

    if (ret < 0) {
      av_log(NULL, AV_LOG_WARNING,
             "[%s.%d %s() Could not allocate samples Buffer\n", __FILE__,
             __LINE__, __FUNCTION__);
      return -1;
    }

    max_dst_nb_samples = out_frame->nb_samples;
    //输入也可能是分平面的，所以要做如下处理
    uint8_t *m_ain[32];
    setup_array(m_ain, in_frame, in_sample_fmt, src_nb_samples);

    //注意这里，out_count和in_count是samples单位，不是byte
    //所以这样av_get_bytes_per_sample(ifmt_ctx->streams[audio_index]->codec->sample_fmt)
    //* src_nb_samples是错的
    len = swr_convert(pSwrCtx, out_frame->data, out_frame->nb_samples,
                      (const uint8_t **)in_frame->data, src_nb_samples);
    if (len < 0) {
      char errmsg[BUF_SIZE_1K];
      av_strerror(len, errmsg, sizeof(errmsg));
      av_log(NULL, AV_LOG_WARNING, "[%s:%d] swr_convert!(%d)(%s)", __FILE__,
             __LINE__, len, errmsg);
      return -1;
    }
  } else {
    printf("pSwrCtx with out init!\n");
    return -1;
  }
  return 0;
}

int Muxer::open_output_file(const char *filename) {
  AVStream *out_stream;
  AVCodecContext *enc_ctx;
  AVCodec *encoder;
  int ret;
  unsigned int i;
  ofmt_ctx = NULL;
  avformat_alloc_output_context2(&ofmt_ctx, NULL, "dash", filename);
  if (!ofmt_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
    return AVERROR_UNKNOWN;
  }
  {
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    out_stream->index = 0;
    if (!out_stream) {
      av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
      return AVERROR_UNKNOWN;
    }

    enc_ctx = out_stream->codec;
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;

    encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    enc_ctx->sample_rate = in_sample_rate;
    enc_ctx->channel_layout = in_channel_layout;
    enc_ctx->channels = in_channels;
    enc_ctx->sample_fmt = encoder->sample_fmts[0];
    AVRational ar = {1, enc_ctx->sample_rate};
    enc_ctx->time_base = ar;

    ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return ret;
    }

    av_opt_set(ofmt_ctx->priv_data, "preset", "superfast", 0);
    av_opt_set(ofmt_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set_int(ofmt_ctx->priv_data, "hls_time", 5, AV_OPT_SEARCH_CHILDREN);
    // av_opt_set_int(ofmt_ctx->priv_data,"hls_list_size", 10,
    // AV_OPT_SEARCH_CHILDREN);

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
      enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  av_dump_format(ofmt_ctx, 0, filename, 1);

  // if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
  ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
    return ret;
  }
  // }
  /* init muxer, write output file header */
  ret = avformat_write_header(ofmt_ctx, NULL);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
    return ret;
  }
  return 0;
}


int Muxer::encode_write_frame(AVFrame *filt_frame, unsigned int stream_index) {
  static int a_total_duration = 0;
  static int v_total_duration = 0;
  int ret;
  //int got_frame_local;
  //AVPacket enc_pkt;
  //int (*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
  //    avcodec_encode_audio2;

  //if (!got_frame) got_frame = &got_frame_local;

  //// av_log(NULL, AV_LOG_INFO, "Encoding frame\n");
  ///* encode filtered frame */
  //enc_pkt.data = NULL;
  //enc_pkt.size = 0;
  //av_init_packet(&enc_pkt);
  //ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt, filt_frame,
  //               got_frame);
  //if (ret < 0) return ret;
  //if (!(*got_frame)) return 0;
  //// if (ifmt_ctx->streams[stream_index]->codec->codec_type !=
  //// AVMEDIA_TYPE_VIDEO)
  //// av_bitstream_filter_filter(aacbsfc, ofmt_ctx->streams[stream_index]->codec,
  //// NULL, &enc_pkt.data, &enc_pkt.size, enc_pkt.data, enc_pkt.size, 0);

  ///* prepare packet for muxing */
  //enc_pkt.stream_index = stream_index;
  //av_packet_rescale_ts(&enc_pkt,
  //                     ofmt_ctx->streams[stream_index]->codec->time_base,
  //                     ofmt_ctx->streams[stream_index]->time_base);

  //if (ofmt_ctx->streams[stream_index]->codec->codec_type !=
  //    AVMEDIA_TYPE_VIDEO) {
  //  enc_pkt.pts = enc_pkt.dts = a_total_duration;
  //  a_total_duration +=
  //      av_rescale_q(filt_frame->nb_samples,
  //                   ofmt_ctx->streams[stream_index]->codec->time_base,
  //                   ofmt_ctx->streams[stream_index]->time_base);
  //}
  //// printf("v_total_duration: %d, a_total_duration: %d\n", v_total_duration,
  //// a_total_duration);
  //av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
  ///* mux encoded frame */
  //ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);

  ret = avcodec_send_frame(ofmt_ctx->streams[stream_index]->codec, frame);
  if (ret < 0) {
    E_LOG("Error while sending a frame to the encoder: {}", ret);
    return -1;
  }

  while (ret >= 0) {
    AVPacket pkt;
    av_init_packet(&pkt);
    ret = avcodec_receive_packet(ofmt_ctx->streams[stream_index]->codec, &pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    else if (ret < 0) {
      E_LOG("Error while encoding a frame: {}", ret);
      return -1;
    }

    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(&pkt,
                         ofmt_ctx->streams[stream_index]->codec->time_base,
                         ofmt_ctx->streams[stream_index]->time_base);
    pkt.stream_index = stream_index;

    if (ofmt_ctx->streams[stream_index]->codec->codec_type != AVMEDIA_TYPE_VIDEO) {
      pkt.pts = pkt.dts = a_total_duration;
      a_total_duration +=
          av_rescale_q(filt_frame->nb_samples,
                       ofmt_ctx->streams[stream_index]->codec->time_base,
                       ofmt_ctx->streams[stream_index]->time_base);
    }

    /* Write the compressed frame to the media file. */
    // log_packet(fmt_ctx, &pkt);
    ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
    av_packet_unref(&pkt);
    if (ret < 0) {
      E_LOG("Error while writing output packet: {}", ret);
      return -1;
    }
  }

  return ret;
}

int Muxer::flush_encoder(unsigned int stream_index) {
  int ret;
  int got_frame;
  AVPacket enc_pkt;

  if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
        AV_CODEC_CAP_DELAY))
    return 0;

  while (1) {
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
    ret = encode_write_frame(NULL, stream_index);
    if (ret < 0) break;
    if (!got_frame) return 0;
  }
  return ret;
}

int Muxer::mux(std::string input_aac, std::string dst) {
  int ret;
  FFmpegDecoder ff_decoder;
  enum AVMediaType type;
  unsigned int stream_index = 0;

  if ((ret = open_output_file(dst.c_str())) < 0) goto end;
  aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
  /* read all packets */
  int count = 0;
  int flag = 1;
  int len = 0;
  uint8_t aac_frame[2000] = {0};
  struct headerUtil::AdtsHeader adtsHeader;

  FILE *aac = fopen( input_aac.c_str(), "rb");
  if (aac == nullptr) {
    free(aac);
    return -1;
  }

  while (1) {
    len = fread(aac_frame, 1, 7, aac);  //先读adts的前七个字节头

    if (len <= 0) {
      fseek(aac, 0, SEEK_SET);
      I_LOG("send over");
      break;
    }

    int ret = headerUtil::parseAdtsHeader(aac_frame, &adtsHeader);
    if (headerUtil::parseAdtsHeader(aac_frame, &adtsHeader) < 0) {
      E_LOG("parse err");
      break;
    }

    // I_LOG("frmae len: {}", adtsHeader.aacFrameLength);

    len = fread(aac_frame + 7, 1, adtsHeader.aacFrameLength - 7, aac);

    if (len < 0) {
      E_LOG("read err");
      break;
    }

    ff_decoder.InputData((unsigned char *)aac_frame, adtsHeader.aacFrameLength);

    frame = ff_decoder.getFrame();

    // I_LOG("retxxx: {}, pkt size: {}, data: {}", ret, frame->pkt_size,
    // frame->pts);

    if (flag) {
      initSwr(stream_index);
      flag = 0;
    }

    AVFrame *frame_out = av_frame_alloc();
    if (0 != TransSample(frame, frame_out, stream_index)) {
      av_log(NULL, AV_LOG_ERROR, "convert audio failed\n");
      ret = -1;
    }
    // frame_out->pts = frame->pkt_pts;
    ret = encode_write_frame(frame_out, stream_index);
    av_frame_free(&frame_out);
    std::this_thread::sleep_for(std::chrono::milliseconds(23));
    ++count;
  }

  /* flush  encoders */
  // for (i = 0; i < ifmt_ctx->nb_streams; i++) {
  // ret = flush_encoder(i);
  // if (ret < 0) {
  // av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
  // goto end;
  // }
  // }
  //av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
  av_write_trailer(ofmt_ctx);

 end:
  // if (ret < 0)
  // av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
  // //av_err2str(ret));
  return ret ? 1 : 0;
}