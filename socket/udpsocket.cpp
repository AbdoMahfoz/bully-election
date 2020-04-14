#include "socket.h"
#include <ws2tcpip.h>
#include <string>

udpSocket::udpSocket() : Socket(false)
{
    this->isInMulticast = false;
    this->multiCastAddress = this->multiCastPort = nullptr;
}

void udpSocket::joinMulticast(const char *groupIp, const char *groupPort)
{
    setAddressPortReuse(true);
    bind(groupPort);
    ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(groupIp);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    int iResult = setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq));
    if (iResult != 0)
    {
        throw socketException(iResult, "when joining multicast");
    }
    if (isInMulticast)
    {
        delete[] multiCastAddress, multiCastPort;
    }
    isInMulticast = true;
    multiCastAddress = new char[strlen(groupIp)];
    strcpy(multiCastAddress, groupIp);
    multiCastPort = new char[strlen(groupPort)];
    strcpy(multiCastPort, groupPort);
}
int udpSocket::sendTo(const char *address, const char *port, const char *buffer, int n)
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
        throw socketException(WSAGetLastError(), "when udp sending");
    }
    freeaddrinfo(result);
    return sentBytes;
}
int udpSocket::sendToMultiCast(const char *buffer, int n)
{
    return sendTo(multiCastAddress, multiCastPort, buffer, n);
}
int udpSocket::receiveFrom(std::string *address, std::string *port, char *buffer, int n)
{
    if(_socket == INVALID_SOCKET)
    {
        bind("0");
    }
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
        int err = WSAGetLastError();
        if(err == WSAETIMEDOUT)
        {
            throw socketTimeoutException();
        }
        throw socketException(err, "when udp receiving");
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
std::string udpSocket::receiveFrom(std::string *sender, std::string *port, int n)
{
    char *c = new char[n];
    int recvBytes = 0;
    try
    {
        recvBytes = receiveFrom(sender, port, c, n);
    }
    catch(socketTimeoutException e)
    {
        delete[] c;
        throw e;
    }
    std::string res(c, recvBytes);
    delete[] c;
    return res;
}
std::stringstream udpSocket::receiveStreamFrom(std::string *address, std::string *port, int n)
{
    return std::stringstream(receiveFrom(address, port, n), std::ios_base::app|std::ios_base::in|std::ios_base::out);
}
udpSocket::~udpSocket()
{
    delete[] multiCastAddress, multiCastPort;
}