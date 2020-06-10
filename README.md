### AAC编解码尝试

 - [x] 配置libfdk-aac ：[参考教程](http://discuss.seekloud.org:50080/d/262-libfdk-acc-windows)   
 - [x] libfdk-aac demo [参考代码](https://github.com/akanchi/aac-example)
 - [x] aac解码
 - [x] aac编码
 - [x] hls/dash转码


 adts -> LATM  
 参考;
 https://blog.csdn.net/yingyemin/article/details/76570576
 https://www.cnblogs.com/ranson7zop/p/7199628.html 
 https://github.com/FFmpeg/FFmpeg/search?q=LATM&unscoped_q=LATM
 https://github.com/FFmpeg/FFmpeg/search?q=adts&unscoped_q=adts

#### 注：编码demo的输入文件必须是严格的.wav文件，不能是其他格式修改后缀得到的，我测试是将.flac用ffmpeg转码成.wav的。
