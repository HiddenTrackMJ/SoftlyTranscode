/**
 * Author: Hidden Track
 * Date: 2020/07/07
 * Time: 10:45
 *
 * 混流器
 *
 */

#include <chrono>

#include "mixer.h"

// test for recv and mux
#include "fdk_dec.h"
#include "rtpRecv.h"


extern "C" {
#include "wav_reader.hpp"
#include "wav_writer.hpp"
}


Mixer::Mixer() {
  frame = av_frame_alloc();
  in_codec_info.sample_rate = 48000;
  in_codec_info.channels = 2;
  in_codec_info.channel_layout = av_get_default_channel_layout(in_codec_info.channels);
  in_codec_info.sample_fmt = AV_SAMPLE_FMT_FLTP;

  stop = false;
}

Mixer::~Mixer() {
    I_LOG("Mixer uninit...");
   //av_write_trailer(ofmt_ctx);
   //if (&packet) av_free_packet(&packet);
   //if (frame) av_frame_free(&frame);
   //if (_frame) av_frame_free(&_frame);

   //if (!ofmt_ctx) av_write_trailer(ofmt_ctx);
   avfilter_graph_free(&filter.filter_graph);
   avcodec_free_context(&filter.dec_ctx);
   avformat_close_input(&filter.fmt_ctx);
   avcodec_free_context(&filter._dec_ctx);
   avformat_close_input(&filter._fmt_ctx);


  if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
    avio_closep(&ofmt_ctx->pb);
  avformat_free_context(ofmt_ctx);
}

int Mixer::open_output_file(const char *filename) {
  AVStream *out_stream;
  AVCodec *encoder;
  int ret;
  ofmt_ctx = NULL;
  avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
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

    encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
      E_LOG("Could not alloc an encoding context\n");
      return -1;
    }
    enc_ctx = out_stream->codec;
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    enc_ctx->sample_rate = in_codec_info.sample_rate;
    enc_ctx->channel_layout = in_codec_info.channel_layout;
    enc_ctx->channels = in_codec_info.channels;
    enc_ctx->sample_fmt = in_codec_info.sample_fmt;
    //enc_ctx->bit_rate = 320000;
    enc_ctx->codec_tag = 0;
    AVRational ar = {1, enc_ctx->sample_rate};
    enc_ctx->time_base = ar;

    ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return ret;
    }

    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0) {
      E_LOG("Could not copy the stream parameters\n");
      return ret;
    }

    //av_opt_set(ofmt_ctx->priv_data, "preset", "superfast", 0);
    //av_opt_set(ofmt_ctx->priv_data, "tune", "zerolatency", 0);
    //av_opt_set_int(ofmt_ctx->priv_data, "hls_time", 5, AV_OPT_SEARCH_CHILDREN);
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
  /* init Mixer, write output file header */
  ret = avformat_write_header(ofmt_ctx, NULL);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
    return ret;
  }
  return 0;
}

int Mixer::encode_write_frame(AVFrame *filt_frame, unsigned int stream_index) {
  static int a_total_duration = 0;
  static int v_total_duration = 0;
  int ret;
  //I_LOG("Write frame {}", filt_frame->pts);
  ret = avcodec_send_frame(enc_ctx, filt_frame);
  if (ret < 0) {
    E_LOG("Error while sending a frame to the encoder: {}", ret);
    return -1;
  }

  while (ret >= 0) {
    AVPacket pkt;
    av_init_packet(&pkt);
    ret = avcodec_receive_packet(enc_ctx, &pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    else if (ret < 0) {
      E_LOG("Error while encoding a frame: {}", ret);
      return -1;
    }

    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(&pkt, enc_ctx->time_base,
                         ofmt_ctx->streams[stream_index]->time_base);
    pkt.stream_index = stream_index;

    if (enc_ctx->codec_type != AVMEDIA_TYPE_VIDEO) {
      pkt.pts = pkt.dts = a_total_duration;
      a_total_duration +=
          av_rescale_q(filt_frame->nb_samples, enc_ctx->time_base,
                       ofmt_ctx->streams[stream_index]->time_base);
    }

    /* Write the compressed frame to the media file. */
    // log_packet(fmt_ctx, &pkt);

    pkt.pts = pkt_count * ofmt_ctx->streams[stream_index]->codec->frame_size;
    pkt.dts = pkt.pts;
    pkt.duration = ofmt_ctx->streams[stream_index]->codec->frame_size;

    pkt.pts = av_rescale_q_rnd(
        pkt.pts, ofmt_ctx->streams[stream_index]->codec->time_base,
        ofmt_ctx->streams[stream_index]->time_base,
        (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = pkt.pts;
    pkt.duration = av_rescale_q_rnd(
        pkt.duration, ofmt_ctx->streams[stream_index]->codec->time_base,
        ofmt_ctx->streams[stream_index]->time_base,
        (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

    ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
    ++pkt_count;
    av_packet_unref(&pkt);
    if (ret < 0) {
      E_LOG("Error while writing output packet: {}", ret);
      return -1;
    }
  }

  return ret;
}

int Mixer::init_filter(const char *filter_desc) {
  char args1[512];
  char args2[512];
  int ret = 0;
  const AVFilter *abuffersrc1 = avfilter_get_by_name("abuffer");
  const AVFilter *abuffersrc2 = avfilter_get_by_name("abuffer");
  const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

  AVFilterInOut *outputs1 = avfilter_inout_alloc();
  AVFilterInOut *outputs2 = avfilter_inout_alloc();
  AVFilterInOut *inputs = avfilter_inout_alloc();

  static const enum AVSampleFormat out_sample_fmts[] = {
       filter.dec_ctx->sample_fmt, (enum AVSampleFormat) - 1};  
      // AV_SAMPLE_FMT_S16,   (enum AVSampleFormat) - 1}; 
  static const int64_t out_channel_layouts[] =
      {filter.dec_ctx->channel_layout, -1};
      //{AV_CH_LAYOUT_MONO, -1};
  static const int out_sample_rates[] = {filter.dec_ctx->sample_rate, -1};
  const AVFilterLink *outlink;

  AVRational time_base_1 =
      filter.fmt_ctx->streams[filter.audio_stream_index]->time_base;
  AVRational time_base_2 =
      filter._fmt_ctx->streams[filter._audio_stream_index]->time_base;

  filter.filter_graph = avfilter_graph_alloc();
  if (!outputs1 || !outputs2 || !inputs || !filter.filter_graph) {
    ret = AVERROR(ENOMEM);
    E_LOG("init filter input output error");
    return -1;
  }

  std::cout << "sample_fmt: "
            << av_get_sample_fmt_name(filter.dec_ctx->sample_fmt)
            << " channel_layout: " << filter.dec_ctx->channel_layout
            << " sample_rates: " << out_sample_rates[0]
            << " bits_per_raw_sample: " << filter.dec_ctx->bits_per_raw_sample
            << std::endl;

  std::cout << "sample_fmt2: "
            << av_get_sample_fmt_name(filter._dec_ctx->sample_fmt)
            << " channel_layout2: " << filter._dec_ctx->channel_layout
            << " sample_rates2: " << out_sample_rates[0]
            << " bits_per_raw_sample2: " << filter._dec_ctx->bits_per_raw_sample
            << std::endl;

  /* buffer audio source: the decoded frames from the decoder will be inserted
   * here. */
  if (!filter.dec_ctx->channel_layout)
    filter.dec_ctx->channel_layout =
        av_get_default_channel_layout(filter.dec_ctx->channels);
  snprintf(
      args1, sizeof(args1),
      "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
      time_base_1.num, time_base_1.den, filter.dec_ctx->sample_rate,
      av_get_sample_fmt_name(filter.dec_ctx->sample_fmt), filter.dec_ctx->channel_layout);
  ret = avfilter_graph_create_filter(&filter.buffersrc_ctx, abuffersrc1, "in0", args1, NULL, filter.filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source1\n");
    goto end;
  }


  /* buffer audio source: the decoded frames from the decoder will be inserted
   * here. */
  if (!filter._dec_ctx->channel_layout)
    filter._dec_ctx->channel_layout =
        av_get_default_channel_layout(filter._dec_ctx->channels);
  snprintf(
      args2, sizeof(args2),
      "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
      time_base_2.num, time_base_2.den, filter._dec_ctx->sample_rate,
      av_get_sample_fmt_name(filter._dec_ctx->sample_fmt), filter._dec_ctx->channel_layout);
  ret = avfilter_graph_create_filter(&filter._buffersrc_ctx, abuffersrc2, "in1", args2, NULL, filter.filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source2\n");
    goto end;
  }
  /* buffer audio sink: to terminate the filter chain. */
  ret = avfilter_graph_create_filter(&filter.buffersink_ctx,
                                     abuffersink, "out", NULL, NULL, filter.filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
    goto end;
  }

  ret = av_opt_set_int_list(filter.buffersink_ctx, "sample_fmts",
                            out_sample_fmts, -1,
                            AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
    goto end;
  }

  ret = av_opt_set_int_list(filter.buffersink_ctx, "channel_layouts",
                            out_channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
    goto end;
  }

  ret = av_opt_set_int_list(filter.buffersink_ctx, "sample_rates",
                            out_sample_rates,
                            -1, AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
    goto end;
  }

  /*
   * Set the endpoints for the filter graph. The filter_graph will
   * be linked to the graph described by filters_descr.
   */

  /*
   * The buffer source output must be connected to the input pad of
   * the first filter described by filters_descr; since the first
   * filter input label is not specified, it is set to "in" by
   * default.
   */
  outputs1->name = av_strdup("in0");
  outputs1->filter_ctx = filter.buffersrc_ctx;
  outputs1->pad_idx = 0;
  outputs1->next = outputs2;

  outputs2->name = av_strdup("in1");
  outputs2->filter_ctx = filter._buffersrc_ctx;
  outputs2->pad_idx = 0;
  outputs2->next = NULL;

  /*
   * The buffer sink input must be connected to the output pad of
   * the last filter described by filters_descr; since the last
   * filter output label is not specified, it is set to "out" by
   * default.
   */
  inputs->name = av_strdup("out");
  inputs->filter_ctx = filter.buffersink_ctx;
  inputs->pad_idx = 0;
  inputs->next = NULL;

  AVFilterInOut *filter_outputs[2];
  filter_outputs[0] = outputs1;
  filter_outputs[1] = outputs2;


  if ((ret = avfilter_graph_parse_ptr(filter.filter_graph, filter.filter_desc,
                                      &inputs, filter_outputs, NULL)) <
      0)  // filter_outputs
  {
    av_log(NULL, AV_LOG_ERROR, "parse ptr fail, ret: %d\n", ret);
    goto end;
  }

  if ((ret = avfilter_graph_config(filter.filter_graph, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "config graph fail, ret: %d\n", ret);
    goto end;
  }

  /* Print summary of the sink buffer
   * Note: args buffer is reused to store channel layout string */
  outlink = filter.buffersink_ctx->inputs[0];
  av_get_channel_layout_string(args1, sizeof(args1), -1,
                               outlink->channel_layout);
  av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
         (int)outlink->sample_rate,
         (char *)av_x_if_null(
             av_get_sample_fmt_name((enum AVSampleFormat)outlink->format), "?"),
         args1);

end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(filter_outputs);

  //char *temp = avfilter_graph_dump(filter.filter_graph, NULL);
  //printf("%s\n", temp);

  return ret;
}

int Mixer::_open_input_file(const char *filename) {
  int ret;
  AVCodec *dec;

  I_LOG("filename: {}", filename);

  if ((ret = avformat_open_input(&(filter._fmt_ctx), filename, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }

  if ((ret = avformat_find_stream_info(filter._fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  /* select the audio stream */
  ret = av_find_best_stream(filter._fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Cannot find an audio stream in the input file\n");
    return ret;
  }
  filter._audio_stream_index = ret;

  /* create decoding context */
  filter._dec_ctx = avcodec_alloc_context3(dec);
  if (!filter._dec_ctx) return AVERROR(ENOMEM);
  avcodec_parameters_to_context(
      filter._dec_ctx,
      filter._fmt_ctx->streams[filter._audio_stream_index]->codecpar);

  /* init the audio decoder */
  if ((ret = avcodec_open2(filter._dec_ctx, dec, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
    return ret;
  }

  return 0;
}

int Mixer::open_input_file(const char *filename) {
  int ret;
  AVCodec *dec;

  I_LOG("filename: {}", filename);

  if ((ret = avformat_open_input(&(filter.fmt_ctx), filename, NULL, NULL)) <
      0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }

  if ((ret = avformat_find_stream_info(filter.fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  /* select the audio stream */
  ret =
      av_find_best_stream(filter.fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Cannot find an audio stream in the input file\n");
    return ret;
  }
  filter.audio_stream_index = ret;

  /* create decoding context */
  filter.dec_ctx = avcodec_alloc_context3(dec);
  if (!filter.dec_ctx) return AVERROR(ENOMEM);
  avcodec_parameters_to_context(
      filter.dec_ctx,
      filter.fmt_ctx->streams[filter.audio_stream_index]->codecpar);

  /* init the audio decoder */
  if ((ret = avcodec_open2(filter.dec_ctx, dec, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
    return ret;
  }

  return 0;
}

int Mixer::recv_aac_mix(int port, std::string input_aac, std::string dst_file) {
  using namespace jrtplib;

  int ret;
  FFmpegDecoder ff_decoder;
  enum AVMediaType type;
  unsigned int stream_index = 0;

  if ((ret = open_output_file(dst_file.c_str())) < 0) return ret;

  /* read all packets */
  int count = 0;
  int flag = 1;
  int len = 0;

  struct headerUtil::AdtsHeader adtsHeader;

  FILE *aac = fopen(input_aac.c_str(), "rb");
  if (aac == nullptr) {
    free(aac);
    return -1;
  }

  seeker::SocketUtil::startupWSA();

  size_t payloadlen = 0;
  uint8_t *payloaddata;

  RTPSession rtpSession;
  RTPSessionParams rtpSessionParams;
  RTPUDPv4TransmissionParams rtpUdpv4Transmissionparams;

  rtpSessionParams.SetOwnTimestampUnit(1.0 / 8000);
  rtpSessionParams.SetUsePollThread(false);
  rtpUdpv4Transmissionparams.SetPortbase(port);
  rtpSessionParams.SetAcceptOwnPackets(true);

  int status = rtpSession.Create(rtpSessionParams, &rtpUdpv4Transmissionparams);
  RtpReceiver::checkerror(status);

  bool stop = false;

  while (!stop) {
    rtpSession.BeginDataAccess();
    RTPPacket *pack;

    // check incoming packets
    if (rtpSession.GotoFirstSourceWithData()) {
      do {
        while ((pack = rtpSession.GetNextPacket()) != NULL) {
          AVFrame *frame_out = av_frame_alloc();
          payloaddata = pack->GetPayloadData();
          payloadlen = pack->GetPayloadLength();

          if (payloadlen < 100) {
            std::cout << "Bye: " << (uint16_t *)payloaddata << std::endl;
            goto Exit;
          }

          adts_header_t *adts = (adts_header_t *)(&payloaddata[0]);

          if (adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf) {
            I_LOG("333...");
            break;
          }

          int aac_frame_size = adts->frame_length_0_to_1 << 11 |
                               adts->frame_length_2_to_9 << 3 |
                               adts->frame_length_10_to_12;

          // I_LOG("size: {}, len, {}", aac_frame_size, payloadlen);

          len = aac_frame_size;

          ff_decoder.InputData((unsigned char *)&payloaddata[0], len);

          frame = ff_decoder.getFrame();

          // I_LOG("retxxx: {}, pkt size: {}, data: {}", ret, frame->pkt_size,
          // frame->pts);

          try {
          } catch (const std::overflow_error &e) {
            E_LOG("err: {}", e.what());
          } catch (const std::runtime_error &e) {
            E_LOG("err: {}", e.what());
          } catch (const std::exception &e) {
            E_LOG("err: {}", e.what());
          } catch (...) {
            E_LOG("err101");
          }

          ret = encode_write_frame(frame_out, stream_index);
          av_frame_free(&frame_out);

          std::this_thread::sleep_for(std::chrono::milliseconds(23));
          ++count;

          rtpSession.DeletePacket(pack);
        }
      } while (rtpSession.GotoNextSourceWithData());
    }

    rtpSession.EndDataAccess();

    int status = rtpSession.Poll();
    RtpReceiver::checkerror(status);

    RTPTime::Wait(RTPTime(0, 10));
  }
Exit:
  av_write_trailer(ofmt_ctx);
  // av_frame_free(&frame);
  rtpSession.Destroy();

  I_LOG("Closed sess");
}

int Mixer::recv_aac_thread( std::string input_aac, pkt_list_info* pkt_list_ctx, int index ) {
  int ret;
  if (index == 1) {
    if ((ret = open_input_file(input_aac.c_str())) < 0) return -1;
    if ((ret = init_filter(filter.filter_desc)) < 0) return -1;
  } else {
    if ((ret = _open_input_file(input_aac.c_str())) < 0) return -1;
  }

  /* read all packets */
  int count = 0;
  int flag = 1;
  int len = 0;
  uint8_t aac_frame[2000] = {0};
  struct headerUtil::AdtsHeader adtsHeader;

  FILE *aac = fopen(input_aac.c_str(), "rb");
  if (aac == nullptr) {
    free(aac);
    return -1;
  }

  while (1) {
    uint8_t *buff_c;
    len = fread(aac_frame, 1, 7, aac);  //先读adts的前七个字节头

    if (len <= 0) {
      fseek(aac, 0, SEEK_SET);
      I_LOG("{} send over", input_aac.c_str());
      break;
    }

    int ret = headerUtil::parseAdtsHeader(aac_frame, &adtsHeader);
    if (headerUtil::parseAdtsHeader(aac_frame, &adtsHeader) < 0) {
      E_LOG("parse err");
      break;
    }

     

    len = fread(aac_frame + 7, 1, adtsHeader.aacFrameLength - 7, aac);

    //I_LOG("frmae len: {}, size: {}", adtsHeader.aacFrameLength, len);

    buff_c = new uint8_t[adtsHeader.aacFrameLength + 1]();
    memcpy(buff_c, &aac_frame[0], adtsHeader.aacFrameLength);
    std::unique_ptr<uint8_t> pkt{buff_c};

    (*pkt_list_ctx).pkt_list_mu->lock();
    (*pkt_list_ctx) .pkt_list.push_back( std::make_pair(std::move(pkt), adtsHeader.aacFrameLength));
    (*pkt_list_ctx).pkt_list_mu->unlock();
    (*pkt_list_ctx).pkt_list_cv->notify_one();

   
    // std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ++count;
  }

  //av_write_trailer(ofmt_ctx);

end:
  // if (ret < 0)
  // av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
  // //av_err2str(ret));
  stop = 1;
  return ret ? 1 : 0;
}

int Mixer::process_thread(std::string dst_file) {
  int ret;
  int count = 0;

  unsigned int stream_index = 0;
  FFmpegDecoder ff_decoder;
  FFmpegDecoder ff_decoder1;
  AVFrame *filt_frame = av_frame_alloc();
  frame = av_frame_alloc();

  if ((ret = open_output_file(dst_file.c_str())) < 0) return ret;

  while (1) {
    AVFrame *frame_out = av_frame_alloc();

    // first stream
    std::unique_lock<std::mutex> lock(*(pkt_list_ctx1.pkt_list_mu));
    pkt_list_ctx1.pkt_list_cv->wait(lock, [this] {
      return !pkt_list_ctx1.pkt_list.empty() ||
             !pkt_list_ctx2.pkt_list.empty() || stop;
    });
    if (stop &&
        (pkt_list_ctx1.pkt_list.empty() || pkt_list_ctx2.pkt_list.empty()))
      break;
    if (!pkt_list_ctx1.pkt_list.empty()) {
      uint8_t *pkt = pkt_list_ctx1.pkt_list.front().first.release();
      int size = pkt_list_ctx1.pkt_list.front().second;
      pkt_list_ctx1.pkt_list.pop_front();

      /*I_LOG("list size1: {}, list size2 {}", pkt_list_ctx1.pkt_list.size(),
            pkt_list_ctx2.pkt_list.size());*/
      ff_decoder.InputData((unsigned char *)&pkt[0], size);

      frame = ff_decoder.getFrame();
      ff_decoder.reset();
      
      frame->nb_samples = filter.fmt_ctx->streams[stream_index]->codec->frame_size;
      frame->channel_layout = filter.dec_ctx->channel_layout;
      frame->format = filter.dec_ctx->sample_fmt;
      frame->sample_rate = filter.dec_ctx->sample_rate;
      frame->pts = av_frame_get_best_effort_timestamp(frame);
    } else {
      frame = NULL;
    }

    /* push the audio data from decoded frame into the filtergraph */
    if (av_buffersrc_add_frame_flags(filter.buffersrc_ctx, frame, 0) < 0) {
      av_log(NULL, AV_LOG_ERROR,
             "Error while feeding the audio filtergraph1\n");
      break;
    }
    av_frame_unref(frame);

  


    //second stream
    std::unique_lock<std::mutex> _lock(*(pkt_list_ctx2.pkt_list_mu));
    pkt_list_ctx2.pkt_list_cv->wait(_lock, [this] {
      return !pkt_list_ctx1.pkt_list.empty() ||
             !pkt_list_ctx2.pkt_list.empty() || stop;
    });
    if (stop && (pkt_list_ctx1.pkt_list.empty() ||
        pkt_list_ctx2.pkt_list.empty()))
      break;
    if (!pkt_list_ctx2.pkt_list.empty()) {
      uint8_t *pkt = pkt_list_ctx2.pkt_list.front().first.release();
      int size = pkt_list_ctx2.pkt_list.front().second;
      pkt_list_ctx2.pkt_list.pop_front();

      ff_decoder1.InputData((unsigned char *)&pkt[0], size);

      frame = ff_decoder1.getFrame();
      ff_decoder1.reset();

      frame->nb_samples = filter._fmt_ctx->streams[stream_index]->codec->frame_size;
      frame->channel_layout = filter._dec_ctx->channel_layout;
      frame->format = filter._dec_ctx->sample_fmt;
      frame->sample_rate = filter._dec_ctx->sample_rate;
      frame->pts = av_frame_get_best_effort_timestamp(frame);
      
    } else {
      frame = NULL;
    }

    /* push the audio data from decoded frame into the filtergraph */
    if (av_buffersrc_add_frame_flags(filter._buffersrc_ctx, frame, 0) < 0) {
      av_log(NULL, AV_LOG_ERROR,
             "Error while feeding the audio filtergraph2\n");
      break;
    }

    av_frame_unref(frame);


    /* pull filtered audio from the filtergraph */
    while (1) {
      ret = av_buffersink_get_frame(filter.buffersink_ctx, filt_frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
      if (ret < 0) {
        E_LOG("Failed to av_buffersink_get_frame_flags");
        return -1;
      }
      if (filt_frame->data[0] != NULL) {
        encode_write_frame(filt_frame, stream_index);
      } else {
        W_LOG("No data");
      }
     
      av_frame_unref(filt_frame);
    }

    av_frame_free(&frame_out);
    ++count;
  }
  av_frame_free(&filt_frame);
  //if (frame) av_frame_free(&frame);
  //if (_frame) av_frame_free(&_frame);
  av_write_trailer(ofmt_ctx);
  I_LOG("mux to {} stopped...", dst_file);
}

int Mixer::open_thread(int port, std::string dst) {
  std::string filename1 =
      "D:/Study/Scala/VSWS/retream/out/build/x64-Release/recv.aac";
      //"D:/Study/Scala/VSWS/transcode/out/build/x64-Release/trip.aac";
  std::string filename2 =
      //"D:/Study/Scala/VSWS/retream/out/build/x64-Release/recv.aac";
      //"D:/Study/Scala/VSWS/transcode/out/build/1.aac";
      "D:/Study/Scala/VSWS/transcode/out/build/x64-Release/trip.aac";
  try {

    int ret = 0;
    
    std::thread recv_thread1{std::mem_fn(&Mixer::recv_aac_thread),
                             std::ref(*this), filename1,
                             &pkt_list_ctx1, 1};

    std::thread recv_thread2{std::mem_fn(&Mixer::recv_aac_thread),
                             std::ref(*this),  filename2,
                             &pkt_list_ctx2, 2};

    recv_thread1.detach();
    recv_thread2.detach();
    //stop = 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
 
    std::thread mix_thread{std::mem_fn(&Mixer::process_thread), std::ref(*this),
                           dst};

    mix_thread.join();
  } catch (const std::system_error &e) {
    E_LOG("Caught system_error with code");
    std::cout << e.code() << std::endl;
    std::cout << e.what() << std::endl;
  }
}