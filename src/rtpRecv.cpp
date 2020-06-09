//
// Created by haoxue on 2020/6/2.
//


#include "rtpRecv.h"
#include "fdk_dec.h"
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

 extern "C" {
#include "wav_reader.hpp"
#include "wav_writer.hpp"
}


using jrtplib::RTPSession;
using jrtplib::RTPSessionParams;
using jrtplib::RTPUDPv4TransmissionParams;
using jrtplib::RTPPacket;
using jrtplib::RTPTime;

using std::to_string;

void RtpReceiver::checkerror(int rtperr) {
    if (rtperr < 0) {
//        E_LOG(RTPGetErrorString(rtperr));

        E_LOG("RTPGetErrorStatus:{}", rtperr);
        // exit(-1);
    }
}


void RtpReceiver::rtpReceivcer_latm() {

    int port = 8080;
    //seeker::SocketUtil::startupWSA();
    WSADATA wsaData = {};
    if (WSAStartup(SOCKET_VERSION, &wsaData) != OK) {
      throw std::runtime_error("WSAStartup failed.");
    }
    std::string filename = SAVE_PATH;
    filename = filename + to_string(seeker::Time::currentTime()) + ".aac";

    size_t len;
    size_t payloadlen = 0;
    uint8_t *payloaddata;
//    uint8_t *cbPayloadbuff[1024 * 100]={0};
//    size_t cbPayloadlen = 0;
    uint8_t pos = 0;
    uint8_t buff[1024 * 100] = {0};
    uint8_t buffsize = 0;
    FILE *fd = fopen(filename.c_str(), "wb");
    if (!fd) {
        E_LOG("Could not open file : {}", filename);
        //exit(1);
    }

    RTPSession rtpSession;
    RTPSessionParams rtpSessionParams;
    RTPUDPv4TransmissionParams rtpUdpv4Transmissionparams;

    rtpSessionParams.SetOwnTimestampUnit(1.0 / 8000.0); //音频为1.0/8000，视频为1.0/90000
    rtpSessionParams.SetUsePollThread(false);
    rtpUdpv4Transmissionparams.SetPortbase(port);
    rtpSessionParams.SetAcceptOwnPackets(true);

    int status = rtpSession.Create(rtpSessionParams, &rtpUdpv4Transmissionparams);
    checkerror(status);

    bool stop = false;
    while (!stop) {

        rtpSession.BeginDataAccess();
        RTPPacket *pack;
        // check incoming packets
        if (rtpSession.GotoFirstSourceWithData()) {
            do {
                while ((pack = rtpSession.GetNextPacket()) != NULL) {

                    I_LOG("payload type is {}", pack->GetPayloadType());

                    payloaddata = pack->GetPayloadData();
                    len = pack->GetPayloadLength();

                    I_LOG("marker is {}", pack->HasMarker());

                    if (pack->HasMarker()) {

                        memcpy(&buff[buffsize], &payloaddata[0], len);
                        buffsize += len;
                        I_LOG("buffsize is {}", buffsize);
                        while (pos < buffsize) {

                            I_LOG("buffsize is {} ,pos is {} ", buffsize, pos);
                            if (buff[pos] == 255) {
                                I_LOG("+++++++++1");
                                payloadlen += 255;
                                pos += 1;
                            } else {
                                I_LOG("hhhhhhhhh");
                                payloadlen += buff[pos];
                                I_LOG("payloadlen is {}", payloadlen);
                                pos += 1;
//                                memcpy(&buff[0], &payloaddata[pos], payloadlen);
                                fwrite(&buff[pos], payloadlen, 1, fd);
                                pos += payloadlen;
                            }
                        }
                        I_LOG(" end pos is {}", pos);
                        buffsize = 0;
                        pos = 0;
                        payloadlen = 0;
                    } else {
                        memcpy(&buff[buffsize], &payloaddata[0], len);
                        buffsize += len;
                    }

                    rtpSession.DeletePacket(pack);
                }
            } while (rtpSession.GotoNextSourceWithData());

        }

        rtpSession.EndDataAccess();

        int status = rtpSession.Poll();
        checkerror(status);

        RTPTime::Wait(RTPTime(0, 100));

    }


}


void RtpReceiver::rtpReceivcer_adts() {

    int port = 8080;
    //seeker::SocketUtil::startupWSA();
    WSADATA wsaData = {};
    if (WSAStartup(SOCKET_VERSION, &wsaData) != OK) {
      throw std::runtime_error("WSAStartup failed.");
    }
    std::string filename = SAVE_PATH;
    filename = filename + to_string(seeker::Time::currentTime()) + ".aac";

    size_t len;
    size_t payloadlen = 0;
    uint8_t *payloaddata;
//    uint8_t *cbPayloadbuff[1024 * 100]={0};
//    size_t cbPayloadlen = 0;
    uint8_t pos = 0;
    uint8_t buff[1024 * 100] = {0};
    uint8_t buffsize = 0;
    FILE *fd = fopen(filename.c_str(), "wb");
    if (!fd) {
        E_LOG("Could not open file : {}", filename);
        //exit(1);
    }

    RTPSession rtpSession;
    RTPSessionParams rtpSessionParams;
    RTPUDPv4TransmissionParams rtpUdpv4Transmissionparams;

    rtpSessionParams.SetOwnTimestampUnit(1.0 / 90000.0);
    rtpSessionParams.SetUsePollThread(false);
    rtpUdpv4Transmissionparams.SetPortbase(port);
    rtpSessionParams.SetAcceptOwnPackets(true);

    int status = rtpSession.Create(rtpSessionParams, &rtpUdpv4Transmissionparams);
    checkerror(status);

    bool stop = false;
    while (!stop) {

        rtpSession.BeginDataAccess();
        RTPPacket *pack;
        // check incoming packets
        if (rtpSession.GotoFirstSourceWithData()) {
            do {
                while ((pack = rtpSession.GetNextPacket()) != NULL) {

                    I_LOG("payload type is {}, marker is {}, payloadlen is {}", pack->GetPayloadType(),pack->HasMarker(),pack->GetPayloadLength());

                    payloaddata = pack->GetPayloadData();
                    len = pack->GetPayloadLength();

                    if (pack->HasMarker()) {

//                        memcpy(&buff[buffsize], &payloaddata[0], len);
//                        buffsize += len;

//                        I_LOG("buffsize is {}", buffsize);

                        unsigned char adtsHeader[7] = {0};

                        int profile = 1;//aac-lc
                        int freqIdx = 4;//44100
                        int chanCfg = 2; //mono channel
                        int packetLen = len + 7 - 4;//inPacket为rtp的payload数据

                        adtsHeader[0] = 0xFF;
                        adtsHeader[1] = 0xF1; //1111 0001
                        adtsHeader[2] = ((profile) << 6) | (freqIdx << 2) | (chanCfg >> 2);
                        adtsHeader[3] = ((chanCfg & 3) << 6) | (packetLen >> 11);
                        adtsHeader[4] = (packetLen >> 3) & 0xff;//(packetLen >> 3) & 0xff;
                        adtsHeader[5] = ((packetLen & 0x7) << 5) | 0x1f;
                        adtsHeader[6] = 0xFC;//one raw data block
                        fwrite(&adtsHeader, sizeof(adtsHeader), 1, fd);
                        fwrite(&payloaddata[pos+4],len-4, 1, fd);

                        buffsize = 0;
                        pos = 0;
                        payloadlen = 0;
                    }
//                    else {
//                        memcpy(&buff[buffsize], &payloaddata[0], len);
//                        buffsize += len;
//                    }

                    rtpSession.DeletePacket(pack);
                }
            } while (rtpSession.GotoNextSourceWithData());

        }

        rtpSession.EndDataAccess();

        int status = rtpSession.Poll();
        checkerror(status);

        RTPTime::Wait(RTPTime(0, 100));

    }


}


void RtpReceiver::recv_aac(){
    int port = 8080;
    //seeker::SocketUtil::startupWSA();
    WSADATA wsaData = {};
    if (WSAStartup(SOCKET_VERSION, &wsaData) != OK) {
      throw std::runtime_error("WSAStartup failed.");
    }
    std::string filename = SAVE_PATH;
    filename = filename + to_string(seeker::Time::currentTime()) + ".aac";

    size_t len;
    size_t payloadlen = 0;
    uint8_t *payloaddata;
//    uint8_t *cbPayloadbuff[1024 * 100]={0};
//    size_t cbPayloadlen = 0;
    uint8_t pos = 0;
    uint8_t buff[1024 * 100] = {0};
    uint8_t buffsize = 0;
    FILE *fd = fopen(filename.c_str(), "wb");
    if (!fd) {
        E_LOG("Could not open file : {}", filename);
        //exit(1);
    }

    RTPSession rtpSession;
    RTPSessionParams rtpSessionParams;
    RTPUDPv4TransmissionParams rtpUdpv4Transmissionparams;

    rtpSessionParams.SetOwnTimestampUnit(1.0 / 10);
    rtpSessionParams.SetUsePollThread(false);
    rtpUdpv4Transmissionparams.SetPortbase(port);
    rtpSessionParams.SetAcceptOwnPackets(true);

    int status = rtpSession.Create(rtpSessionParams, &rtpUdpv4Transmissionparams);
    checkerror(status);

    bool stop = false;
    while (!stop) {

        rtpSession.BeginDataAccess();
        RTPPacket *pack;
        // check incoming packets
        if (rtpSession.GotoFirstSourceWithData()) {
            do {
                while ((pack = rtpSession.GetNextPacket()) != NULL) {

                    I_LOG("payload type is {}, marker is {}, payloadlen is {}", pack->GetPayloadType(),pack->HasMarker(),pack->GetPayloadLength());

                    payloaddata = pack->GetPayloadData();
                    len = pack->GetPayloadLength();

//                    if (pack->HasMarker()) {

                        fwrite(&payloaddata[pos],len, 1, fd);

//                    }

                    rtpSession.DeletePacket(pack);
                }
            } while (rtpSession.GotoNextSourceWithData());

        }

        rtpSession.EndDataAccess();

        int status = rtpSession.Poll();
        checkerror(status);

        RTPTime::Wait(RTPTime(0, 100));

    }

}


//直接保存拿到的payload
void RtpReceiver::recv_aac(int port) {
  seeker::SocketUtil::startupWSA();
  std::string filename = SAVE_PATH;
  filename = "recv.aac" ;//filename + to_string(seeker::Time::currentTime()) + "recv.aac";
  size_t payloadlen = 0;
  uint8_t *payloaddata;
  uint8_t pos = 0;
  uint8_t frame[2000] = {0};
  struct headerUtil::AdtsHeader adtsHeader;
  FILE *fd = fopen(filename.c_str(), "wb");
  void *wav = NULL;

  AacDecoder fdkaac_dec;

  int ret = fdkaac_dec.aacdec_init_adts();

  int nbPcm = fdkaac_dec.aacdec_pcm_size();

  if (nbPcm == 0) {
    nbPcm = 50 * 1024;
  }

  std::vector<char> pcm_buf(nbPcm, 0);

  if (!fd) {
    E_LOG("Could not open file : {}", filename);
    // exit(1);
  }

  RTPSession rtpSession;
  RTPSessionParams rtpSessionParams;
  RTPUDPv4TransmissionParams rtpUdpv4Transmissionparams;

  rtpSessionParams.SetOwnTimestampUnit(1.0 / 44100);
  rtpSessionParams.SetUsePollThread(false);
  rtpUdpv4Transmissionparams.SetPortbase(port);
  rtpSessionParams.SetAcceptOwnPackets(true);

  int status = rtpSession.Create(rtpSessionParams, &rtpUdpv4Transmissionparams);
  checkerror(status);

  bool stop = false;
  while (!stop) {
    rtpSession.BeginDataAccess();
    RTPPacket *pack;
    // check incoming packets
    if (rtpSession.GotoFirstSourceWithData()) {
      do {
        while ((pack = rtpSession.GetNextPacket()) != NULL) {
          //I_LOG("payload type is {}, marker is {}, payloadlen is {}",
          //      pack->GetPayloadType(), pack->HasMarker(),
          //      pack->GetPayloadLength());
          payloaddata = pack->GetPayloadData();
          payloadlen = pack->GetPayloadLength();

          if (payloadlen < 100) {
            std::cout << "Bye: " << (uint16_t *)payloaddata << std::endl;
            goto Exit;
          }

          adts_header_t *adts = (adts_header_t *)(&payloaddata[0] + pos);

          if (adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf) {
            I_LOG("333...");
            break;
          }

          int aac_frame_size = adts->frame_length_0_to_1 << 11 |
                               adts->frame_length_2_to_9 << 3 |
                               adts->frame_length_10_to_12;

          //std::cout << "data:" << (uint16_t *)payloaddata << std::endl;
          //I_LOG("data: {}", (uint16_t *)payloaddata);
          I_LOG("size: {}, len, {}", aac_frame_size, payloadlen);

          int leftSize = aac_frame_size;
          int ret = fdkaac_dec.aacdec_fill((char *)&payloaddata[0],
                                           aac_frame_size,
                                           &leftSize);
          // pos += payloadlen;

          // memcpy(frame, payloaddata, ADTS_HRADER_LENGTH);
          //// output = output + ADTS_HRADER_LENGTH;

          //if (headerUtil::parseAdtsHeader(frame, &adtsHeader) < 0) {
          //  E_LOG("parse err");
          //  break;
          //}

          //int leftSize = adtsHeader.aacFrameLength;
          //int ret = fdkaac_dec.aacdec_fill(
          //    (char *)payloaddata, adtsHeader.aacFrameLength,
          //                                 &leftSize);
          ////pos += payloadlen;

          if (ret != 0) {
            D_LOG("here1...");
            continue;
          }

          if (leftSize > 0) {
            D_LOG("here2...");
            continue;
          }

          int validSize = 0;
          ret = fdkaac_dec.aacdec_decode_frame(&pcm_buf[0], pcm_buf.size(),
                                               &validSize);

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

            wav =
                wav_write_open("recv.wav", fdkaac_dec.aacdec_sample_rate(),
                               16, fdkaac_dec.aacdec_num_channels());
            if (!wav) {
              D_LOG("here6...");
              break;
            }
          }
          //std::cout << "data:" << (uint32_t *)&pcm_buf[0] << std::endl;
          wav_write_data(wav, (unsigned char *)&pcm_buf[0], validSize);

          fwrite(&payloaddata[pos], payloadlen, 1, fd);

          rtpSession.DeletePacket(pack);
        }
      } while (rtpSession.GotoNextSourceWithData());
    }

    rtpSession.EndDataAccess();

    int status = rtpSession.Poll();
    checkerror(status);

    RTPTime::Wait(RTPTime(0, 10));
  }
Exit:
  if (wav) {
    wav_write_close(wav);
  }

  rtpSession.Destroy();
  fclose(fd);
  I_LOG("Closed sess");
}

