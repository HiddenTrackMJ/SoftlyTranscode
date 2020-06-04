#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include <iostream>
#include "seeker/loggerApi.h"


extern "C" {
#include "fdk-aac/aacdecoder_lib.h"
#include "fdk-aac/aacenc_lib.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
};

#include "wav_writer.hpp"

#include "fdk-aac/aacdecoder_lib.h"
using namespace std;

int main3() {
  HANDLE_AACDECODER handle;
  handle = aacDecoder_Open(TT_MP4_LATM_MCP0, 1);  // TT_MP4_LATM_MCP0 TT_MP4_RAW
  int conceal_method = 2;                   // 0 muting 1 noise 2 interpolation
  // UCHAR eld_conf[] = { 0xF8, 0xE8, 0x50, 0x00 };//ste eld 44.1k
  UCHAR ld_conf[] = {0xBA, 0x88, 0x00, 0x00};  // mono ld 32k 0xF8, 0xF0, 0x20, 0x00
                                                  // 0xBA88, 0x0000
                                                  // 0xBA, 0x88, 0x00, 0x00

  //23 5 1 
  //1011 1010 1000 1000

  UCHAR* conf[] = {ld_conf};
  static UINT conf_len = sizeof(ld_conf);
  AAC_DECODER_ERROR err;
  AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(handle, conf, &conf_len);
  if (err > 0) cout << "conf err:" << err << endl;
  aacDecoder_SetParam(handle, AAC_CONCEAL_METHOD, conceal_method);
  aacDecoder_SetParam(handle, AAC_PCM_MAX_OUTPUT_CHANNELS, 1);  // MONO:1
  aacDecoder_SetParam(handle, AAC_PCM_MIN_OUTPUT_CHANNELS, 1);  // MONO:1

  const char *lengthfile, *aacfile, *pcmfile;
  lengthfile = "length.txt";
  aacfile = "encode1.aac";
  pcmfile = "decoder.pcm";
  unsigned int size = 1024;
  unsigned int valid;
  FILE* lengthHandle;
  FILE* aacHandle;
  FILE* pcmHandle;
  void* wav = NULL;
  lengthHandle = fopen(lengthfile, "rb");
  aacHandle = fopen(aacfile, "rb");
  pcmHandle = fopen(pcmfile, "wb");
  unsigned char* data = (unsigned char*)malloc(size);
  unsigned int decsize = 8 * 2048 * sizeof(INT_PCM);
  unsigned char* decdata = (unsigned char*)malloc(decsize);

  int frameCnt = 0;
  do {
    int ll = fread(&size, 1, 2, lengthHandle);
    //if (ll < 1) break;
    //printf("size %d\n", size);
   // size = 85;
    fread(data, 1, size, aacHandle);
    valid = size;
    err = aacDecoder_Fill(handle, &data, &size, &valid);
    if (err > 0) cout << "fill err:" << err << endl;
    // aacDecoder_SetParam(handle, AAC_TPDEC_CLEAR_BUFFER, 1);

    err = aacDecoder_DecodeFrame(handle, (INT_PCM*)decdata,
                                 decsize / sizeof(INT_PCM), 0);
    if (err > 0) cout << "dec err:" << err << endl;
   // if (err != 0) break;
    CStreamInfo* info = aacDecoder_GetStreamInfo(handle);
    /* cout<<"channels"<<info->numChannels<<endl;
     cout<<"sampleRate"<<info->sampleRate<<endl;
     cout<<"frameSize"<<info->frameSize<<endl;
     cout<<"decsize"<<decsize<<endl;
     cout<<"decdata"<<decdata[0]<<endl;*/

    if (!wav) {
      wav = wav_write_open("sss.wav", 48000, 16, 2);
      if (!wav) {
        D_LOG("here6...");
        break;
      }
    }
      
    wav_write_data(wav, (unsigned char*)&decdata[0], info->frameSize * 2);

    fwrite(decdata, 1, info->frameSize * 2, pcmHandle);
    frameCnt++;
  } while (true);
  cout << "frame count:" << frameCnt << endl;
  fflush(pcmHandle);
  fclose(lengthHandle);
  fclose(aacHandle);
  fclose(pcmHandle);
  aacDecoder_Close(handle);
  return 0;
}