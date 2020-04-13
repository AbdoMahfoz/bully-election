#include "socket.h"

WSAData *Socket::wsaData = nullptr;
addrinfo *Socket::clientHints, *Socket::hints = nullptr;
int Socket::instanceCount = 0;
std::mutex Socket::m;

Socket::Socket(bool isTcp)
{
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    _socket = INVALID_SOCKET;
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
        hints = new addrinfo();
        ZeroMemory(hints, sizeof(*hints));
        hints->ai_family = AF_INET;
        hints->ai_socktype = (isTcp ? SOCK_STREAM : SOCK_DGRAM);
        hints->ai_protocol = (isTcp ? IPPROTO_TCP : IPPROTO_UDP);
        hints->ai_flags = AI_PASSIVE;
    }
    lock.unlock();
}
inline addrinfo *Socket::getHints()
{
    return hints;
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
Socket::~Socket()
{
    std::unique_lock<std::mutex> lock(m, std::defer_lock);
    lock.lock();
    instanceCount--;
    if (instanceCount == 0)
    {
        WSACleanup();
        delete wsaData, clientHints, hints;
    }
    lock.unlock();
}