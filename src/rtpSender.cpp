//
// Created by 郝雪 on 2020/6/4.
//

#include <iostream>
#include "rtpSender.h"

//jrtplib
#include "rtpsession.h"
#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtptimeutilities.h"
#include "rtppacket.h"
#include "rtperrors.h"
#pragma comment(lib, "jrtplib.lib")

//common
#include "seeker/socketUtil.h"
#include "seeker/common.h"
#include "config.h"
#include "seeker/loggerApi.h"
#include "string"

#include "encoder.h"

//extern "C" {
//#include "wav_reader.hpp"
//#include "wav_writer.hpp"
//}



using jrtplib::RTPSession;
using jrtplib::RTPSessionParams;
using jrtplib::RTPUDPv4TransmissionParams;
using jrtplib::RTPPacket;
using jrtplib::RTPTime;
using jrtplib::RTPIPv4Address;

using std::string;


int RtpSender::parseAdtsHeader(uint8_t *in, struct AdtsHeader *res) {
    static int frame_number = 0;
    memset(res, 0, sizeof(*res));

    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)) {
        res->id = ((unsigned int) in[1] & 0x08) >> 3;
        //printf("adts:id  %d\n", res->id);
        res->layer = ((unsigned int) in[1] & 0x06) >> 1;
        //printf("adts:layer  %d\n", res->layer);
        res->protectionAbsent = (unsigned int) in[1] & 0x01;
        //printf("adts:protection_absent  %d\n", res->protectionAbsent);
        res->profile = ((unsigned int) in[2] & 0xc0) >> 6;
        //printf("adts:profile  %d\n", res->profile);
        res->samplingFreqIndex = ((unsigned int) in[2] & 0x3c) >> 2;
        //printf("adts:sf_index  %d\n", res->samplingFreqIndex);
        res->privateBit = ((unsigned int) in[2] & 0x02) >> 1;
        //printf("adts:pritvate_bit  %d\n", res->privateBit);
        res->channelCfg = ((((unsigned int) in[2] & 0x01) << 2) | (((unsigned int) in[3] & 0xc0) >> 6));
        //printf("adts:channel_configuration  %d\n", res->channelCfg);
        res->originalCopy = ((unsigned int) in[3] & 0x20) >> 5;
        //printf("adts:original  %d\n", res->originalCopy);
        res->home = ((unsigned int) in[3] & 0x10) >> 4;
        //printf("adts:home  %d\n", res->home);
        res->copyrightIdentificationBit = ((unsigned int) in[3] & 0x08) >> 3;
        //printf("adts:copyright_identification_bit  %d\n", res->copyrightIdentificationBit);
        res->copyrightIdentificationStart = (unsigned int) in[3] & 0x04 >> 2;
        //printf("adts:copyright_identification_start  %d\n", res->copyrightIdentificationStart);
        res->aacFrameLength = (((((unsigned int) in[3]) & 0x03) << 11) |
                               (((unsigned int) in[4] & 0xFF) << 3) |
                               ((unsigned int) in[5] & 0xE0) >> 5);
        //printf("adts:aac_frame_length  %d\n", res->aacFrameLength);
        res->adtsBufferFullness = (((unsigned int) in[5] & 0x1f) << 6 |
                                   ((unsigned int) in[6] & 0xfc) >> 2);
        //printf("adts:adts_buffer_fullness  %d\n", res->adtsBufferFullness);
        res->numberOfRawDataBlockInFrame = ((unsigned int) in[6] & 0x03);
        //printf("adts:no_raw_data_blocks_in_frame  %d\n", res->numberOfRawDataBlockInFrame);

        return 0;
    } else {
        printf("failed to parse adts header\n");
        return -1;
    }
}

void RtpSender::checkerror(int rtperr) {
    if (rtperr < 0) {
        std::cout << "ERROR: " << rtperr << std::endl;
        exit(-1);
    }
}

void RtpSender::send_aac() {

    uint16_t destport;
    uint32_t destip;
    int status, index;
    std::string ipstr;

    unsigned char buffer[2000];
    seeker::SocketUtil::startupWSA();
    std::cout << "Enter the destination IP address" << std::endl;
    std::cin >> ipstr;
    destip = inet_addr(ipstr.c_str());
    if (destip == INADDR_NONE) {
        std::cerr << "Bad IP address specified" << std::endl;
        return;
    }
    destip = ntohl(destip);
    std::cout << "Enter the destination port" << std::endl;
    std::cin >> destport;

    // 创建RTP会话
    RTPSession sess;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;
    sessparams.SetOwnTimestampUnit(1.0 / 8000.0);
    transparams.SetPortbase(22400);
    status = sess.Create(sessparams, &transparams);
    checkerror(status);
    RTPIPv4Address addr(destip, destport);

    status = sess.SetDefaultTimestampIncrement(100);
    checkerror(status);
    status = sess.AddDestination(addr);

    checkerror(status);


    // 设置RTP会话默认参数

    sess.SetDefaultPayloadType(0);

    sess.SetDefaultMark(true);

    /*假设音频的采样率位44100，即每秒钟采样44100次

    AAC一般将1024次采样编码成一帧，所以一秒就有44100/1024=43帧

    RTP包发送的每一帧数据的时间增量为44100/43=1025

    每一帧数据的时间间隔为1000/43=23ms*/
//    sess.SetDefaultTimestampIncrement(20);
    sess.SetDefaultMark(true);

    // 发送流媒体数据

    index = 1;

    unsigned int read_len = 1000;
    int len = 0;
    char *filename =
        "D:/Study/Scala/VSWS/transcode/out/build/x64-Release/encode.aac";  //"/Users/haoxue/Music/output.aac";
    FILE *aac = fopen(filename, "rb");
    if (aac == NULL) {
        free(aac);
        return;
    }

    do {
        len = fread(buffer, 1, read_len, aac);
        status = sess.SendPacket(buffer, len);
        checkerror(status);
        I_LOG("send packet size is {}", len);
        RTPTime::Wait(RTPTime(0, 100000));
    } while (len == read_len);

    I_LOG("send data end");

}


//时间戳，时间戳增量，每帧发送的时间都和待发送的aac有关
void RtpSender::send_aac(const string &destipstr, int destport,
                         const string &filename) {
  uint32_t destip;
  int status;
  uint8_t frame[2000] = {0};
  destip = inet_addr(destipstr.c_str());
  if (destip == INADDR_NONE) {
    E_LOG("Bad IP address specified");
    return;
  }
  destip = ntohl(destip);
  seeker::SocketUtil::startupWSA();
  // 创建RTP会话
  RTPSession sess;
  RTPUDPv4TransmissionParams transparams;
  RTPSessionParams sessparams;
  sessparams.SetOwnTimestampUnit(1.0 / 8000);  //采样率
  transparams.SetPortbase(51000);
  transparams.SetAllowOddPortbase(true);
  status = sess.Create(sessparams, &transparams);
  checkerror(status);  // create之后check一下
  //设置session参数
  sess.SetDefaultTimestampIncrement(
      1025);  // 44100/1024=43帧;44100/43=1025;时间间隔 1000/43=23ms
  RTPIPv4Address addr(destip, destport);
  sess.AddDestination(addr);
  sess.SetDefaultPayloadType(97);
  sess.SetDefaultMark(true);
  // 发送流媒体数据
  int len = 0;
  struct AdtsHeader adtsHeader;
  FILE *aac = fopen(filename.c_str(), "rb");
  if (aac == nullptr) {
    free(aac);
    return;
  }

  aac_encoder enc;
  enc.init(2, TT_MP4_ADTS);

  uint8_t output[20480] = {0};  //[20480]
  int output_size = 0;

  do {
    //len = fread(frame, 1, ADTS_HRADER_LENGTH, aac);  //先读adts的前七个字节头

    //if (len <= 0) {
    //  fseek(aac, 0, SEEK_SET);
    //  I_LOG("send over");
    //  break;
    //}

    //if (parseAdtsHeader(frame, &adtsHeader) < 0) {
    //  E_LOG("parse err");
    //  break;
    //}

    //I_LOG("frmae len: {}", adtsHeader.aacFrameLength);

    //len = fread(frame + ADTS_HRADER_LENGTH, 1, adtsHeader.aacFrameLength - 7,
    //            aac);

    //if (len < 0) {
    //  E_LOG("read err");
    //  break;
    //}

    enc.encode_one_frame(output, output_size);
    if (output_size <= 0) break;
    while (output_size > 0) {
      //std::cout <<"222 out_put: "<< output << ", out_size: " << output_size << std::endl;
      memcpy(frame, output, ADTS_HRADER_LENGTH);
      //output = output + ADTS_HRADER_LENGTH;
     
      if (parseAdtsHeader(frame, &adtsHeader) < 0) {
        E_LOG("parse err");
        break;
      }
      //I_LOG("frmae len: {}", adtsHeader.aacFrameLength);

      memcpy(frame + ADTS_HRADER_LENGTH, output, adtsHeader.aacFrameLength - 7);
      output_size -= adtsHeader.aacFrameLength;
      status = sess.SendPacket(output, adtsHeader.aacFrameLength);
      checkerror(status);
      I_LOG("send packet size is {}", adtsHeader.aacFrameLength);
      memset(output, 0, 4096);
      RTPTime::Wait(RTPTime(0, 23000));
    }

  } while (true);

  fclose(aac);
  sess.Destroy();
}
