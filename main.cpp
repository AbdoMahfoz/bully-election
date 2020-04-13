#include "socket/socket.h"
#include <thread>
#include <chrono>

void tcpTest()
{
    int n;
    std::cin >> n;
    if (n == 0)
    {
        Socket server(true, true);
        server.bind("8234");
        Socket *client = server.accept();
        std::cout << client->receive() << '\n';
        client->send("OK");
        delete client;
    }
    else
    {
        Socket client(true);
        if (client.connect("localhost", "8234"))
        {
            client.send("Confirm");
            std::cout << client.receive() << '\n';
        }
    }
}

void udpTest()
{
    int n;
    std::cin >> n;
    if (n == 0)
    {
        Socket server(false, true);
        std::string sender, port;
        server.bind("8234");
        std::string res = server.receiveFrom(&sender, &port);
        std::cout << sender << ':' << port << " = " << res << '\n';
        server.sendTo(sender.c_str(), port.c_str(), "Yes");
    }
    else
    {
        Socket client(false);
        client.sendTo(NULL, "8234", "Confirm");
        std::cout << client.receiveFrom(NULL, NULL) << '\n';
    }
}

int main()
{
    try
    {
        tcpTest();
    }
    catch (socketException e)
    {
        std::cout << e.iResult << ' ' << e.msg << '\n';
    }
}