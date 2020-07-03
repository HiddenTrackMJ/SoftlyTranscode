/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2012 Clément Bœsch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for audio decoding and filtering
 * @example filtering_audio.c
 */

extern "C" {
    #include "SDL.h"
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}
//#include <unistd.h>

#include <string>
#include <iostream>
#include <tuple>
#include "FFmpegUtil.h"
#include "wav_writer.hpp"
#include "wav_reader.hpp"

static const char *filter_descr =
    "aresample=8000,aformat=sample_fmts=s16:channel_layouts=mono";
static const char *player = "ffplay -f s16le -ar 8000 -ac 1 -";

static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int audio_stream_index = -1;
void *wav = NULL;
std::string output = "./test.wav";
FILE *file = fopen("tmp.pcm", "ab");


static int open_input_file(const char *filename) {
  int ret;
  AVCodec *dec;

  if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }

  /* select the audio stream */
  ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR,
           "Cannot find an audio stream in the input file\n");
    return ret;
  }
  audio_stream_index = ret;

  /* create decoding context */
  dec_ctx = avcodec_alloc_context3(dec);
  if (!dec_ctx) return AVERROR(ENOMEM);
  avcodec_parameters_to_context(dec_ctx,
                                fmt_ctx->streams[audio_stream_index]->codecpar);

  /* init the audio decoder */
  if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
    return ret;
  }

  return 0;
}

static int init_filters(const char *filters_descr) {
  char args[512];
  int ret = 0;
  const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");
  const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs = avfilter_inout_alloc();
  static const enum AVSampleFormat out_sample_fmts[] = {dec_ctx->sample_fmt, (enum AVSampleFormat)-1};//{AV_SAMPLE_FMT_S16}
  static const int64_t out_channel_layouts[] = {dec_ctx->channel_layout, -1};//{AV_CH_LAYOUT_MONO, -1}
  static const int out_sample_rates[] = {dec_ctx->sample_rate, -1};
  std::cout << "sample_fmt: " << av_get_sample_fmt_name(dec_ctx->sample_fmt)
            << "channel_layout: " << dec_ctx->channel_layout
            << "sample_rates: " << out_sample_rates[0]
            << "bits_per_raw_sample: " << dec_ctx->bits_per_raw_sample << std::endl;
 
  const AVFilterLink *outlink;
  AVRational time_base = fmt_ctx->streams[audio_stream_index]->time_base;

  filter_graph = avfilter_graph_alloc();
  if (!outputs || !inputs || !filter_graph) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  /* buffer audio source: the decoded frames from the decoder will be inserted
   * here. */
  if (!dec_ctx->channel_layout)
    dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
  snprintf(
      args, sizeof(args),
      "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
      time_base.num, time_base.den, dec_ctx->sample_rate,
      av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);

  printf(
      args, sizeof(args),
      "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%\n" PRIx64,
      time_base.num, time_base.den, dec_ctx->sample_rate,
      av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);

  ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in", args,
                                     NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
    goto end;
  }

  /* buffer audio sink: to terminate the filter chain. */
  ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL,
                                     NULL, filter_graph);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
    goto end;
  }

  ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
                            AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
    goto end;
  }

  ret = av_opt_set_int_list(buffersink_ctx, "channel_layouts",
                            out_channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
    goto end;
  }

  ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", out_sample_rates,
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
  outputs->name = av_strdup("in");
  outputs->filter_ctx = buffersrc_ctx;
  outputs->pad_idx = 0;
  outputs->next = NULL;

  /*
   * The buffer sink input must be connected to the output pad of
   * the last filter described by filters_descr; since the last
   * filter output label is not specified, it is set to "out" by
   * default.
   */
  inputs->name = av_strdup("out");
  inputs->filter_ctx = buffersink_ctx;
  inputs->pad_idx = 0;
  inputs->next = NULL;

  if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr, &inputs,
                                      &outputs, NULL)) < 0)
    goto end;

  if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) goto end;

  /* Print summary of the sink buffer
   * Note: args buffer is reused to store channel layout string */
  outlink = buffersink_ctx->inputs[0];
  av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
  av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
         (int)outlink->sample_rate,
         (char *)av_x_if_null(
             av_get_sample_fmt_name((enum AVSampleFormat)outlink->format), "?"),
         args);

end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);

  return ret;
}


static void print_frame(const AVFrame *frame) {
  const int n = frame->nb_samples *
                av_get_channel_layout_nb_channels(frame->channel_layout);
  //std::cout << "print_frame" << std::endl;
  const uint16_t *p = (uint16_t *)frame->data[0];
  const uint16_t *p_end = p + n;

  
  //while (p < p_end) {
  //  fputc(*p & 0xff, stdout);
  //  fputc(*p >> 8 & 0xff, stdout);
  //  p++;
  //}
  //fflush(stdout);
 
  //if (NULL == file) {
  //  perror("fopen tmp.mp3 error\n");
  //  return;
  //} else {
  //  perror("fopen tmp.aac successful\n");
  //}
  //fwrite(frame->data[0], n , 1, file);
 

  if (!wav) {
    wav = wav_write_open(output.c_str(), dec_ctx->sample_rate,
                         16,
                         dec_ctx->channels);
  }

  wav_write_data(wav, (unsigned char *)&frame->data[0], n * 2);
  
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
  using namespace std;
  //AudioData *audioData = (AudioData *)userdata;
  FFmpegUtil::ReSampler *reSampler = (FFmpegUtil::ReSampler *)userdata;

  static uint8_t *outBuffer = nullptr;
  static int outBufferSize = 0;
  static AVFrame *aFrame = av_frame_alloc();
  static AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  int ret = 0;
  av_init_packet(packet);
  //AVFormatContext *fmt_ctxd = fmt_ctx;
  AVCodecContext *codec_ctx = dec_ctx;

  while (true) {
    while (1) {
      ret = av_read_frame(fmt_ctx, packet);
      if (ret < 0) {
        cout << "read frame error" << endl;
      }
      if (packet->stream_index == audio_stream_index) break;
    }
    {
      ret = avcodec_send_packet(codec_ctx, packet);
      if (ret >= 0) {
        av_packet_unref(packet);
        int ret = avcodec_receive_frame(codec_ctx, aFrame);
        if (ret >= 0) {
          ret = 2;
          break;
        } else if (ret == AVERROR(EAGAIN)) {
          continue;
        } else {
          cout << " Failed to avcodec_receive_frame " << endl;
        }
      } else if (ret == AVERROR(EAGAIN)) {
        // buff full, can not decode anymore, do nothing.
      } else {
        cout << "Failed to avcodec_send_packet" << endl;
      }
    }
  }

  int outDataSize = -1;
  int outSamples = -1;

  if (outBuffer == nullptr) {
    outBufferSize = reSampler->allocDataBuf(&outBuffer, aFrame->nb_samples);
  } else {
    memset(outBuffer, 0, outBufferSize);
  }

  std::tie(outSamples, outDataSize) =
      reSampler->reSample(outBuffer, outBufferSize, aFrame);

  if (outDataSize != len) {
    cout << "WARNING: outDataSize[" << outDataSize << "] != len[" << len << "]"
         << endl;
  }

  std::memcpy(stream, outBuffer, outDataSize);
}

void playAudio(FFmpegUtil::ffmpeg_util f, SDL_AudioDeviceID &audioDeviceID) {
  // for audio play
  //auto fmt_ctx = f.get_fmt_ctx();
  auto acodec_ctx = dec_ctx;  // f.get_acodec_ctx();
  int64_t in_layout = acodec_ctx->channel_layout;
  int in_channels = acodec_ctx->channels;
  int in_sample_rate = acodec_ctx->sample_rate;
  AVSampleFormat in_sample_fmt = AVSampleFormat(acodec_ctx->sample_fmt);

  std::cout << "in sr: " << in_sample_rate << " in sf: " << in_sample_fmt << std::endl;
  FFmpegUtil::AudioInfo inAudio(in_layout, in_sample_rate, in_channels,
                                in_sample_fmt);

  FFmpegUtil::AudioInfo outAudio = FFmpegUtil::ReSampler::getDefaultAudioInfo(in_sample_rate);
  outAudio.sampleRate = inAudio.sampleRate;

  FFmpegUtil::ReSampler reSampler(inAudio, outAudio);



  SDL_AudioSpec audio_spec;
  SDL_AudioSpec spec;

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // set audio settings from codec info
  audio_spec.freq = acodec_ctx->sample_rate;
  audio_spec.format = AUDIO_S16SYS;
  audio_spec.channels = acodec_ctx->channels;
  audio_spec.samples = 1024;
  audio_spec.callback = audio_callback;
  audio_spec.userdata = &reSampler;

  // open audio device
  audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &audio_spec, &spec, 0);

  // SDL_OpenAudioDevice returns a valid device ID that is > 0 on success or 0
  // on failure
  if (audioDeviceID == 0) {
    std::string errMsg = "Failed to open audio device:";
    errMsg += SDL_GetError();
    std::cout << errMsg << std::endl;
    throw std::runtime_error(errMsg);
  }

  SDL_PauseAudioDevice(audioDeviceID, 0);
  std::cout << "waiting audio play..." << std::endl;
}

#undef main
int main(int argc, char **argv) {
  int ret;
  AVPacket packet;
  AVFrame *frame = av_frame_alloc();
  AVFrame *filt_frame = av_frame_alloc();

  if (!frame || !filt_frame) {
    perror("Could not allocate frame");
    exit(1);
  }
  //if (argc != 2) {
  //  fprintf(stderr, "Usage: %s file | %s\n", argv[0], player);
  //  exit(1);
  //}

  std::string file_name = "D:/Study/Scala/VSWS/retream/out/build/x64-Release/recv.aac";


  if ((ret = open_input_file(file_name.c_str())) < 0) goto end;
  if ((ret = init_filters(filter_descr)) < 0) goto end;

    // sdl play audio
  //FFmpegUtil::ffmpeg_util ffmpeg_ctx(file_name);
  //SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);

  //if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
  //  SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to init SDL - %s\n !",
  //               SDL_GetError);
  //  return -1;
  //}

  //SDL_AudioDeviceID audioDeviceID;
  //playAudio(ffmpeg_ctx, audioDeviceID);

  /* read all packets */
  while (1) {
    if ((ret = av_read_frame(fmt_ctx, &packet)) < 0) break;

    if (packet.stream_index == audio_stream_index) {
      ret = avcodec_send_packet(dec_ctx, &packet);
      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Error while sending a packet to the decoder\n");
        break;
      }

      while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        } else if (ret < 0) {
          av_log(NULL, AV_LOG_ERROR,
                 "Error while receiving a frame from the decoder\n");
          goto end;
        }

        if (ret >= 0) {
          /* push the audio data from decoded frame into the filtergraph */
          if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame,
                                           AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Error while feeding the audio filtergraph\n");
            break;
          }

          /* pull filtered audio from the filtergraph */
          while (1) {
            ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) goto end;
            print_frame(frame);
            av_frame_unref(filt_frame);
          }
          av_frame_unref(frame);
        }
      }
    }
    av_packet_unref(&packet);
  }
end:
  avfilter_graph_free(&filter_graph);
  avcodec_free_context(&dec_ctx);
  avformat_close_input(&fmt_ctx);
  av_frame_free(&frame);
  av_frame_free(&filt_frame);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "Error occurred: %d\n",ret);
    exit(1);
  }
  if (wav) {
    wav_write_close(wav);
  }

  fclose(file);
  exit(0);
}