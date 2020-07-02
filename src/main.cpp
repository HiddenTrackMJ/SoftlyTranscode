// transcode.cpp: 定义应用程序的入口点。
//


#include "rtpSender.h"
#include "rtpRecv.h"
#include "seeker/logger.h"
#include "FFmpegDecoder.h"
#include "fdk_demo.hpp"
#include "muxer.h"

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else  // Linux/Unix
#include <dirent.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <iostream>
#include <string>
#include <thread>


#define First 1

extern int decode_test(std::string input, std::string output);
extern int encode_test(std::string input, std::string output);

#ifdef First
int main(int argc, char *argv[]) {
  seeker::Logger::init();

  I_LOG("This is TransCode");

//  std::string s = "hello1";
//
//  std::string dir = "./" + s;
//  if (access(dir.c_str(), 0) == -1) {
//#ifdef WIN32
//    int flag = mkdir(dir.c_str());
//#endif
//#ifdef linux
//    int flag = mkdir(dir.c_str(), 0777);
//#endif
//    if (flag == 0) {
//      I_LOG( "Create dir successfully" );
//    } else {
//      E_LOG("Failed to create dir");
//    }
//  }
//
//  return 0;

  
  //muxer.mux(  "D:/Study/Scala/VSWS/transcode/out/build/x64-Release/encode.aac",
  //    "./hls/master.m3u8");
  //return 0;

  int select;

  std::cout << "Please select mode, 1 for decoding else for encoding: "
            << std::endl;
  std::cin >> select;

  if (select == 1) {
    RtpSender sender;
    sender.send_aac();
    //sender.send_aac("127.0.0.1", 8080, "D:/Study/Scala/VSWS/transcode/out/build/x64-Release/1.wav");
    //decode_test("short.aac", "short.wav");
  } else {

    RtpReceiver recvr;
    Muxer muxer;
    //recvr.recv_aac();
    //recvr.recv_aac(8080);
    //muxer.recv_aac_mux(8080, "D:/Study/Scala/VSWS/transcode/out/build/x64-Release/encode.aac",
    //          "./hls/master.m3u8");

    auto n = std::thread::hardware_concurrency();  //获取cpu核心个数
    std::cout << n << std::endl;  

    muxer.open_thread(8080, "./hls/master.m3u8");
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

