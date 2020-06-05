#pragma once

#include "config.h"
#include "seeker/loggerApi.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif  

#include <iostream>
#include "rtpipv4address.h"
#include "rtppacket.h"
#include "rtpsession.h"
#include "rtpsessionparams.h"
#include "rtptimeutilities.h"
#include "rtpudpv4transmitter.h"
#pragma comment(lib, "jrtplib.lib")

#define MAX_BUFF_SIZE 1920 * 1080
using jrtplib::RTPSession;
using jrtplib::RTPSessionParams;
using jrtplib::RTPIPv4Address;
using jrtplib::RTPUDPv4TransmissionParams;
using jrtplib::RTPPacket;
using jrtplib::RTPTime;
using jrtplib::RTPGetErrorString;

//extern int platform;

class RtpSess {
 public:
  RtpSess(int port);
  ~RtpSess();

  void init();
  RTPSession sess;
  uint8_t* pBuff;
  int GetH264Packet();
  int GetFirstSourceWithData();
  int GotoNextSourceWithData();
  int EndDataAccess();
  void Release();
    uint8_t buff[1024 * 100] = {0};
  int size;

 private:
  
  FILE* fd;
  size_t len;
  size_t headlen;
  uint8_t* loaddata;
  RTPPacket* pack;
  int i = 0;
  int position = 0;
  unsigned int h264_startcode = 0x01000000;
  uint8_t fu_ind;
  uint8_t fu_hdr;
  uint8_t nula_hdr;

  uint16_t portBase = 1234;
//  char ip[16] = "127.0.0.1";
  
  RTPSessionParams sessParams;
  RTPUDPv4TransmissionParams transParams;

  double timeStampUnit;


  void checkerror(int rtperr) {
    if (rtperr < 0) {
        E_LOG("ERROR: {}", RTPGetErrorString(rtperr));
      exit(-1);
    }
  }
};