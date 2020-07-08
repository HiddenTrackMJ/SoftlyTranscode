// transcode.cpp: 定义应用程序的入口点。
//


#include "rtpSender.h"
#include "rtpRecv.h"
#include "seeker/logger.h"
#include "FFmpegDecoder.h"
#include "fdk_demo.hpp"
#include "muxer.h"
#include "mixer.h"

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


int main98489(int argc, char *argv[]) {
  seeker::Logger::init();

  I_LOG("This is TransCode");

  Mixer mixer;
  //mixer.mix("D:/Study/Scala/VSWS/retream/out/build/x64-Release/recv.aac", "./hls/test.aac");
  mixer.open_thread(8080, "./hls/mix.aac");
  return 1000;

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

  encode_test("test.wav", "test1.aac");
  return 0;

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

