/**
 * Author: Hidden Track
 * Date: 2020/06/10
 * Time: 14:23
 *
 * aac×ªÂëhls/dash 
 *
 */

#pragma once
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
}
#include <stdio.h>
#include <string>
#include <iostream>

#include "FFmpegDecoder.h"
#include "adts_header.hpp"
#include "seeker/loggerApi.h"

#define BUF_SIZE_20K 2048000
#define BUF_SIZE_1K 1024000


class Muxer {
 private:
  AVFormatContext *ofmt_ctx;

  SwrContext *pSwrCtx;

  int in_sample_rate;

  int in_channels;

  int64_t in_channel_layout;

  enum AVSampleFormat in_sample_fmt;

  AVBitStreamFilterContext *aacbsfc;

  AVFrame *frame;

  AVPacket packet;  //= { .data = NULL, .size = 0 };


public:
  Muxer();

  ~Muxer();

  void initSwr(int audio_index);

  void setup_array(uint8_t *out[32], AVFrame *in_frame, enum AVSampleFormat format, int samples);

  int TransSample(AVFrame *in_frame, AVFrame *out_frame, int audio_index);

  int open_output_file(const char *filename);

  int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index);

  int flush_encoder(unsigned int stream_index);

  int mux(std::string input_aac, std::string dst);
};
