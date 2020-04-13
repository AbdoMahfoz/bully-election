#include <iostream>
#include "controller.h"
#include "socket/socket.h"

int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        controller::main();
    }
    else
    {
        udpSocket s;
        while(true)
        {
            s.sendTo("239.125.125.125", "8233", argv[1]);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}