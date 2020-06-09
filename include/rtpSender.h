//
// Created by 郝雪 on 2020/6/4.
//
#pragma once
#include "adts_header.hpp"

#ifndef RETREAM_RTPSENDER_H
#define RETREAM_RTPSENDER_H

#define ADTS_HRADER_LENGTH 7
class RtpSender{
  

 public:
    void send_aac();
    void send_aac(const std::string& ipstr, int destport, const std::string& filename);
    void checkerror(int rtperr);

};


#endif  // RETREAM_RTPSENDER_H