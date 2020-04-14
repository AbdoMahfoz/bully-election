#include "socket.h"

WSAData *Socket::wsaData = nullptr;
addrinfo *Socket::tcpHints, *Socket::udpHints = nullptr;
int Socket::instanceCount = 0;
std::mutex Socket::m;

Socket::Socket(bool isTcp)
{
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    _socket = INVALID_SOCKET;
    this->isTcp = isTcp;
    this->reuseAddress = this->broadcast = false;
    this->timeout = 0;
    lock.lock();
    instanceCount++;
    if (instanceCount == 1)
    {
        wsaData = new WSAData();
        int iResult = WSAStartup(MAKEWORD(2, 2), wsaData);
        if (iResult != 0)
        {
            throw socketException(iResult, "in initialization");
        }
        tcpHints = new addrinfo();
        ZeroMemory(tcpHints, sizeof(*tcpHints));
        tcpHints->ai_family = AF_INET;
        tcpHints->ai_socktype = SOCK_STREAM;
        tcpHints->ai_protocol = IPPROTO_TCP;
        tcpHints->ai_flags = AI_PASSIVE;
        udpHints = new addrinfo();
        ZeroMemory(udpHints, sizeof(*udpHints));
        udpHints->ai_family = AF_INET;
        udpHints->ai_socktype = SOCK_DGRAM;
        udpHints->ai_protocol = IPPROTO_UDP;
        udpHints->ai_flags = AI_PASSIVE;
    }
    lock.unlock();
}
inline addrinfo *Socket::getHints()
{
    return isTcp ? tcpHints : udpHints;
}
void Socket::activateop(bool value, int op)
{
    char val = (value ? '1' : '0');
    int iResult = setsockopt(_socket, SOL_SOCKET, op, &val, sizeof(val));
    if (iResult != 0)
    {
        throw socketException(iResult, "when setting socket options");
    }
}
void Socket::activatePortReuse()
{
    if (reuseAddress)
    {
        activateop(reuseAddress, SO_REUSEADDR);
    }
}
void Socket::activateBroadcast()
{
    if (broadcast)
    {
        activateop(broadcast, SO_BROADCAST);
    }
}
void Socket::activateTimeout()
{
    if(timeout > 0)
    {    
        int iResult = setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        if (iResult != 0)
        {
            throw socketException(iResult, "when setting timeout");
        }
    }
}
void Socket::setAddressPortReuse(bool value)
{
    reuseAddress = value;
}
void Socket::setBroadcast(bool value)
{
    broadcast = value;
}
void Socket::setTimeOut(int timeout)
{
    this->timeout = timeout;
}
void Socket::bind(const char *port)
{
    addrinfo *result;
    int iResult = getaddrinfo(NULL, port, getHints(), &result);
    if (iResult != 0)
    {
        throw socketException(iResult, "when creating binding socket");
    }
    while (true)
    {
        _socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (_socket == INVALID_SOCKET)
        {
            throw socketException(WSAGetLastError(), "when creating binding socket");
        }
        activateBroadcast();
        activatePortReuse();
        activateTimeout();
        iResult = ::bind(_socket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            if (result->ai_next != NULL)
            {
                result = result->ai_next;
                continue;
            }
        }
        break;
    }
    freeaddrinfo(result);
    if (_socket == INVALID_SOCKET)
    {
        throw socketException(WSAGetLastError(), "when creating binding socket");
    }
}
Socket::~Socket()
{
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    lock.lock();
    instanceCount--;
    if (instanceCount == 0)
    {
        WSACleanup();
        delete wsaData, tcpHints, udpHints;
    }
    lock.unlock();
}
void Socket::getSocketBinding(char** address, int* port)
{
    sockaddr_in sin;
    int addrlen = sizeof(sin);
    int iResult = getsockname(_socket, (sockaddr *)&sin, &addrlen);
    if(iResult == 0)
    {
        if(port != NULL)
        {
            *port = ntohs(sin.sin_port);
        }
        if(address != NULL)
        {
            *address = new char[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sin.sin_addr, *address, INET_ADDRSTRLEN);
        }
    }
    else
    {
        throw socketException(iResult, "when getting socket binding");
    }
}
std::string Socket::getPort()
{
    int port;
    getSocketBinding(NULL, &port);
    return std::to_string(port);
}
std::string Socket::getAddress()
{
    char* address;
    getSocketBinding(&address, NULL);
    std::string res(address);
    delete[] address;
    return res;
}