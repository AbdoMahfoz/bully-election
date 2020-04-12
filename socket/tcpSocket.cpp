#include "socket.h"

WSAData *tcpSocket::wsaData = nullptr;
addrinfo *tcpSocket::hints = nullptr;
int tcpSocket::instanceCount = 0;

tcpSocket::tcpSocket(bool isServer)
{
    instanceCount++;
    _socket = INVALID_SOCKET;
    this->isServer = isServer;
    if (instanceCount == 1)
    {
        wsaData = new WSAData();
        int iResult = WSAStartup(MAKEWORD(2, 2), wsaData);
        if (iResult != 0)
        {
            throw socketException(iResult);
        }
        hints = new addrinfo();
        ZeroMemory(hints, sizeof(*hints));
        hints->ai_family = AF_UNSPEC;
        hints->ai_socktype = SOCK_STREAM;
        hints->ai_protocol = IPPROTO_TCP;
        if (isServer)
        {
            hints->ai_flags = AI_PASSIVE;
        }
    }
}
void tcpSocket::bind(const char *port)
{
    addrinfo *result;
    int iResult = getaddrinfo(NULL, port, hints, &result);
    if (iResult != 0)
    {
        throw socketException(iResult);
    }
    _socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (_socket == INVALID_SOCKET)
    {
        throw socketException(WSAGetLastError());
    }
    iResult = ::bind(_socket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    if (iResult == SOCKET_ERROR)
    {
        closesocket(_socket);
        throw socketException(WSAGetLastError());
    }
}
tcpSocket *tcpSocket::accept()
{
    if (listen(_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError());
    }
    SOCKET resSocket = INVALID_SOCKET;
    resSocket = ::accept(_socket, NULL, NULL);
    if (resSocket == INVALID_SOCKET)
    {
        throw socketException(WSAGetLastError());
    }
    tcpSocket *res = new tcpSocket();
    res->_socket = resSocket;
    return res;
}
bool tcpSocket::connect(const char *address, const char *port)
{
    addrinfo *result;
    int iResult = getaddrinfo(address, port, hints, &result);
    if (iResult != 0)
    {
        throw socketException(iResult);
    }
    while (true)
    {
        _socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (_socket == INVALID_SOCKET)
        {
            throw socketException(WSAGetLastError());
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
            throw socketException(WSAGetLastError());
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
    iResult = ::send(_socket, buffer, n, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError());
    }
    return iResult;
}
int tcpSocket::receive(char *buffer, int n)
{
    int iResult;
    iResult = recv(_socket, buffer, n, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError());
    }
    return iResult;
}
tcpSocket::~tcpSocket()
{
    instanceCount--;
    if (_socket != INVALID_SOCKET)
    {
        close();
    }
    if (instanceCount == 0)
    {
        std::cout << "WINSOCK CLEAN\n";
        WSACleanup();
        delete wsaData, hints;
    }
}