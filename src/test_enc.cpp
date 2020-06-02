#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

#include "fdkTry.h"

extern "C" {
#include "wav_reader.h"
}

int main(int argc, char *argv[]) {
  const int bitrate = 128000;

  AacEncoder fdkaac_enc;

  ofstream out_aac("hello.aac", ios::binary);
  int format, sample_rate=44100, channels = 2, bits_per_sample = 14400;

  //if (fdkaac_enc.aacenc_init(2, channels, sample_rate, bitrate) != AACENC_OK)
  // {
  //  std::cout << "init here" << std::endl;
  //  return -1;
  //}

  void *wav = wav_read_open("1.wav");
  if (!wav) {
    std::cout << "init here1" << std::endl;
    return 1;
  }
  if (!wav_get_header(wav, &format, &channels, &sample_rate, &bits_per_sample,
                      NULL)) {
    std::cout << "init here2" << std::endl;
    exit(1);
    return 1;
  }
  if (format != 1) {
    std::cout << "init here3" << std::endl;
    return 1;
  }
  if (bits_per_sample != 16) {
    std::cout << "init here4" << std::endl;
    return 1;
  }

  if (fdkaac_enc.aacenc_init(2, channels, sample_rate, bitrate) != AACENC_OK) {
    std::cout << "init here" << std::endl;
    return -1;
  }

  cout << "format: " << format << endl;
  cout << "channels: " << channels << endl;
  cout << "sample_rate: " << sample_rate << endl;

  int pcmSize = channels * 2 * fdkaac_enc.aacenc_frame_size();
  std::vector<char> pcm_buf(pcmSize, 0);

  int nbSamples = fdkaac_enc.aacenc_frame_size();

  int nbAac = fdkaac_enc.aacenc_max_output_buffer_size();
  std::vector<char> aac_buf(nbAac, 0);

  int err = 0;
  while (1) {
    int aacSize = aac_buf.size();
    int read = wav_read_data(wav, (unsigned char *)&pcm_buf[0], pcm_buf.size());

    if (read <= 0) break;

    if ((err = fdkaac_enc.aacenc_encode(&pcm_buf[0], read, nbSamples,
                                        &aac_buf[0], aacSize)) != AACENC_OK) {
      cout << "error code:" << err << endl;
    }

    if (aacSize > 0) {
      out_aac.write(aac_buf.data(), aacSize);
    }
  }

  wav_read_close(wav);
  out_aac.close();

  cout << "end" << endl;


 // return 0;
}