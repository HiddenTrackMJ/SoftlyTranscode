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
#include <vector>
#include <map>

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

  AVRational time_base;

  codec_info() {
    sample_rate = 48000;
    channels = 2;
    channel_layout =
        av_get_default_channel_layout(channels);
    sample_fmt = AV_SAMPLE_FMT_FLTP;
    time_base = {1, sample_rate};
  }
};



class Mixer {
 public:
  struct src_filter {
    char args[512];

    std::string pad_name;

    const AVFilter *abuffersrc;

    AVFilterContext *buffersrc_ctx;

    codec_info src_codec_info;

    AVFilterInOut *outputs;

    src_filter::src_filter() {}

    void src_filter::init(int n) {
      abuffersrc = avfilter_get_by_name("abuffer");
      outputs = avfilter_inout_alloc();
      if (!outputs || !abuffersrc) {
        E_LOG("init filter input output error!!!");
      }
      if (abuffersrc) I_LOG("shit");
      std::string p = "in" + std::to_string(n);
      I_LOG("init src_filter: {}", p);
      pad_name = p;
    }
  };

  class filter_info {
   public:
    std::string filter_desc = "[in0][in1]amix=inputs=2[out]";
    //"aresample=48000,aformat=sample_fmts=fltp:channel_layouts=stereo";

    std::map<int32_t, std::shared_ptr<src_filter>> src_filter_map;

    AVFilterContext *buffersink_ctx;

    AVFilterContext *buffersrc_ctx;

    AVFilterContext *_buffersrc_ctx;

    AVFilterGraph *filter_graph;

    filter_info::filter_info() { }

    void filter_info::init(int n) {
      filter_desc = gen_filter_desc(n);
      I_LOG("init filter_desc: {}", filter_desc);
      int i = 0;
      for (i = 0; i < n; i++) {
        std::shared_ptr<src_filter> filter_ptr(new src_filter);
        filter_ptr->init(i);
        //buffersrc_map.insert(std::make_pair((int32_t)i, std::make_pair(std::move(buffer_ptr), std::move(filter_ptr))));
        src_filter_map.insert(std::make_pair((int32_t)i, std::move(filter_ptr)));
      }
    }
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

  static std::string gen_filter_desc(int n);

  int open_output_file(const char *filename);

  int init_filter(const char *filters_descr);

  int init_filters(int n);

  int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index);//向输出文件写入

  int recv_aac_mix(int port, std::string input_aac, std::string dst);

  int recv_aac_thread(std::string input_aac, pkt_list_info *pkt_list_ctx, int index); //监听线程，接收aac数据

  int process_thread(std::string dst_file); // 处理数据线程

  int open_thread(int port, std::string dst); //开启监听线程及处理数据线程

  void close_thread();
};