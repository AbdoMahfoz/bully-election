#include "socket.h"

WSAData *tcpSocket::wsaData = nullptr;
addrinfo *tcpSocket::clientHints, *tcpSocket::serverHints = nullptr;
int tcpSocket::instanceCount = 0;
std::mutex tcpSocket::m;

tcpSocket::tcpSocket(bool isServer)
{
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    _socket = INVALID_SOCKET;
    this->isServer = isServer;
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
        clientHints = new addrinfo();
        ZeroMemory(clientHints, sizeof(*clientHints));
        clientHints->ai_family = AF_UNSPEC;
        clientHints->ai_socktype = SOCK_STREAM;
        clientHints->ai_protocol = IPPROTO_TCP;
        serverHints = new addrinfo();
        ZeroMemory(serverHints, sizeof(*serverHints));
        serverHints->ai_family = AF_UNSPEC;
        serverHints->ai_socktype = SOCK_STREAM;
        serverHints->ai_protocol = IPPROTO_TCP;
        serverHints->ai_flags = AI_PASSIVE;
    }
    lock.unlock();
}
inline addrinfo *tcpSocket::getHints()
{
    return isServer ? serverHints : clientHints;
}
void tcpSocket::bind(const char *port)
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
tcpSocket *tcpSocket::accept()
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
    tcpSocket *res = new tcpSocket();
    res->_socket = resSocket;
    return res;
}
bool tcpSocket::connect(const char *address, const char *port)
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
void tcpSocket::close()
{
    if (!isServer)
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
int tcpSocket::send(const char *buffer, int n)
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
int tcpSocket::receive(char *buffer, int n)
{
    int iResult;
    iResult = recv(_socket, buffer, n, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError(), "when receiving data");
    }
    return iResult;
}
std::string tcpSocket::receive(int n)
{
    char *c = new char[n];
    int recvBytes = receive(c, n);
    std::string res(c, recvBytes);
    return res;
}
tcpSocket::~tcpSocket()
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