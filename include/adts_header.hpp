#pragma once
#include <stdint.h>

#include <string>

namespace headerUtil {


struct AdtsHeader {
  unsigned int syncword;  // 12 bit 同步字 '1111 1111
                          // 1111'，说明一个ADTS帧的开始
  unsigned int id;        // 1 bit MPEG 标示符， 0 for MPEG-4，1 for MPEG-2
  unsigned int layer;     // 2 bit 总是'00'
  unsigned int protectionAbsent;   // 1 bit 1表示没有crc，0表示有crc
  unsigned int profile;            // 1 bit 表示使用哪个级别的AAC
  unsigned int samplingFreqIndex;  // 4 bit 表示使用的采样频率
  unsigned int privateBit;         // 1 bit
  unsigned int channelCfg;         // 3 bit 表示声道数
  unsigned int originalCopy;       // 1 bit
  unsigned int home;               // 1 bit

  /*下面的为改变的参数即每一帧都不同*/
  unsigned int copyrightIdentificationBit;    // 1 bit
  unsigned int copyrightIdentificationStart;  // 1 bit
  unsigned int aacFrameLength;  // 13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
  unsigned int adtsBufferFullness;  // 11 bit 0x7FF 说明是码率可变的码流

  /* number_of_raw_data_blocks_in_frame
   * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
   * 所以说number_of_raw_data_blocks_in_frame == 0
   * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
   */
  unsigned int numberOfRawDataBlockInFrame;  // 2 bit
};


inline int parseAdtsHeader(uint8_t* in, struct AdtsHeader* res) {
  static int frame_number = 0;
  memset(res, 0, sizeof(*res));

  if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)) {
    res->id = ((unsigned int)in[1] & 0x08) >> 3;
    // printf("adts:id  %d\n", res->id);
    res->layer = ((unsigned int)in[1] & 0x06) >> 1;
    // printf("adts:layer  %d\n", res->layer);
    res->protectionAbsent = (unsigned int)in[1] & 0x01;
    // printf("adts:protection_absent  %d\n", res->protectionAbsent);
    res->profile = ((unsigned int)in[2] & 0xc0) >> 6;
    // printf("adts:profile  %d\n", res->profile);
    res->samplingFreqIndex = ((unsigned int)in[2] & 0x3c) >> 2;
    // printf("adts:sf_index  %d\n", res->samplingFreqIndex);
    res->privateBit = ((unsigned int)in[2] & 0x02) >> 1;
    // printf("adts:pritvate_bit  %d\n", res->privateBit);
    res->channelCfg = ((((unsigned int)in[2] & 0x01) << 2) |
                       (((unsigned int)in[3] & 0xc0) >> 6));
    // printf("adts:channel_configuration  %d\n", res->channelCfg);
    res->originalCopy = ((unsigned int)in[3] & 0x20) >> 5;
    // printf("adts:original  %d\n", res->originalCopy);
    res->home = ((unsigned int)in[3] & 0x10) >> 4;
    // printf("adts:home  %d\n", res->home);
    res->copyrightIdentificationBit = ((unsigned int)in[3] & 0x08) >> 3;
    // printf("adts:copyright_identification_bit  %d\n",
    // res->copyrightIdentificationBit);
    res->copyrightIdentificationStart = (unsigned int)in[3] & 0x04 >> 2;
    // printf("adts:copyright_identification_start  %d\n",
    // res->copyrightIdentificationStart);
    res->aacFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
                           (((unsigned int)in[4] & 0xFF) << 3) |
                           ((unsigned int)in[5] & 0xE0) >> 5);
    // printf("adts:aac_frame_length  %d\n", res->aacFrameLength);
    res->adtsBufferFullness =
        (((unsigned int)in[5] & 0x1f) << 6 | ((unsigned int)in[6] & 0xfc) >> 2);
    // printf("adts:adts_buffer_fullness  %d\n", res->adtsBufferFullness);
    res->numberOfRawDataBlockInFrame = ((unsigned int)in[6] & 0x03);
    // printf("adts:no_raw_data_blocks_in_frame  %d\n",
    // res->numberOfRawDataBlockInFrame);

    return 0;
  } else {
    printf("failed to parse adts header\n");
    return -1;
  }
}
}  // namespace headerUtil