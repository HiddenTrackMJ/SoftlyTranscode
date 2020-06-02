#pragma once
// 此类设计输入H264码流，输出BGR buffer 数据
#include <stdio.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "fdk-aac/aacdecoder_lib.h"
};

class ImageMat {
 public:
  ImageMat() {}
  ~ImageMat() {
    if (data) {
      delete data;
    }
  }
  int Create(int width, int height, int channel) {
    if (width * height * channel > this->width * this->height * this->channel) {
      if (this->data) {
        delete this->data;
        this->data = new unsigned char[width * height * channel];
      } else {
        this->data = new unsigned char[width * height * channel];
      }
    }

    this->width = width;
    this->height = height;
    this->channel = channel;

    return 0;
  }

  int width = 0;
  int height = 0;
  int channel = 0;
  unsigned char *data = NULL;
};
class FFmpegDecoder {
 public:
  FFmpegDecoder();
  ~FFmpegDecoder();

  // ImageMat 最后一个参数最好传入一个vector<ImageMat >
  // 因为有时候传入一帧会输出多帧，此处如果输出多帧，只取最后一帧，可以自己改善
  int InputData(
      unsigned char *inputData, int inputLen,
      ImageMat &mImageMat);  // 返回值0 有输出， 返回值<0
                             // 没有输出数据，刷新数据时候inputLen = 0;

 private:
  int decode(AVPacket *mpkt, ImageMat &mImageMat);

  const AVCodec *codec;
  AVCodecParserContext *parser = NULL;
  AVCodecContext *c = NULL;
  AVFrame *frame;
  AVPacket *pkt;

  struct SwsContext *sws_ctx = NULL;
};
