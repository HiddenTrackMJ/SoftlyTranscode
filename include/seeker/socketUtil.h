#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#else
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <arpa/inet.h>
#include <sys/socket.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#define OK 0
#define ERR -1
#define SOCKET_VERSION MAKEWORD(2, 2)


namespace seeker {

class SocketUtil {
 public:
  static sockaddr_in createAddr(int port, const std::string& host = "") {
    sockaddr_in addr = {0};
    const char * hostData = host.empty() ? nullptr : host.c_str();
    addr.sin_family = AF_INET;
    setSocketAddr(&addr, hostData, port);
    return addr;
  }

  static void cleanWSA() {
#ifdef _WIN32
    WSACleanup();
#else
    // do nothing.
#endif
  }


  static void closeSocket(SOCKET s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
  }


  static void setSocketAddr(sockaddr_in* addr, const char* ip, const int port = -1) {
    if (ip == nullptr) {
#ifdef _WIN32
      (*addr).sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
      (*addr).sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    } else {
#ifdef _WIN32
      (*addr).sin_addr.S_un.S_addr = inet_addr(ip);
#else
      (*addr).sin_addr.s_addr = inet_addr(ip);
#endif
    }

    if(port != -1) {
      (*addr).sin_port = htons(port);
    }

  }


  static void startupWSA() {
#ifdef _WIN32
    WSADATA wsaData = {};
    if (WSAStartup(SOCKET_VERSION, &wsaData) != OK) {
      throw std::runtime_error("WSAStartup failed.");
    }
#else
    //nothing to do.
#endif
  }
};



}  // namespace seeker
