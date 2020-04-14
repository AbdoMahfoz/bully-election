#ifndef SOCKET_CLASS
#define SOCKET_CLASS

#include <winsock2.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

class socketException : public std::exception
{
public:
    int iResult;
    char *msg;
    socketException(int iResult, char *msg = nullptr) : 
    std::exception((std::to_string(iResult) + " " + msg).c_str())
    {
        this->iResult = iResult;
        this->msg = msg;
    }
};

class socketTimeoutException : public socketException
{
public:
    socketTimeoutException() : socketException(10060, "Timeout"){}
};

class Socket
{
private:
    bool isTcp, reuseAddress, broadcast, updateTimeout, updateReuse, updateBroadcast;
    int timeout;
    void activateop(bool val, int op);
    void activatePortReuse();
    void activateBroadcast();
    void activateTimeout();
    void getSocketBinding(char** address, int* port);
protected:
    Socket(bool isTcp);
    addrinfo *getHints();
    SOCKET _socket;
public:
    virtual ~Socket();
    void setAddressPortReuse(bool value);
    void setBroadcast(bool value);
    void setTimeOut(int milliseconds);
    void bind(const char *port);
    std::string getPort();
    std::string getAddress();
};

class tcpSocket : public Socket
{
private:
    bool isConnected;
public:
    tcpSocket();
    ~tcpSocket();
    tcpSocket *accept();
    bool connect(const char *address, const char *port);
    int send(const char *buffer, int n = -1);
    int receive(char *buffer, int n);
    std::string receive(int n = 1024);
    void close();
    void forceClose();
};

class udpSocket : public Socket
{
private:
    bool isInMulticast;
    char *multiCastAddress, *multiCastPort;
public:
    udpSocket();
    ~udpSocket();
    void joinMulticast(const char *groupIp, const char *groupPort);
    int sendToMultiCast(const char *buffer, int n = -1);
    int sendTo(const char *address, const char *port, const char *buffer, int n = -1);
    int receiveFrom(std::string *address, std::string *port, char *buffer, int n);
    std::string receiveFrom(std::string *address, std::string *port, int n = 1024);
};

#endif