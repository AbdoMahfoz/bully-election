#include "socket.h"

WSAData *Socket::wsaData = nullptr;
addrinfo *Socket::clientHints, *Socket::serverHints = nullptr;
int Socket::instanceCount = 0;
std::mutex Socket::m;

Socket::Socket(bool isTcp, bool isServer)
{
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    _socket = INVALID_SOCKET;
    this->isServer = isServer;
    this->isTcp = isTcp;
    this->reuseAddress = this->broadcast = false;
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
        int sockType = (isTcp ? SOCK_STREAM : SOCK_DGRAM);
        int protocol = (isTcp ? IPPROTO_TCP : IPPROTO_UDP);
        clientHints = new addrinfo();
        ZeroMemory(clientHints, sizeof(*clientHints));
        clientHints->ai_family = AF_INET;
        clientHints->ai_socktype = sockType;
        clientHints->ai_protocol = protocol;
        serverHints = new addrinfo();
        ZeroMemory(serverHints, sizeof(*serverHints));
        serverHints->ai_family = AF_INET;
        serverHints->ai_socktype = sockType;
        serverHints->ai_protocol = protocol;
        serverHints->ai_flags = AI_PASSIVE;
    }
    lock.unlock();
}
inline addrinfo *Socket::getHints()
{
    return isServer ? serverHints : clientHints;
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
void Socket::setAddressPortReuse(bool value)
{
    reuseAddress = value;
}
void Socket::setBroadcast(bool value)
{
    broadcast = value;
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
Socket *Socket::accept()
{
    if (listen(_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError(), "when listening");
    }
    SOCKET resSocket = INVALID_SOCKET;
    resSocket = ::accept(_socket, NULL, NULL);
    if (resSocket == INVALID_SOCKET)
    {
        throw socketException(WSAGetLastError(), "when accepting connection");
    }
    Socket *res = new Socket(true);
    res->_socket = resSocket;
    return res;
}
bool Socket::connect(const char *address, const char *port)
{
    addrinfo *result;
    int iResult = getaddrinfo(address, port, getHints(), &result);
    if (iResult != 0)
    {
        throw socketException(iResult, "when connecting");
    }
    while (true)
    {
        _socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (_socket == INVALID_SOCKET)
        {
            throw socketException(WSAGetLastError(), "when connecting");
        }
        iResult = ::connect(_socket, result->ai_addr, (int)result->ai_addrlen);
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
        return false;
    }
    return true;
}
void Socket::close()
{
    if (isTcp && !isServer)
    {
        int iResult = shutdown(_socket, SD_SEND);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            throw socketException(WSAGetLastError(), "when shutting down");
        }
        char buffer[1024];
        do
        {
            iResult = receive(buffer, 1024);
        } while (iResult > 0);
    }
    closesocket(_socket);
    _socket = INVALID_SOCKET;
}
int Socket::send(const char *buffer, int n)
{
    int iResult;
    if (n == -1)
    {
        n = strlen(buffer);
    }
    iResult = ::send(_socket, buffer, n, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError(), "when sending data");
    }
    return iResult;
}
int Socket::receive(char *buffer, int n)
{
    int iResult;
    iResult = recv(_socket, buffer, n, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError(), "when receiving data");
    }
    return iResult;
}
std::string Socket::receive(int n)
{
    char *c = new char[n];
    int recvBytes = receive(c, n);
    std::string res(c, recvBytes);
    delete[] c;
    return res;
}
int Socket::sendTo(const char *address, const char *port, const char *buffer, int n)
{
    if (_socket == INVALID_SOCKET)
    {
        bind("0");
    }
    addrinfo *result;
    int iResult = getaddrinfo(address, port, getHints(), &result);
    if (iResult != 0)
    {
        throw socketException(iResult, "when udp sending");
    }
    if (n == -1)
    {
        n = strlen(buffer);
    }
    int sentBytes = sendto(_socket, buffer, n, 0, result->ai_addr, (int)result->ai_addrlen);
    if (sentBytes == SOCKET_ERROR)
    {
        throw socketException(sentBytes, "when udp sending");
    }
    freeaddrinfo(result);
    return sentBytes;
}
int Socket::receiveFrom(std::string *address, std::string *port, char *buffer, int n)
{
    if (n == -1)
    {
        n = strlen(buffer);
    }
    sockaddr_in sAddr;
    ZeroMemory(&sAddr, sizeof(sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = 0;
    sAddr.sin_addr.S_un.S_addr = inet_addr(NULL);
    int sAddrLen = sizeof(sAddr);
    int recvBytes = recvfrom(_socket, buffer, n, 0, (sockaddr *)&sAddr, &sAddrLen);
    if (recvBytes == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError(), "when udp receiving");
    }
    if (address != NULL)
    {
        char *peer_addr_str = new char[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sAddr.sin_addr, peer_addr_str, INET_ADDRSTRLEN);
        *address = std::string(peer_addr_str);
        delete[] peer_addr_str;
    }
    if (port != NULL)
    {
        *port = std::to_string(ntohs(sAddr.sin_port));
    }
    return recvBytes;
}
std::string Socket::receiveFrom(std::string *sender, std::string *port, int n)
{
    char *c = new char[n];
    int recvBytes = receiveFrom(sender, port, c, n);
    std::string res(c, recvBytes);
    delete[] c;
    return res;
}
Socket::~Socket()
{
    if (_socket != INVALID_SOCKET)
    {
        close();
    }
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    lock.lock();
    instanceCount--;
    if (instanceCount == 0)
    {
        WSACleanup();
        delete wsaData, clientHints, serverHints;
    }
    lock.unlock();
}