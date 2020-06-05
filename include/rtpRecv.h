//
// Created by 郝雪 on 2020/6/2.
//

#ifndef RETREAM_RTPRECV_H
#define RETREAM_RTPRECV_H

class RtpReceiver{
public:
    void rtpReceivcer_adts();
    void rtpReceivcer_latm();
    void checkerror(int rtperr);
    void recv_aac();
    void recv_aac(int port);
};


#endif //RETREAM_RTPRECV_H
