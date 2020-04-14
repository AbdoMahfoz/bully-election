#include "socket.h"
#include <ws2tcpip.h>
#include <string>

tcpSocket::tcpSocket() : Socket(true)
{
    this->isConnected = false;
    this->targetAddress = this->targetPort = "";
}

tcpSocket *tcpSocket::accept()
{
    if (listen(_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        throw socketException(WSAGetLastError(), "when listening");
    }
    SOCKET resSocket = INVALID_SOCKET;
    sockaddr_in sAddr;
    ZeroMemory(&sAddr, sizeof(sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = 0;
    sAddr.sin_addr.S_un.S_addr = inet_addr(NULL);
    int sAddrLen = sizeof(sAddr);
    resSocket = ::accept(_socket, (sockaddr *)&sAddr, &sAddrLen);
    if (resSocket == INVALID_SOCKET)
    {
        throw socketException(WSAGetLastError(), "when accepting connection");
    }
    tcpSocket *res = new tcpSocket();
    res->isConnected = true;
    res->_socket = resSocket;
    char *peer_addr_str = new char[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sAddr.sin_addr, peer_addr_str, INET_ADDRSTRLEN);
    res->targetAddress = std::string(peer_addr_str);
    delete[] peer_addr_str;
    res->targetPort = std::to_string(ntohs(sAddr.sin_port));
    return res;
}
bool tcpSocket::connect(const char *address, const char *port)
{
    addrinfo *result;
    int iResult = getaddrinfo(address, port, getHints(), &result);
    if (iResult != 0)
    {
        throw socketException(iResult, "when resolving connect address");
    }
    while (true)
    {
        if(_socket == INVALID_SOCKET)
        {
            bind("0");
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
        isConnected = false;
        return false;
    }
    isConnected = true;
    targetAddress = std::string(address);
    targetPort = std::string(port);
    return true;
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
        int err = WSAGetLastError();
        if(err == WSAETIMEDOUT)
        {
            throw socketTimeoutException();
        }
        throw socketException(err, "when receiving data");
    }
    return iResult;
}
std::string tcpSocket::receive(int n)
{
    char *c = new char[n];
    int recvBytes = 0;
    try
    {
        recvBytes = receive(c, n);
    }
    catch(socketTimeoutException e)
    {
        delete[] c;
        throw e;
    }
    catch(socketException e)
    {
        delete[] c;
        throw e;
    }
    std::string res(c, recvBytes);
    delete[] c;
    return res;
}
std::stringstream tcpSocket::receiveStream(int n)
{
    return std::stringstream(receive(n), std::ios_base::app|std::ios_base::in|std::ios_base::out);
}
const std::string& tcpSocket::getTargetAddress()
{
    return targetAddress;
}
const std::string& tcpSocket::getTargetPort()
{
    return targetPort;
}
void tcpSocket::close()
{
    if(isConnected)
    {
        isConnected = false;
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
void tcpSocket::forceClose()
{
    isConnected = false;
    closesocket(_socket);
    _socket = INVALID_SOCKET;
}
tcpSocket::~tcpSocket()
{
    if (_socket != INVALID_SOCKET)
    {
        close();
    }
}