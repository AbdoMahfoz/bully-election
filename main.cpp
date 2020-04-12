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
        char buffer[1024];
        int buffLen = client->receive(buffer, 1024);
        buffer[buffLen] = '\0';
        std::cout << buffer << '\n';
        client->send("OK", 2);
        delete client;
    }
    else
    {
        tcpSocket client;
        if (client.connect("localhost", "8234"))
        {
            client.send("Confirm", 7);
            char buffer[1024];
            int buffLen = client.receive(buffer, 1024);
            buffer[buffLen] = '\0';
            std::cout << buffer << '\n';
        }
    }
}