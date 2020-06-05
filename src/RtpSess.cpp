#include "RtpSess.h"
#include <sstream>

RtpSess::RtpSess(int port) {
  position = 0;
  portBase = port;
  timeStampUnit = 1.0 / 90000.0;

}

void RtpSess::init(){
#ifdef WIN32
  WSADATA dat;
  WSAStartup(MAKEWORD(2, 2), &dat);
#endif  // WIN32

  std::ostringstream oss;
  oss << portBase;
  std::string filename = SAVE_PATH;

  filename = filename + oss.str() + ".aac";
  fd = fopen(filename.c_str(), "wb");
  if (!fd) {
      fprintf(stderr, "Could not open %s\n", filename.c_str());
      exit(1);
  }

  sessParams.SetOwnTimestampUnit(1.0 / 90000);
  sessParams.SetUsePollThread(false);
  sessParams.SetAcceptOwnPackets(true);
  transParams.SetAllowOddPortbase(true);
  transParams.SetPortbase(portBase);

  int status = sess.Create(sessParams, &transParams);
  checkerror(status);
  pBuff = new uint8_t[MAX_BUFF_SIZE];
  sess.BeginDataAccess();
  //RTPIPv4Address addr(ntohl(inet_addr(ip)), portBase);
  //sess.AddDestination(addr);

  
}


RtpSess::~RtpSess() {
    I_LOG("rtp sess uninit");

  // if (buff != nullptr) {
  //  delete buff;
  //}
  sess.EndDataAccess();
  sess.BYEDestroy(RTPTime(10.0), 0, 0);
#ifdef RTP_SOCKETTYPE_WINSOCK
  WSACleanup();
#endif  // RTP_SOCKETTYPE_WINSOCK

}

void RtpSess::Release() {
    I_LOG("rtp sess release");
//  if (NULL != buff) {
//    delete[] buff;
//  }
//
  if (fd) {
      fclose(fd);
  }

  sess.EndDataAccess();
  sess.BYEDestroy(RTPTime(10.0), 0, 0);
#ifdef RTP_SOCKETTYPE_WINSOCK
  WSACleanup();
#endif  // RTP_SOCKETTYPE_WINSOCK
}


int RtpSess::GetFirstSourceWithData() {

  int status = sess.Poll();
  checkerror(status);
  if (sess.GotoFirstSourceWithData()) {
    return 1;
  }

  return 0;
}

int RtpSess::GotoNextSourceWithData() {
  return sess.GotoNextSourceWithData();
}

int RtpSess::EndDataAccess() { return sess.EndDataAccess(); }

int RtpSess::GetH264Packet() {
  int size = 0;
  while ((pack = sess.GetNextPacket()) != NULL) {
          loaddata = pack->GetPayloadData();
          len = pack->GetPayloadLength();
          fu_ind = loaddata[0];
          fu_hdr = loaddata[1];
          nula_hdr = loaddata[0];

          if (pack->GetPayloadType() == 96)  // H264
          {
            //                        cout << to_string((nula_hdr) & (0x1f)) <<
            //                        endl;

            //解码pps和sps
            if (((nula_hdr) & (0x1f)) == 24) {
              memcpy(&buff[position], &h264_startcode, 4);
              position += 4;
              uint16_t size1 =
                  ((uint16_t)loaddata[1] << 8) | ((uint16_t)loaddata[2]);
              //cout << "size 1 is" + to_string(size1) << endl;
              memcpy(&buff[position], &loaddata[3], (size_t)size1);
              position += size1;
              memcpy(&buff[position], &h264_startcode, 4);
              position += 4;
              uint16_t size2 = ((uint16_t)loaddata[3 + size1] << 8) |
                               ((uint16_t)loaddata[4 + size1]);
             // cout << "size 2 is" + to_string(size2) << endl;
              memcpy(&buff[position], &loaddata[5 + size1], (size_t)size2);
              position += size2;
              size = position;
              fwrite(buff, position, 1, fd);
              position = 0;
              return size;

            } else if (((nula_hdr) & (0x1f)) > 0 &&
                       ((nula_hdr) & (0x1f)) < 24) {
              uint8_t TYPE = (nula_hdr) & (0x1f);  //应用的是FU_HEADER的TYPE
              //                            cout << "type is" + to_string(TYPE)
              //                            << endl;

              memcpy(&buff[position], &h264_startcode, 4);
              position += 4;
              memcpy(&buff[position], loaddata, len);
              position += len;
              size = position;
              fwrite(buff, position, 1, fd);
              position = 0;
              return size;
            } else if (((nula_hdr) & (0x1f)) == 28) {
              //                            cout<<to_string((*fu_hdr)&(0x1f))<<endl;

              if (pack->HasMarker())  // the last packet
              {
                memcpy(&buff[position], &loaddata[2], len - 2);
                position += len - 2;
                size = position;
                fwrite(buff, position, 1, fd);
                position = 0;
                return size;
              } else {
                //分片的第一个包
                if (((fu_hdr) >> 7) == 1) {
                  //                                    cout<<"first"<<endl;
                  uint8_t F;
                  uint8_t NRI;
                  uint8_t TYPE;
                  uint8_t nh;

                  F = (fu_ind) & (0x80);
                  NRI = (fu_ind) & (0x60);
                  TYPE = (fu_hdr) & (0x1f);  //应用的是FU_HEADER的TYPE
                  nh = F | NRI | TYPE;

                  memcpy(&buff[position], &h264_startcode, 4);
                  position += 4;

                  memcpy(&buff[position], &nh, 1);
                  position += 1;

                  memcpy(&buff[position], &loaddata[2], len - 2);
                  position += len - 2;
                  size = position;
                } else {
                  memcpy(&buff[position], &loaddata[2], len - 2);
                  position += len - 2;
                  size = position;
                }
              }
            }
          } else {
            printf("!!!  GetPayloadType = %d !!!! \n ", pack->GetPayloadType());
          }

          sess.DeletePacket(pack);
  }

 // std::cout << "size: " << size << std::endl;
  return 0;
}