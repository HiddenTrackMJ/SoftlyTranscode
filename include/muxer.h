/**
 * Author: Hidden Track
 * Date: 2020/06/10
 * Time: 14:23
 *
 * aac转码hls/dash 
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
#include <chrono>
#include <list>


#include "FFmpegDecoder.h"
#include "adts_header.hpp"
#include "seeker/loggerApi.h"

#define BUF_SIZE_20K 2048000
#define BUF_SIZE_1K 1024000

using std::unique_ptr;

class Muxer {
 private:
  AVFormatContext *ofmt_ctx;

  AVCodecContext *enc_ctx;  //输出文件的codec

  SwrContext *pSwrCtx;

  int in_sample_rate;

  int in_channels;

  int64_t in_channel_layout;

  enum AVSampleFormat in_sample_fmt;

  AVBitStreamFilterContext *aacbsfc;

  AVFrame *frame;

  AVPacket packet;  //= { .data = NULL, .size = 0 };

  bool stop;

  int pkt_count = 0;

  std::list<std::pair<std::unique_ptr<uint8_t>, int>> pkt_list{};

  unique_ptr<std::mutex> pkt_list_mu = unique_ptr<std::mutex>(new std::mutex);

  unique_ptr<std::condition_variable> pkt_list_cv =
      unique_ptr<std::condition_variable>(new std::condition_variable);


public:
  Muxer();

  ~Muxer();

  void initSwr(int audio_index);

  void setup_array(uint8_t *out[32], AVFrame *in_frame, enum AVSampleFormat format, int samples);

  int TransSample(AVFrame *in_frame, AVFrame *out_frame);

  int ReSample(AVFrame *in_frame, AVFrame *out_frame);

  int open_output_file(const char *filename);

  int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index);

  int flush_encoder(unsigned int stream_index);

  int mux(std::string input_aac, std::string dst);

  int recv_aac_mux(int port, std::string input_aac, std::string dst);

  int recv_aac_thread(int port);

  int process_thread(std::string dst_file);

  int open_thread(int port, std::string dst);

  void close_thread();
};
