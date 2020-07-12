///*
// * Copyright (c) 2010 Nicolas George
// * Copyright (c) 2011 Stefano Sabatini
// * Copyright (c) 2012 Clément Bœsch
// *
// * Permission is hereby granted, free of charge, to any person obtaining a copy
// * of this software and associated documentation files (the "Software"), to deal
// * in the Software without restriction, including without limitation the rights
// * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// * copies of the Software, and to permit persons to whom the Software is
// * furnished to do so, subject to the following conditions:
// *
// * The above copyright notice and this permission notice shall be included in
// * all copies or substantial portions of the Software.
// *
// * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// * THE SOFTWARE.
// */
//
///**
// * @file
// * API example for audio decoding and filtering
// * @example filtering_audio.c
// */
//
//extern "C" {
//    #include "SDL.h"
//#include <libavcodec/avcodec.h>
//#include <libavfilter/buffersink.h>
//#include <libavfilter/buffersrc.h>
//#include <libavformat/avformat.h>
//#include <libavutil/opt.h>
//}
////#include <unistd.h>
//
//#include <string>
//#include <iostream>
//#include <thread>
//#include <tuple>
//#include "FFmpegUtil.h"
//#include "wav_writer.hpp"
//#include "wav_reader.hpp"
//
//#define BUF_SIZE_20K 2048000
//#define BUF_SIZE_1K 1024000
//
//
//static const char *filter_desc =
//    "aresample=48000,aformat=sample_fmts=fltp:channel_layouts=stereo";
//static const char *player = "ffplay -f s16le -ar 8000 -ac 1 -";
//
//static AVFormatContext *fmt_ctx;
//static AVCodecContext *dec_ctx;
//AVFilterContext *buffersink_ctx;
//AVFilterContext *buffersrc_ctx;
//AVFilterGraph *filter_graph;
//static int audio_stream_index = -1;
//void *wav = NULL;
//std::string output = "./test.wav";
//
//static SwrContext *pSwrCtx = NULL;
//
//void initSwr(int audio_index) {
//
//  if (NULL == pSwrCtx) {
//    pSwrCtx = swr_alloc();
//  }
//
//  pSwrCtx = swr_alloc_set_opts(
//      NULL, fmt_ctx->streams[audio_index]->codec->channel_layout,
//      fmt_ctx->streams[audio_index]->codec->sample_fmt,
//      fmt_ctx->streams[audio_index]->codec->sample_rate,
//      fmt_ctx->streams[audio_index]->codec->channel_layout,
//      fmt_ctx->streams[audio_index]->codec->sample_fmt,
//      fmt_ctx->streams[audio_index]->codec->sample_rate, 0,
//      NULL);
//
//  if(swr_init(pSwrCtx))
//    std::cout << "swr init" << std::endl;
//  else {
//    std::cout << " swr init err " << std::endl;
//  }
//
//}
//
//
//static void setup_array(uint8_t *out[32], AVFrame *in_frame,
//                        enum AVSampleFormat format, int samples) {
//  if (av_sample_fmt_is_planar(format)) {
//    int i;
//    int plane_size = av_get_bytes_per_sample((format)) * samples;
//    in_frame->data[0] + i *plane_size;
//    for (i = 0; i < in_frame->channels; i++) {
//      out[i] = in_frame->data[i];
//    }
//  } else {
//    out[0] = in_frame->data[0];
//  }
//}
//
//std::tuple<int, int> reSample(uint8_t *dataBuffer, int dataBufferSize,
//                              const AVFrame *frame) {
//  std::cout << "reSample: nb_samples=" << frame->nb_samples
//            << ", sample_rate = " << frame->sample_rate
//            << ", frame_data: " << (const uint8_t **)&frame->data[0]
//            << ", reSample:dataBufferSize=" << dataBufferSize
//            << ", dataBuffer = " << (const uint8_t **)&dataBuffer << std::endl;
//
//  int outSamples =
//      swr_convert(pSwrCtx, &dataBuffer, dataBufferSize,
//                  (const uint8_t **)&frame->data[0], frame->nb_samples);
//   std::cout << "reSample: nb_samples=" << frame->nb_samples << ", sample_rate = " << frame->sample_rate <<  ", outSamples=" << outSamples << std::endl;
//  if (outSamples <= 0) {
//    // throw std::runtime_error("error: outSamples=" + outSamples);
//  }
//
//  int outDataSize = av_samples_get_buffer_size(
//      NULL, dec_ctx->channels, outSamples,
//      dec_ctx->sample_fmt, 1);
//
//  if (outDataSize <= 0) {
//    // throw std::runtime_error("error: outDataSize=" + outDataSize);
//  }
//
//  return {outSamples, outDataSize};
//}
//
//int TransSample(AVFrame *in_frame, AVFrame *out_frame, int audio_index) {
//  int ret;
//  int max_dst_nb_samples = 4096;
//  int64_t src_nb_samples = in_frame->nb_samples;
//  out_frame->pts = in_frame->pts;
//  uint8_t *paudiobuf;
//  int decode_size, input_size, len;
//  
//  if (pSwrCtx != nullptr) {
//    out_frame->nb_samples = av_rescale_rnd(
//        swr_get_delay(pSwrCtx, dec_ctx->sample_rate) + src_nb_samples,
//        dec_ctx->sample_rate, dec_ctx->sample_rate,
//        AV_ROUND_UP);
//
//    ret = av_samples_alloc(
//        out_frame->data, &out_frame->linesize[0],
//                           dec_ctx->channels, out_frame->nb_samples,
//                           dec_ctx->sample_fmt, 0);
//
//    if (ret < 0) {
//      av_log(NULL, AV_LOG_WARNING,
//             "[%s.%d %s() Could not allocate samples Buffer\n", __FILE__,
//             __LINE__, __FUNCTION__);
//      return -1;
//    }
//
//    max_dst_nb_samples = out_frame->nb_samples;
//
//    uint8_t *m_ain[32];
//    setup_array(m_ain, in_frame,
//                dec_ctx->sample_fmt,
//                src_nb_samples);
//
//
//    len = swr_convert(pSwrCtx, out_frame->data, out_frame->nb_samples,
//                      (const uint8_t **)in_frame->data, src_nb_samples);
//    
//    if (len < 0) {
//      char errmsg[BUF_SIZE_1K];
//      av_strerror(len, errmsg, sizeof(errmsg));
//      av_log(NULL, AV_LOG_WARNING, "[%s:%d] swr_convert!(%d)(%s)", __FILE__,
//             __LINE__, len, errmsg);
//      return -1;
//    }
//  } else {
//    std::cout << "others " << std::endl;
//    printf("pSwrCtx with out init!\n");
//    return -1;
//  }
//  return 0;
//}
//
//
//static int open_input_file(const char *filename) {
//  int ret;
//  AVCodec *dec;
//
//  if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
//    return ret;
//  }
//
//  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
//    return ret;
//  }
//
//  /* select the audio stream */
//  ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
//  if (ret < 0) {
//    av_log(NULL, AV_LOG_ERROR,
//           "Cannot find an audio stream in the input file\n");
//    return ret;
//  }
//  audio_stream_index = ret;
//
//  /* create decoding context */
//  dec_ctx = avcodec_alloc_context3(dec);
//  if (!dec_ctx) return AVERROR(ENOMEM);
//  avcodec_parameters_to_context(dec_ctx,
//                                fmt_ctx->streams[audio_stream_index]->codecpar);
//
//  /* init the audio decoder */
//  if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
//    return ret;
//  }
//
//  return 0;
//}
//
//static int init_filters(const char *filters_descr) {
//
//  /** An instance of a filter */
//  AVFilterContext *buffersink_ctx; //sink filter实例（输出）
//  AVFilterContext *buffersrc_ctx;  //src filter实例（输入）
//  AVFilterGraph *filter_graph;
//  char args[512];  
//  int ret = 0;
//
//  /**
//   * Filter definition. This defines the pads a filter contains, and all the
//   * callback functions used to interact with the filter.
//   */
//  const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");  //有多少个输入就创建多少个，当输入输出为一的时候相当于直接重采样
//  const AVFilter *abuffersink = avfilter_get_by_name("abuffersink"); //有多少个输出就创建多少个
//
//  /**
//   * A linked-list of the inputs/outputs of the filter chain.
//   *
//   * This is mainly useful for avfilter_graph_parse() / avfilter_graph_parse2(),
//   * where it is used to communicate open (unlinked) inputs and outputs from and
//   * to the caller.
//   * This struct specifies, per each not connected pad contained in the graph,
//   * the filter context and the pad index required for establishing a link.
//   */
//
//  AVFilterInOut *outputs = avfilter_inout_alloc(); //输入filter chain
//  AVFilterInOut *inputs = avfilter_inout_alloc();  //输出filter chain
//  static const enum AVSampleFormat out_sample_fmts[] = {
//      AV_SAMPLE_FMT_FLTP, (enum AVSampleFormat) - 1};  //输出sample_fmt
//  static const int64_t out_channel_layouts[] = {3, -1};//输出channel_layouts
//  static const int out_sample_rates[] = {48000, -1};   //输出sample_rates
// 
//  const AVFilterLink *outlink;
//  AVRational time_base = fmt_ctx->streams[audio_stream_index]->time_base;
//
//  filter_graph = avfilter_graph_alloc();
//  if (!outputs || !inputs || !filter_graph) {
//    ret = AVERROR(ENOMEM);
//    goto end;
//  }
//
//  /* buffer audio source: the decoded frames from the decoder will be inserted
//   * here. */
//  if (!dec_ctx->channel_layout)
//    dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
//  snprintf(
//      args, sizeof(args),
//      "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
//      time_base.num, time_base.den, dec_ctx->sample_rate,
//      av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);
//
//  ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in", args,
//                                     NULL, filter_graph);
//  if (ret < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
//    goto end;
//  }
//
//  /* buffer audio sink: to terminate the filter chain. */
//  ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL,
//                                     NULL, filter_graph);
//  if (ret < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
//    goto end;
//  }
//
//  ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
//                            AV_OPT_SEARCH_CHILDREN);
//  if (ret < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
//    goto end;
//  }
//
//  ret = av_opt_set_int_list(buffersink_ctx, "channel_layouts",
//                            out_channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
//  if (ret < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
//    goto end;
//  }
//
//  ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", out_sample_rates,
//                            -1, AV_OPT_SEARCH_CHILDREN);
//  if (ret < 0) {
//    av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
//    goto end;
//  }
//
//  /*
//   * Set the endpoints for the filter graph. The filter_graph will
//   * be linked to the graph described by filters_descr.
//   */
//
//  /*
//   * The buffer source output must be connected to the input pad of
//   * the first filter described by filters_descr; since the first
//   * filter input label is not specified, it is set to "in" by
//   * default.
//   */
//  outputs->name = av_strdup("in");
//  outputs->filter_ctx = buffersrc_ctx;
//  outputs->pad_idx = 0;
//  outputs->next = NULL;
//
//  //outputs1->name = av_strdup("in0");
//  //outputs1->filter_ctx = buffersrc_ctx1;
//  //outputs1->pad_idx = 0;
//  //outputs1->next = outputs2;
//
//  //outputs2->name = av_strdup("in1");
//  //outputs2->filter_ctx = buffersrc_ctx2;
//  //outputs2->pad_idx = 0;
//  //outputs2->next = NULL;
//
//  /*
//   * The buffer sink input must be connected to the output pad of
//   * the last filter described by filters_descr; since the last
//   * filter output label is not specified, it is set to "out" by
//   * default.
//   */
//  inputs->name = av_strdup("out");
//  inputs->filter_ctx = buffersink_ctx;
//  inputs->pad_idx = 0;
//  inputs->next = NULL;
//
//  if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs,
//                                      &outputs, NULL)) < 0)
//    goto end;
//
//  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) goto end;
//
//  /* Print summary of the sink buffer
//   * Note: args buffer is reused to store channel layout string */
//  outlink = buffersink_ctx->inputs[0];
//  av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
//  av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
//         (int)outlink->sample_rate,
//         (char *)av_x_if_null(
//             av_get_sample_fmt_name((enum AVSampleFormat)outlink->format), "?"),
//         args);
//
//end:
//  avfilter_inout_free(&inputs);
//  avfilter_inout_free(&outputs);
//
//  return ret;
//}
//
//
//static void print_frame(const AVFrame *frame) {
//  const int n = frame->nb_samples *
//                av_get_channel_layout_nb_channels(dec_ctx->channel_layout);
//  //std::cout << "print_frame" << std::endl;
//  const uint16_t *p = (uint16_t *)frame->data[0];
//  const uint16_t *p_end = p + n;
//
//  FILE *file = NULL;
//  //while (p < p_end) {
//  //  fputc(*p & 0xff, stdout);
//  //  fputc(*p >> 8 & 0xff, stdout);
//  //  p++;
//  //}
//  //fflush(stdout);
// 
// file = fopen("tmp.pcm", "ab+");
//  if (NULL == file) {
//    //perror("fopen tmp.mp3 error\n");
//    return;
//  } else {
//    //perror("fopen tmp.aac successful\n");
//  }
//  fwrite(frame->data[0], n * 2, 1, file);
//  fclose(file);
//
// 
//
//  //if (!wav) {
//  //  wav = wav_write_open(output.c_str(), dec_ctx->sample_rate,
//  //                       16,
//  //                       dec_ctx->channels);
//  //}
//
//  //wav_write_data(wav, (unsigned char *)&frame->data[0], n * 2);
//  
//}
//
//void audio_callback(void *userdata, Uint8 *stream, int len) {
//  using namespace std;
//  //AudioData *audioData = (AudioData *)userdata;
//  std::cout << " call back"<< std::endl;
//  FFmpegUtil::ReSampler *reSampler = (FFmpegUtil::ReSampler *)userdata;
//
//  static uint8_t *outBuffer = nullptr;
//  static int outBufferSize = 0;
//  static AVFrame *aFrame = av_frame_alloc();
//  static AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
//  int ret = 0;
//  av_init_packet(packet);
//  //AVFormatContext *fmt_ctx = fmt_ctx;
//  AVCodecContext *codec_ctx = dec_ctx;
//
//  while (true) {
//    while (1) {
//      ret = av_read_frame(fmt_ctx, packet);
//      if (ret < 0) {
//        cout << "read frame error" << endl;
//      }
//      if (packet->stream_index == audio_stream_index) break;
//    }
//    {
//      ret = avcodec_send_packet(codec_ctx, packet);
//      if (ret >= 0) {
//        av_packet_unref(packet);
//        int ret = avcodec_receive_frame(codec_ctx, aFrame);
//        if (ret >= 0) {
//          ret = 2;
//          break;
//        } else if (ret == AVERROR(EAGAIN)) {
//          continue;
//        } else {
//          cout << " Failed to avcodec_receive_frame " << endl;
//        }
//      } else if (ret == AVERROR(EAGAIN)) {
//        // buff full, can not decode anymore, do nothing.
//        cout << "Failed to avcodec_send_packet  fullll" << endl;
//      } else {
//        cout << "Failed to avcodec_send_packet" << endl;
//      }
//    }
//  }
//
//  int outDataSize = -1;
//  int outSamples = -1;
//
//  std::cout << "test: " << aFrame->nb_samples << std::endl;
//
//  //if (outBuffer == nullptr) {
//  //  outBufferSize = 2 * aFrame->nb_samples *
//  //                  av_get_channel_layout_nb_channels(aFrame->channel_layout);
//  //  outBuffer = (uint8_t *)av_malloc(sizeof(uint8_t) * outBufferSize);
//  //  //reSampler->allocDataBuf(&outBuffer, aFrame->nb_samples);
//  //} else {
//  //  memset(outBuffer, 0, outBufferSize);
//  //}
//
//  //try {
//  //  std::tie(outSamples, outDataSize) =
//  //      reSample(outBuffer, outBufferSize, aFrame);
//  //} catch (std::runtime_error e) {
//  //  std::cout << e.what() << std::endl;
//  //}
//
//  //if (outDataSize != len) {
//  //  cout << "WARNING: outDataSize[" << outDataSize << "] != len[" << len << "]"
//  //       << endl;
//  //}
//
//  std::memcpy(stream, &aFrame->data[0],
//              2 * aFrame->nb_samples *
//                  av_get_channel_layout_nb_channels(aFrame->channel_layout));
//}
//
//void playAudio(FFmpegUtil::ffmpeg_util f, SDL_AudioDeviceID &audioDeviceID) {
//  // for audio play
//  //auto fmt_ctx = f.get_fmt_ctx();
//  auto acodec_ctx = dec_ctx;  // f.get_acodec_ctx();
//  int64_t in_layout = acodec_ctx->channel_layout;
//  int in_channels = acodec_ctx->channels;
//  int in_sample_rate = acodec_ctx->sample_rate;
//  AVSampleFormat in_sample_fmt = AVSampleFormat(acodec_ctx->sample_fmt);
//
//  std::cout << "in sr: " << in_sample_rate << " in sf: " << in_sample_fmt << std::endl;
//  FFmpegUtil::AudioInfo inAudio(in_layout, in_sample_rate, in_channels,
//                                in_sample_fmt);
//
//  FFmpegUtil::AudioInfo outAudio = FFmpegUtil::ReSampler::getDefaultAudioInfo(in_sample_rate);
//  outAudio.sampleRate = inAudio.sampleRate;
//
//  FFmpegUtil::ReSampler reSampler(inAudio, outAudio);
//  reSampler.initx(inAudio, outAudio);
//
//
//
//  SDL_AudioSpec audio_spec;
//  SDL_AudioSpec spec;
//
//  //std::this_thread::sleep_for(std::chrono::milliseconds(500));
//
//  // set audio settings from codec info
//  audio_spec.freq = acodec_ctx->sample_rate;
//  audio_spec.format = AUDIO_S16SYS;
//  audio_spec.channels = acodec_ctx->channels;
//  audio_spec.samples = 1024;
//  audio_spec.callback = audio_callback;
//  audio_spec.userdata = &reSampler;
//
//  // open audio device
//  audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, &spec, 0);
//
//  // SDL_OpenAudioDevice returns a valid device ID that is > 0 on success or 0
//  // on failure
//  if (audioDeviceID == 0) {
//    std::string errMsg = "Failed to open audio device:";
//    errMsg += SDL_GetError();
//    std::cout << errMsg << std::endl;
//    throw std::runtime_error(errMsg);
//  }
//
//  SDL_PauseAudioDevice(audioDeviceID, 0);
//  std::cout << "waiting audio play..." << std::endl;
//}
//
//#undef main
//int main(int argc, char **argv) {
//  int ret;
//  AVPacket packet;
//  AVFrame *frame = av_frame_alloc();
//  AVFrame *filt_frame = av_frame_alloc();
//  packet.data = NULL;
//
//  if (!frame || !filt_frame) {
//    perror("Could not allocate frame");
//    exit(1);
//  }
//  //if (argc != 2) {
//  //  fprintf(stderr, "Usage: %s file | %s\n", argv[0], player);
//  //  exit(1);
//  //}
//
//  std::string file_name =
//    //  "D:/Download/Videos/LadyLiu/Trip.flv";  
//  //"D:/Study/Scala/VSWS/transcode/out/build/x64-Release/trip.mp3";
//  "D:/Study/Scala/VSWS/retream/out/build/x64-Release/recv.aac";
//
//
//  if ((ret = open_input_file(file_name.c_str())) < 0) return -1;  // goto end;
//  if ((ret = init_filters(filter_desc)) < 0) return -1;          // goto end;
//
//    // sdl play audio
//  //FFmpegUtil::ffmpeg_util ffmpeg_ctx(file_name);
//  //SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
//
//  //if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
//  //  SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to init SDL - %s\n !",
//  //               SDL_GetError);
//  //  return -1;
//  //}
//
//  //SDL_AudioDeviceID audioDeviceID;
//  //std::thread audio_thread(playAudio, ffmpeg_ctx, std::ref(audioDeviceID));
//  //audio_thread.join();
//  //std::this_thread::sleep_for(std::chrono::milliseconds(20000));
//  //return 0;
//
//  /* read all packets */
//  //initSwr(audio_stream_index);
//  while (1) {
//    if ((ret = av_read_frame(fmt_ctx, &packet)) < 0) break;
//
//    if (packet.stream_index == audio_stream_index) {
//      ret = avcodec_send_packet(dec_ctx, &packet);
//      std::cout << "pkt size: " << packet.size;
//      if (ret < 0) {
//        av_log(NULL, AV_LOG_ERROR,
//               "Error while sending a packet to the decoder\n");
//        break;
//      }
//
//      while (ret >= 0) {
//        ret = avcodec_receive_frame(dec_ctx, frame);
//        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//          break;
//        } else if (ret < 0) {
//          av_log(NULL, AV_LOG_ERROR,
//                 "Error while receiving a frame from the decoder\n");
//          goto end;
//        }
//
//        if (ret >= 0) {
//          AVFrame *frame_out = av_frame_alloc();
//
//          /* push the audio data from decoded frame into the filtergraph */
//          if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
//                                           0) < 0) {
//            av_log(NULL, AV_LOG_ERROR,
//                   "Error while feeding the audio filtergraph\n");
//            break;
//          }
//
//          /* pull filtered audio from the filtergraph */
//          while (1) {
//            ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
//            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
//            if (ret < 0) goto end;
//            //std::cout << "out_frame nb_sample: " << frame_out->data
//            //          << std::endl;
//            //if (0 != TransSample(filt_frame, frame_out, audio_stream_index)) {
//            //  av_log(NULL, AV_LOG_ERROR, "convert audio failed\n");
//            //}
//            print_frame(filt_frame);
//            av_frame_unref(filt_frame);
//          }
//          av_frame_unref(frame);
//          av_frame_free(&frame_out);
//        }
//      }
//    }
//    av_packet_unref(&packet);
//  }
//end:
//  avfilter_graph_free(&filter_graph);
//  avcodec_free_context(&dec_ctx);
//  avformat_close_input(&fmt_ctx);
//  av_frame_free(&frame);
//  av_frame_free(&filt_frame);
//
//  if (ret < 0 && ret != AVERROR_EOF) {
//    fprintf(stderr, "Error occurred: %d\n",ret);
//    exit(1);
//  }
//  //if (wav) {
//  //  wav_write_close(wav);
//  //}
//
//
//  exit(0);
//}