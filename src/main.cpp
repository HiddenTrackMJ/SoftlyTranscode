// transcode.cpp: 定义应用程序的入口点。
//


#include "rtpSender.h"
#include "rtpRecv.h"
#include "seeker/logger.h"
#include "FFmpegDecoder.h"
#include "fdk_demo.hpp"
#include "muxer.h"

#define First 1

extern int decode_test(std::string input, std::string output);
extern int encode_test(std::string input, std::string output);

#ifdef First
int main(int argc, char *argv[]) {
  seeker::Logger::init();

  I_LOG("This is TransCode");

  Muxer muxer;
  muxer.mux("D:/Study/Scala/VSWS/transcode/out/build/x64-Release/encode.aac",
            "./dash/index.mpd");

  return 0;
  int select;

  std::cout << "Please select mode, 1 for decoding else for encoding: "
            << std::endl;
  std::cin >> select;

  if (select == 1) {
    RtpSender sender;
    //sender.send_aac();
    sender.send_aac("127.0.0.1", 8080, "D:/Study/Scala/VSWS/transcode/out/build/x64-Release/1.wav");
    //decode_test("short.aac", "short.wav");
  } else {

    RtpReceiver recvr;
    //recvr.recv_aac();
    recvr.recv_aac(8080);
    //encode_test("1.wav", "time.aac");
  }

  return 0;
}

#else 
int main(int argc, char **argv) {
  seeker::Logger::init();
  const char *filename, *outfilename;
  FILE *f;
  uint8_t inbuf[4096 + AV_INPUT_BUFFER_PADDING_SIZE];
  int ret;
  int data_size;

  filename = "D:/Download/Videos/LadyLiu/hello2.h264";  // 输入的264文件
  outfilename = "D:/Download/Videos/LadyLiu/hhh.h264";

  /* set end of buffer to 0 (this ensures
  that no overreading happens for
   * damaged MPEG streams) */
  memset(inbuf + 4096, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  FFmpegDecoder decoder;
  ImageMat mImageMat;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Could not open %s\n", filename);
    exit(1);
  }

  while (!feof(f)) {
    /* read raw data from the input file */
    data_size = fread(inbuf, 1, 4096, f);
    if (!data_size) break;

    /*  printf("recv %d , data: %s,\n", data_size,
            inbuf);*/

    int retC = decoder.InputData(inbuf, data_size, mImageMat);

  }

  /* flush the decoder */
  int t_w = 0;
  int t_h = 0;
  // decode(c, frame, NULL, outfilename, mImageMat);

  decoder.InputData(NULL, 0, mImageMat);

  fclose(f);

  return -100;
}
#endif