#include "socket.h"

WSAData *tcpSocket::wsaData = nullptr;
int tcpSocket::instanceCount = 0;

tcpSocket::tcpSocket()
{
    instanceCount++;
    if (instanceCount == 1)
    {
        wsaData = new WSAData();
        int iResult = WSAStartup(MAKEWORD(2, 2), wsaData);
        if (iResult != 0)
        {
            std::cout << "WSAStartup failed: " << iResult << "\n";
            exit(1);
        }
        hints = new addrinfo();
        ZeroMemory(hints, sizeof(*hints));
        hints->ai_family = AF_UNSPEC;
        hints->ai_socktype = SOCK_STREAM;
        hints->ai_protocol = IPPROTO_TCP;
    }
}
bool tcpSocket::connect(char *address, char *port)
{
}
void tcpSocket::send(char *buffer, int n)
{
}
void tcpSocket::receive(char *buffer, int n)
{
}
tcpSocket::~tcpSocket()
{
    instanceCount--;
    if (instanceCount == 0)
    {
        delete wsaData, hints;
    }
}