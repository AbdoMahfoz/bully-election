#ifndef SOCKET_CLASS
#define SOCKET_CLASS

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

class socketException : public std::exception
{
public:
    int iResult;
    char *msg;
    socketException(int iResult, char *msg = nullptr) : std::exception((std::to_string(iResult) + " " + msg).c_str())
    {
        this->iResult = iResult;
        this->msg = msg;
    }
};

class Socket
{
private:
    static int instanceCount;
    static WSAData *wsaData;
    static addrinfo *serverHints, *clientHints;
    static std::mutex m;
    SOCKET _socket;
    bool isServer;
    bool isTcp;

public:
    Socket(bool isTcp, bool isServer = false);
    ~Socket();
    inline addrinfo *getHints();
    //tcp server
    void bind(const char *port);
    Socket *accept();
    //tcp client
    bool connect(const char *address, const char *port);
    int send(const char *buffer, int n = -1);
    int receive(char *buffer, int n);
    std::string receive(int n = 1024);
    //tcp common
    void close();
    //udp
    int sendTo(const char *address, const char *port, const char *buffer, int n = -1);
    int receiveFrom(std::string *address, std::string *port, char *buffer, int n);
    std::string receiveFrom(std::string *address, std::string *port, int n = 1024);
};

#endif