#ifndef SOCKET_CLASS
#define SOCKET_CLASS

//#define _WIN32_WINNT 0x0501

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

class socketException : public std::exception
{
public:
    int iResult;
    char *msg;
    socketException(int iResult, char *msg = nullptr)
    {
        this->iResult = iResult;
        this->msg = msg;
    }
};

class tcpSocket
{
private:
    static int instanceCount;
    static WSAData *wsaData;
    static addrinfo *hints;
    SOCKET _socket;
    bool isServer;

public:
    tcpSocket(bool isServer = false);
    ~tcpSocket();
    //server
    void bind(const char *port);
    tcpSocket *accept();
    //client
    bool connect(const char *address, const char *port);
    void close();
    int send(const char *buffer, int n);
    int receive(char *buffer, int n);
};

#endif