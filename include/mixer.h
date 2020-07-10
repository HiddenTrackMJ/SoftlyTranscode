/**
 * Author: Hidden Track
 * Date: 2020/07/07
 * Time: 10:37
 *
 * 混流器头文件
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

#include <chrono>
#include <iostream>
#include <list>
#include <string>

#include "FFmpegDecoder.h"
#include "adts_header.hpp"
#include "seeker/loggerApi.h"

#define BUF_SIZE_20K 2048000
#define BUF_SIZE_1K 1024000

using std::unique_ptr;

struct codec_info {
  int sample_rate;

  int channels;

  int64_t channel_layout;

  enum AVSampleFormat sample_fmt;
};



class Mixer {
 public:
  struct filter_info {
    char *filter_desc = "[in0][in1]amix=inputs=2[out]"; 
        //"aresample=48000,aformat=sample_fmts=fltp:channel_layouts=stereo";

    const char *player = "ffplay -f s16le -ar 8000 -ac 1 -";

    AVFormatContext *fmt_ctx = nullptr;

    AVCodecContext *dec_ctx;

    AVFormatContext *_fmt_ctx = nullptr;

    AVCodecContext *_dec_ctx;

    AVFilterContext *buffersink_ctx;

    AVFilterContext *buffersrc_ctx;

    AVFilterContext *_buffersrc_ctx;

    AVFilterGraph *filter_graph;

    int audio_stream_index = -1;

    int _audio_stream_index = -1;
  };

  struct pkt_list_info {
    std::list<std::pair<std::unique_ptr<uint8_t>, int>> pkt_list{};

    unique_ptr<std::mutex> pkt_list_mu = unique_ptr<std::mutex>(new std::mutex);

    unique_ptr<std::condition_variable> pkt_list_cv =
        unique_ptr<std::condition_variable>(new std::condition_variable);
  };

 private:
  AVFormatContext *ofmt_ctx;

  AVCodecContext *enc_ctx;  //输出文件的codec

  codec_info in_codec_info; //输入codec信息

  codec_info out_codec_info;  //输出codec信息

  filter_info filter;

  filter_info _filter;

  AVFrame *frame;

  bool stop;

  int pkt_count = 0;

  pkt_list_info pkt_list_ctx1;

  pkt_list_info pkt_list_ctx2;

 public:
  Mixer();

  ~Mixer();

  int open_input_file(const char *filename);

  int _open_input_file(const char *filename);

  int open_output_file(const char *filename);

  int init_filter(const char *filters_descr);

  int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index);

  int recv_aac_mix(int port, std::string input_aac, std::string dst);

  int recv_aac_thread(std::string input_aac, pkt_list_info *pkt_list_ctx,
                      int index); //监听线程，接收aac数据

  int process_thread(std::string dst_file); // 处理数据线程

  int open_thread(int port, std::string dst); //开启监听线程及处理数据线程

  void close_thread();
};