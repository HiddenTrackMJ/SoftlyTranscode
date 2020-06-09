#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include <iostream>

#include "fdk-aac/aacenc_lib.h"
#include "seeker/loggerApi.h"
#include "wav_reader.hpp"

class aac_encoder {
 private:
  int bitrate = 64000;
  int ch;
  const char *infile, *outfile;
  FILE *out;
  void *wav;
  int format, sample_rate, channels, bits_per_sample;
  int input_size;
  uint8_t *input_buf;
  int16_t *convert_buf;
  int aot = 2;
  int afterburner = 1;
  int eld_sbr = 0;
  int vbr = 0;
  HANDLE_AACENCODER handle;
  CHANNEL_MODE mode;
  AACENC_InfoStruct info = {0};
  int trans_mux = TT_MP4_ADTS; 

 public:
  int init(int in_aot, int in_trans);
  void encode_process();
  int encode_one_frame(uint8_t* output, int& out_size);
  void close();

};