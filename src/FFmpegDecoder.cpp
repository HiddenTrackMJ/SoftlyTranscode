//#include "config.h"
#include "seeker/loggerApi.h"
#include "FFmpegDecoder.h"
#include <iostream>
//#include "opencv2/opencv.hpp"

FFmpegDecoder::FFmpegDecoder() {
    //avcodec_register_all();

    pkt = av_packet_alloc();
    if (!pkt) exit(1);

    /* find the MPEG-1 video decoder */
    //codec = avcodec_find_encoder_by_name("libfdk_aac");
   // codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec) {
        E_LOG("Codec not found");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        E_LOG("Could not allocate video codec context");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        E_LOG("parser not found");
        exit(1);
    }

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        E_LOG("Could not open codec");
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        E_LOG("Could not allocate video frame");
        exit(1);
    }
}

FFmpegDecoder::~FFmpegDecoder() {
    I_LOG("ffmpeg decoder uninit");
    av_parser_close(parser);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

int FFmpegDecoder::InputData(unsigned char *inputData, int inputLen,
                                    ImageMat &mImageMat) {
    int ret = 0;
    int error = -1;
    if (inputData == NULL) {
        decode(NULL, mImageMat);
    }
    
    while (inputLen > 0) {
        ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, inputData,
                               inputLen, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            E_LOG("Error while parsing");
            // exit(1);
        }
        inputData += ret;
        inputLen -= ret;

        // T_LOG("ret: {}, pkt size: {}, data: {}", ret, pkt->size, inputData);
        if (pkt->size) {

            error = decode(pkt, mImageMat);

            //printf(
            //    "size = %d .....stream_index = %d frame=%p  \n",
            //    pkt->size, pkt->stream_index, frame);
        }
    }

    return error;
}

int FFmpegDecoder::decode(AVPacket *mpkt, ImageMat &mImageMat) {
    int ret;
    ret = avcodec_send_packet(c, mpkt);
    if (ret < 0) {
        E_LOG("Error sending a packet for decoding");
        // exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(c, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;
        else if (ret < 0) {
            E_LOG("Error during decoding");
            // exit(1);
        }

        fflush(stdout);

    }
    return 0;
}
