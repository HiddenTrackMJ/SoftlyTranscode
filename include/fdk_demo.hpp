#include "seeker/loggerApi.h"
#include <fstream>
#include <iostream>
#include <vector>
#include "wav_writer.hpp"
#include "wav_reader.hpp"
#include "fdk_dec.h"
#include "fdk_enc.h"

inline int encode_test(std::string input, std::string output) {
  I_LOG("Start encoding...");

  const int bitrate = 128000;

  AacEncoder fdkaac_enc;

  std::ofstream out_aac(output.c_str(), std::ios::binary);
  int format, sample_rate = 32000, channels = 1, bits_per_sample = 14400;

  void *wav = wav_read_open(input.c_str());
  if (!wav) {
    D_LOG("init failed here1...");
    return 1;
  }
  if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample,
                      NULL)) {
    D_LOG("init failed here2...");
    exit(1);
    return 1;
  }
  if (format != 1) {
    D_LOG("init failed here3...");
    return 1;
  }
  if (bits_per_sample != 16) {
    D_LOG("init failed here4...");
    return 1;
  }

  I_LOG("s_r: {}, channel: {}, b/s: {}", sample_rate, channels,
        bits_per_sample);

  if (fdkaac_enc.aacenc_init(23, channels, sample_rate, bitrate) != AACENC_OK) {
    D_LOG("init failed here5...");
    return -1;
  }

  I_LOG("format: {}", format);
  I_LOG("channels: {}", channels);
  I_LOG("sample_rate: {}", sample_rate);

  int pcmSize = channels * 2 * fdkaac_enc.aacenc_frame_size();
  std::vector<char> pcm_buf(pcmSize, 0);

  int nbSamples = fdkaac_enc.aacenc_frame_size();

  int nbAac = fdkaac_enc.aacenc_max_output_buffer_size();
  std::vector<char> aac_buf(nbAac, 0);

  int err = 0;
  while (1) {
    int aacSize = aac_buf.size();
    int read = wav_read_data(wav, (unsigned char *)&pcm_buf[0], pcm_buf.size());

    if (read <= 0) {
      break;
    }

    if ((err = fdkaac_enc.aacenc_encode(&pcm_buf[0], read, nbSamples,
                                        &aac_buf[0], aacSize)) != AACENC_OK) {
      E_LOG("decode error, code: {}", err);
    }

    if (aacSize > 0) {
      out_aac.write(aac_buf.data(), aacSize);
    }
  }

  wav_read_close(wav);
  out_aac.close();

  I_LOG("Stop encoding...");
  return 0;
}

inline int decode_test(std::string input, std::string output) {

  I_LOG("Start decoding...");

  std::ifstream in_aac(input.c_str(), std::ios::binary);

  void *wav = NULL;

  AacDecoder fdkaac_dec;

  int ret = fdkaac_dec.aacdec_init_adts();

  
  int nbPcm = fdkaac_dec.aacdec_pcm_size();

  if (nbPcm == 0) {
    nbPcm = 50 * 1024;
  }

  std::vector<char> pcm_buf(nbPcm, 0);

  in_aac.seekg(0, std::ios::end);
  int nbAacSize = in_aac.tellg();
  I_LOG("aac_size: {}", nbAacSize);
  in_aac.seekg(0, std::ios::beg);

  std::vector<char> aac_buf(nbAacSize, 0);

  in_aac.read(&aac_buf[0], nbAacSize);

  int pos = 0;

  while (1) {

    if (nbAacSize - pos < 7) {
      I_LOG("222...");
      break;
    }

    adts_header_t *adts = (adts_header_t *)(&aac_buf[0] + pos);

    if (adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf) {
      I_LOG("333...");
      break;
    }

    int aac_frame_size = adts->frame_length_0_to_1 << 11 |
                         adts->frame_length_2_to_9 << 3 |
                         adts->frame_length_10_to_12;

    if (pos + aac_frame_size > nbAacSize) {
      I_LOG("444...");
      break;
    }

    int leftSize = aac_frame_size;
    int ret =
        fdkaac_dec.aacdec_fill(&aac_buf[0] + pos, aac_frame_size, &leftSize);
    pos += aac_frame_size;

    if (ret != 0) {
      D_LOG("here1...");
      continue;
    }

    if (leftSize > 0) {
      D_LOG("here2...");
      continue;
    }

    int validSize = 0;
    ret =
        fdkaac_dec.aacdec_decode_frame(&pcm_buf[0], pcm_buf.size(), &validSize);

    if (ret == AAC_DEC_NOT_ENOUGH_BITS) {
      D_LOG("here3...");
      continue;
    }

    if (ret != 0) {
      D_LOG("here4...");
      continue;
    }

    if (!wav) {
      if (fdkaac_dec.aacdec_sample_rate() <= 0) {
        D_LOG("here5...");
        break;
      }

      wav = wav_write_open(output.c_str(), fdkaac_dec.aacdec_sample_rate(), 16,
                           fdkaac_dec.aacdec_num_channels());
      if (!wav) {
        D_LOG("here6...");
        break;
      }
    }
    D_LOG("here7...");
    wav_write_data(wav, (unsigned char *)&pcm_buf[0], validSize);
  }

  in_aac.close();
  if (wav) {
    wav_write_close(wav);
  }

  I_LOG("Stop decoding...");
  return 0;
}