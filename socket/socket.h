#ifndef SOCKET_CLASS
#define SOCKET_CLASS

#define _WIN32_WINNT 0x0501

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

class tcpSocket
{
private:
    static int instanceCount;
    static WSAData *wsaData;
    struct addrinfo *hints;

public:
    tcpSocket();
    bool connect(char *address, char *port);
    void send(char *buffer, int n);
    void receive(char *buffer, int n);
    ~tcpSocket();
};

#endif