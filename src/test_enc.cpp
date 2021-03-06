
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#include <iostream>
#include "fdk-aac/aacenc_lib.h"
#include "seeker/loggerApi.h"

#include "wav_reader.hpp"
using namespace std;

int main2(int argc, char *argv[]) {
  int bitrate;
  const char *infile, *outfile, *lengthfile;
  FILE *out;
  FILE *lengthHandle;
  void *wav;
  int format, sample_rate, channels, bits_per_sample;
  int input_size;
  uint8_t *input_buf;
  int16_t *convert_buf;
  int aot;
  int afterburner = 1;
  int eld_sbr = 0;
  int vbr = 0;
  HANDLE_AACENCODER handle;
  CHANNEL_MODE mode;
  AACENC_InfoStruct info = {0};
  bitrate = 64000;               // target bitrate.
  aot = 23;                      // type eld 39 ld 23
  infile = "1.wav";  // original file
  outfile = "encode1.aac";        // encode aac
  lengthfile = "length.txt";     // record frame length

  wav = wav_read_open(infile);
  if (!wav) {
    fprintf(stderr, "Unable to open wav file %s\n", infile);
    return 1;
  }
  if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample,
                      NULL)) {
    fprintf(stderr, "Bad wav file %s\n", infile);
    return 1;
  }
  if (format != 1) {
    fprintf(stderr, "Unsupported WAV format %d\n", format);
    return 1;
  }
  if (bits_per_sample != 16) {
    fprintf(stderr, "Unsupported WAV sample depth %d\n", bits_per_sample);
    return 1;
  }
  switch (channels) {
    case 1:
      mode = MODE_1;
      break;
    case 2:
      mode = MODE_2;
      break;
    case 3:
      mode = MODE_1_2;
      break;
    case 4:
      mode = MODE_1_2_1;
      break;
    case 5:
      mode = MODE_1_2_2;
      break;
    case 6:
      mode = MODE_1_2_2_1;
      break;
    default:
      fprintf(stderr, "Unsupported WAV channels %d\n", channels);
      return 1;
  }

  int trans_mux = TT_MP4_RAW;  // TT_MP4_LATM_MCP0//TT_MP4_RAW; TT_MP4_ADTS

   I_LOG("aot: {}, transport type: {}, mode: {}", aot, trans_mux, mode);

  if (aacEncOpen(&handle, 0, channels) != AACENC_OK) {
    fprintf(stderr, "Unable to open encoder\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_AOT, aot) != AACENC_OK) {
    fprintf(stderr, "Unable to set the AOT\n");
    return 1;
  }
  if (aot == 39 && eld_sbr) {
    if (aacEncoder_SetParam(handle, AACENC_SBR_MODE, 1) != AACENC_OK) {
      fprintf(stderr, "Unable to set SBR mode for ELD\n");
      return 1;
    }
  }
  if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, sample_rate) !=
      AACENC_OK) {
    fprintf(stderr, "Unable to set the AOT\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK) {
    fprintf(stderr, "Unable to set the channel mode\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
    fprintf(stderr, "Unable to set the wav channel order\n");
    return 1;
  }
  if (vbr) {
    if (aacEncoder_SetParam(handle, AACENC_BITRATEMODE, vbr) != AACENC_OK) {
      fprintf(stderr, "Unable to set the VBR bitrate mode\n");
      return 1;
    }
  } else {
    if (aacEncoder_SetParam(handle, AACENC_BITRATE, bitrate) != AACENC_OK) {
      fprintf(stderr, "Unable to set the bitrate\n");
      return 1;
    }
  }
  if (aacEncoder_SetParam(handle, AACENC_TRANSMUX,
                          trans_mux /*TT_MP4_ADTS TT_MP4_RAW*/) != AACENC_OK) {
    fprintf(stderr, "Unable to set the ADTS transmux\n");
    return 1;
  }  // TT_MP4_RAW TT_MP4_LATM_MCP1 TT_MP4_LATM_MCP0 TT_MP4_LOAS is ok.
  if (aacEncoder_SetParam(handle, AACENC_AFTERBURNER, afterburner) !=
      AACENC_OK) {
    fprintf(stderr, "Unable to set the afterburner mode\n");
    return 1;
  }
  if (aacEncEncode(handle, NULL, NULL, NULL, NULL) != AACENC_OK) {
    fprintf(stderr, "Unable to initialize the encoder\n");
    return 1;
  }
  if (aacEncInfo(handle, &info) != AACENC_OK) {
    fprintf(stderr, "Unable to get the encoder info\n");
    return 1;
  }

  out = fopen(outfile, "wb");
  lengthHandle = fopen(lengthfile, "wb");
  if (!out) {
    perror(outfile);
    return 1;
  }
  if (!lengthHandle) {
    perror(lengthfile);
    return 1;
  }

  input_size = channels * 2 * info.frameLength;
  input_buf = (uint8_t *)malloc(input_size);
  convert_buf = (int16_t *)malloc(input_size);
  char lengthStr[2];
  cout << "info.frameLength: " << info.frameLength << endl;
  cout << "sample_rate: " << sample_rate << endl;
  cout << "aot: " << aot << endl;
  cout << "mode: " << mode << endl;
  cout << "bitrate: " << bitrate << endl;
  cout << "afterburner: " << afterburner << endl;

  int cnt = 0;
  while (1) {
    cnt++;
    AACENC_BufDesc in_buf = {0}, out_buf = {0};
    AACENC_InArgs in_args = {0};
    AACENC_OutArgs out_args = {0};
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size;
    int read, i;
    void *in_ptr, *out_ptr;
    uint8_t outbuf[20480];
    AACENC_ERROR err;

    read = wav_read_data(wav, input_buf, input_size);
    for (i = 0; i < read / 2; i++) {
      const uint8_t *in = &input_buf[2 * i];
      convert_buf[i] = in[0] | (in[1] << 8);
    }
    in_ptr = convert_buf;
    in_size = read;
    in_elem_size = 2;

    in_args.numInSamples = read <= 0 ? -1 : read / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    out_ptr = outbuf;
    out_size = sizeof(outbuf);
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_size;
    out_buf.bufElSizes = &out_elem_size;

    if ((err = aacEncEncode(handle, &in_buf, &out_buf, &in_args, &out_args)) !=
        AACENC_OK) {
      if (err == AACENC_ENCODE_EOF) break;
      fprintf(stderr, "Encoding failed\n");
      return 1;
    }
    if (out_args.numOutBytes == 0) continue;
    //cout << "size " << out_args.numOutBytes << endl;
    sprintf(lengthStr, "%x", out_args.numOutBytes);
    fwrite(outbuf, 1, out_args.numOutBytes, out);
    fwrite(&(out_args.numOutBytes), 1, 2, lengthHandle);
  }
  cout << "frame count:" << cnt << endl;
  free(input_buf);
  free(convert_buf);
  fclose(out);
  fclose(lengthHandle);
  wav_read_close(wav);
  aacEncClose(&handle);
  return 0;
}


int main3(int argc, char *argv[]) {
  int bitrate = 64000;
  int ch;
  const char *infile, *outfile;
  FILE *out;
  void *wav;
  int format, sample_rate, channels, bits_per_sample;
  int input_size;
  uint8_t *input_buf;
  int16_t *convert_buf;
  int aot = 23;
  int afterburner = 1;
  int eld_sbr = 0;
  int vbr = 0;
  HANDLE_AACENCODER handle;
  CHANNEL_MODE mode;
  AACENC_InfoStruct info = {0};

  bitrate = 64000;  // target bitrate.
  aot = 2;         // type eld 39 ld 23

  infile = "1.wav";        // original file
  outfile = "encode.aac";  // encode aac
  //lengthfile = "length.txt"; 

  wav = wav_read_open(infile);
  if (!wav) {
    fprintf(stderr, "Unable to open wav file %s\n", infile);
    return 1;
  }
  if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample,
                      NULL)) {
    fprintf(stderr, "Bad wav file %s\n", infile);
    return 1;
  }
  if (format != 1) {
    fprintf(stderr, "Unsupported WAV format %d\n", format);
    return 1;
  }
  if (bits_per_sample != 16) {
    fprintf(stderr, "Unsupported WAV sample depth %d\n", bits_per_sample);
    return 1;
  }
  switch (channels) {
    case 1:
      mode = MODE_1;
      break;
    case 2:
      mode = MODE_2;
      break;
    case 3:
      mode = MODE_1_2;
      break;
    case 4:
      mode = MODE_1_2_1;
      break;
    case 5:
      mode = MODE_1_2_2;
      break;
    case 6:
      mode = MODE_1_2_2_1;
      break;
    default:
      fprintf(stderr, "Unsupported WAV channels %d\n", channels);
      return 1;
  }

  int trans_mux = TT_MP4_ADTS; //TT_MP4_LATM_MCP0
  
   I_LOG("aot: {}, transport type: {}, mode: {}", aot, trans_mux, mode);

   I_LOG("s_r: {}, channel: {}, b/s: {}", sample_rate, channels,
        bits_per_sample);

  if (aacEncOpen(&handle, 0, channels) != AACENC_OK) {
    fprintf(stderr, "Unable to open encoder\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_AOT, aot) != AACENC_OK) {
    fprintf(stderr, "Unable to set the AOT\n");
    return 1;
  }
  if (aot == 39 && eld_sbr) {
    if (aacEncoder_SetParam(handle, AACENC_SBR_MODE, 1) != AACENC_OK) {
      fprintf(stderr, "Unable to set SBR mode for ELD\n");
      return 1;
    }
  }
  if (aacEncoder_SetParam(handle, AACENC_SAMPLERATE, sample_rate) !=
      AACENC_OK) {
    fprintf(stderr, "Unable to set the AOT\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_CHANNELMODE, mode) != AACENC_OK) {
    fprintf(stderr, "Unable to set the channel mode\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
    fprintf(stderr, "Unable to set the wav channel order\n");
    return 1;
  }
  if (vbr) {
    if (aacEncoder_SetParam(handle, AACENC_BITRATEMODE, vbr) != AACENC_OK) {
      fprintf(stderr, "Unable to set the VBR bitrate mode\n");
      return 1;
    }
  } else {
    if (aacEncoder_SetParam(handle, AACENC_BITRATE, bitrate) != AACENC_OK) {
      fprintf(stderr, "Unable to set the bitrate\n");
      return 1;
    }
  }
  if (aacEncoder_SetParam(handle, AACENC_TRANSMUX, trans_mux) != AACENC_OK) {
    fprintf(stderr, "Unable to set the transmux\n");
    return 1;
  }
  if (aacEncoder_SetParam(handle, AACENC_AFTERBURNER, afterburner) !=
      AACENC_OK) {
    fprintf(stderr, "Unable to set the afterburner mode\n");
    return 1;
  }
  if (aacEncEncode(handle, NULL, NULL, NULL, NULL) != AACENC_OK) {
    fprintf(stderr, "Unable to initialize the encoder\n");
    return 1;
  }
  if (aacEncInfo(handle, &info) != AACENC_OK) {
    fprintf(stderr, "Unable to get the encoder info\n");
    return 1;
  }

  out = fopen(outfile, "wb");
  if (!out) {
    perror(outfile);
    return 1;
  }

  input_size = 4096;  // channels * 2 * info.frameLength;
  input_buf = (uint8_t *)malloc(input_size);
  convert_buf = (int16_t *)malloc(input_size);
  int j = 0;
  while (1) {
    AACENC_BufDesc in_buf = {0}, out_buf = {0};
    AACENC_InArgs in_args = {0};
    AACENC_OutArgs out_args = {0};
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size;
    int read, i;
    void *in_ptr, *out_ptr;
    uint8_t outbuf[20480];
    AACENC_ERROR err;

    read = wav_read_data(wav, input_buf, input_size);
    I_LOG("{} th read, size: {}", ++j, input_size);
    for (i = 0; i < read / 2; i++) {
      const uint8_t *in = &input_buf[2 * i];
      convert_buf[i] = in[0] | (in[1] << 8);
    }
    in_ptr = convert_buf;
    in_size = read;
    in_elem_size = 2;

    in_args.numInSamples = read <= 0 ? -1 : read / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    out_ptr = outbuf;
    out_size = sizeof(outbuf);
    out_elem_size = 1;
    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_size;
    out_buf.bufElSizes = &out_elem_size;

    if ((err = aacEncEncode(handle, &in_buf, &out_buf, &in_args, &out_args)) !=
        AACENC_OK) {
      if (err == AACENC_ENCODE_EOF) break;
      fprintf(stderr, "Encoding failed\n");
      return 1;
    }
    if (out_args.numOutBytes == 0) continue;
    uint8_t *output = outbuf;
    int output_size = out_args.numOutBytes;
   /* std::cout << j << " th  " << "111 out_put: " << *output << ", out_size: " << output_size
              << std::endl;*/
    fwrite(outbuf, 1, out_args.numOutBytes, out);
  }
  free(input_buf);
  free(convert_buf);
  fclose(out);
  wav_read_close(wav);
  aacEncClose(&handle);

  return 0;
}