//
// Created by 郝雪 on 2020/6/2.
//
#pragma once
#include "adts_header.hpp"
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
#include <chrono>

#ifndef RETREAM_RTPRECV_H
#define RETREAM_RTPRECV_H

class RtpReceiver{
public:
    void rtpReceivcer_adts();
    void rtpReceivcer_latm();
    static void checkerror(int rtperr);
    void recv_aac();
    void recv_aac(int port);
};


#endif //RETREAM_RTPRECV_H
