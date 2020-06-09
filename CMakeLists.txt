# CMakeList.txt: transcode 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("transcode")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

include(./.SET_LOCAL_PATH.cmake)

#=======================ffmpeg=============================

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

find_path(AVFILTER_INCLUDE_DIR libavfilter/avfilter.h)
find_library(AVFILTER_LIBRARY avfilter)

#find_path(POSTPROC_INCLUDE_DIR libpostproc/postprocess.h)
#find_library(POSTPROC_LIBRARY postproc)

find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)

#=======================ffmpeg=============================


#=======================libfdk-aac=========================
find_library(FDK_LIBRARY fdk-aac)

message(STATUS "FDK library status:")
message(STATUS "    libraries: ${FDK_LIBRARY}")
message(STATUS "    include path: ${FDK_INCLUDE_DIR}")

#=======================libfdk-aac=========================


#=======================jrtplib============================

if (WIN32) # Windows

  find_library(JRTP_LIBRARY jrtplib)
  find_library(JTHREAD_LIBRARY jthread)
  #include_directories(${JTHREAD_INCLUDE_PATH})
  #link_libraries( ${JTHREAD_LIBRARY})

  #set(CMAKE_CURRENT_BINARY_DIR "D:/Study/Scala/VSWS/transcode/out/build/x64-Release1")

  message(STATUS "JRTP library status:")
  message(STATUS "jrtplib libraries: ${JRTP_LIBRARY}")
  message(STATUS "jrtplib include path: ${JRTPLIB_INCLUDE_PATH}")
  message(STATUS "jthread libraries: ${JTHREAD_LIBRARY}")
  message(STATUS "jthread include path: ${JTHREAD_INCLUDE_PATH}")
  message(CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

ELSE() # MAC or linux

  find_library(JRTP_LIBRARY jrtp)
  #find_library(JTHREAD_LIBRARY jthread)

endif()



#=======================jrtplib============================

add_executable (${PROJECT_NAME}
       "src/main.cpp" 
       "src/FFmpegDecoder.cpp"
        "src/remuxing.cpp" 
        "src/fdk_enc.cpp" 
        "src/fdk_dec.cpp" 
         "src/test_enc.cpp" 
         "src/test_dec.cpp" 
         "src/rtpRecv.cpp"
         "include/fdk_demo.hpp"
         "src/rtpSender.cpp" 
         "src/RtpSess.cpp" 
         "src/encoder.cpp" 
         "src/hls_encoder.cpp" 
         "src/muxing.cpp" 
         "src/dash_encoder.cpp")

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${JTHREAD_INCLUDE_PATH}
        ${JRTPLIB_INCLUDE_PATH}
        ${AVCODEC_INCLUDE_DIR}
        ${AVFORMAT_INCLUDE_DIR}
        ${AVUTIL_INCLUDE_DIR}
        ${AVDEVICE_INCLUDE_DIR}
        ${AVFILTER_INCLUDE_DIR}
        ${SWRESAMPLE_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR}
        ${FDK_INCLUDE_DIR}
        )

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${JRTP_LIBRARY}
        ${JTHREAD_LIBRARY}
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${AVDEVICE_LIBRARY}
        ${AVFILTER_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${FDK_LIBRARY}
        )

