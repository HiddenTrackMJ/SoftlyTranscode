// transcode.cpp: 定义应用程序的入口点。
//

#include "seeker/logger.h"
#include "FFmpegDecoder.h"

#define First 1

#ifdef First
int main(int argc, char *argv[]) {
  seeker::Logger::init();

  I_LOG("This is TransCode");


  ImageMat mImageMat;
  FFmpegDecoder decoder;




  return 0;
}

#else 
int main(int argc, char **argv) {
  seeker::Logger::init();
  const char *filename, *outfilename;
  FILE *f;
  uint8_t inbuf[4096 + AV_INPUT_BUFFER_PADDING_SIZE];
  int ret;
  int data_size;

  filename = "D:/Download/Videos/LadyLiu/hello2.h264";  // 输入的264文件
  outfilename = "D:/Download/Videos/LadyLiu/hhh.h264";

  /* set end of buffer to 0 (this ensures
  that no overreading happens for
   * damaged MPEG streams) */
  memset(inbuf + 4096, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  FFmpegDecoder decoder;
  ImageMat mImageMat;

  f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Could not open %s\n", filename);
    exit(1);
  }

  while (!feof(f)) {
    /* read raw data from the input file */
    data_size = fread(inbuf, 1, 4096, f);
    if (!data_size) break;

    /*  printf("recv %d , data: %s,\n", data_size,
            inbuf);*/

    int retC = decoder.InputData(inbuf, data_size, mImageMat);

  }

  /* flush the decoder */
  int t_w = 0;
  int t_h = 0;
  // decode(c, frame, NULL, outfilename, mImageMat);

  decoder.InputData(NULL, 0, mImageMat);

  fclose(f);

  return -100;
}
#endif