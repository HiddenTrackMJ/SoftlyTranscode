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


#define First 0

extern int decode_test(std::string input, std::string output);
extern int encode_test(std::string input, std::string output);

#ifdef First
int main(int argc, char *argv[]) {
  seeker::Logger::init();

  I_LOG("This is TransCode");

   int n = 5;

   const char *pad_name;

   std::string p = "in" + std::to_string(n);

   pad_name = "in";  // p.c_str();


  Mixer mixer;
  //mixer.mix("D:/Study/Scala/VSWS/retream/out/build/x64-Release/recv.aac", "./hls/test.aac");
  mixer.open_thread(8080, "./hls/mix.aac");
  I_LOG("pad_name: {}", pad_name);
  //mixer.init_filters(2);
  //I_LOG("decs: {}", mixer.gen_filter_desc(8).c_str());
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

#else
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avutil.h"
#include "libavutil/fifo.h"
}
class Fmt_ctx_in {
 public:
  AVAudioFifo* _fifo_spk;
  int index;
  int* ptr;

  void Set_fifo(AVAudioFifo* _fifo_spk2) { _fifo_spk = _fifo_spk2; }
  Fmt_ctx_in() {
    _fifo_spk = NULL;
    index = -1;
  }
};



int main() {
  int index = 3;
  std::shared_ptr<Fmt_ctx_in> fmt_ptr1(new Fmt_ctx_in);
  std::shared_ptr<Fmt_ctx_in> fmt_ptr2(new Fmt_ctx_in);
  std::vector<std::shared_ptr<Fmt_ctx_in> > _fmt_ctx_in = {};
  _fmt_ctx_in.push_back(fmt_ptr1);
  _fmt_ctx_in.push_back(fmt_ptr2);
  for (auto it : _fmt_ctx_in) {
    index++;
    it->index = index;
    it->ptr = &(it->index);
    std::cout << "ptr1: " << it->ptr << std::endl;
  }

   for (auto it : _fmt_ctx_in) {
    std::cout << "index: " << it->index << std::endl;
     std::cout << "ptr2: " << it->ptr << std::endl;
  }

  return 0;
}

#endif