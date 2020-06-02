#pragma once
// �����������H264���������BGR buffer ����
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

  // ImageMat ���һ��������ô���һ��vector<ImageMat >
  // ��Ϊ��ʱ����һ֡�������֡���˴���������֡��ֻȡ���һ֡�������Լ�����
  int InputData(
      unsigned char *inputData, int inputLen,
      ImageMat &mImageMat);  // ����ֵ0 ������� ����ֵ<0
                             // û��������ݣ�ˢ������ʱ��inputLen = 0;

 private:
  int decode(AVPacket *mpkt, ImageMat &mImageMat);

  const AVCodec *codec;
  AVCodecParserContext *parser = NULL;
  AVCodecContext *c = NULL;
  AVFrame *frame;
  AVPacket *pkt;

  struct SwsContext *sws_ctx = NULL;
};
