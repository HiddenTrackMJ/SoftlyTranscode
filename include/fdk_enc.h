/**
 * Author: Hidden Track
 * Date: 2020/06/02
 * Time: 20:23
 *
 * 编码器头文件
 *
 */
#pragma once

#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"

#include "fdk-aac/aacdecoder_lib.h"
#include "fdk-aac/aacenc_lib.h"
};

class aacenc_t {
 public:
  // the encoder handler.
  HANDLE_AACENCODER enc;

  // encoder info.
  int frame_size;

  // user specified params.
  int aot;
  int channels;
  int sample_rate;
  int bitrate;
};

class AacEncoder {
 public:
  AacEncoder();
  ~AacEncoder();

  int aacenc_init(int aot, int channels, int sample_rate, int bitrate);
  void aacenc_close();
  int aacenc_encode(char *pcm, int nb_pcm, int nb_samples, char *aac,
                    int &pnb_aac);
  int aacenc_frame_size();
  int aacenc_max_output_buffer_size();

 private:
  aacenc_t _h;
};

//inline int encode_test(std::string input, std::string output);
