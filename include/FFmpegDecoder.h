/**
 * Author: Hidden Track
 * Date: 2020/06/05
 * Time: 11:37
 *
 * 解码器头文件
 *
 */

#pragma once
#include <stdio.h>
#include <atomic>

extern "C" {
#include "fdk-aac/aacdecoder_lib.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
};



class FFmpegDecoder {
 public:
  FFmpegDecoder();
  ~FFmpegDecoder();

  int InputData(
      unsigned char *inputData, int inputLen);  // 返回值0 有输出， 返回值<0
                             // 没有输出数据，刷新数据时候inputLen = 0;

  AVPacket *getPkt();
  AVFrame *getFrame();
  void reset();

 private:
  int decode(AVPacket *mpkt);
  std::atomic<int> ts_gen{0};
  const AVCodec *codec;
  AVCodecParserContext *parser = nullptr;
  AVCodecContext *c = nullptr;
  AVFrame *frame;
  AVFrame *out_frame;
  AVPacket *pkt;

  struct SwsContext *sws_ctx = nullptr;
};
