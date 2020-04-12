#include "socket/socket.h"

int main()
{
    int n;
    std::cin >> n;
    if (n == 0)
    {
        tcpSocket server(true);
        server.bind("8234");
        tcpSocket *client = server.accept();
        std::cout << client->receive() << '\n';
        client->send("OK");
        delete client;
    }
    else
    {
        tcpSocket client;
        if (client.connect("localhost", "8234"))
        {
            client.send("Confirm");
            std::cout << client.receive() << '\n';
        }
    }
}