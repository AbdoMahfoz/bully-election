#include "unit.h"

void unit::master(tcpSocket* socket)
{
    try
    {
        while(controlExists)
        {
            socket->send("ADD 1 3");
            int n = std::stoi(socket->receive());
            if(n != 4)
            {
                unit::log("Control ERROR");
            }
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
    catch(std::exception e)
    {
        unit::log("ERROR in master(): " + std::string(e.what()));
    }
    socket->forceClose();
    delete socket;
}
void unit::slave(tcpSocket* socket)
{
    try
    {
        socket->setTimeOut(100);
        while(controlExists)
        {
            try
            {
                std::string cmd;
                int a, b;
                socket->receiveStream() >> cmd >> a >> b;
                if(cmd == "ADD")
                {
                    unit::log("Received addition command: " + std::to_string(a) + " + " + std::to_string(b));
                    socket->send(std::to_string(a + b).c_str());
                }
                else
                {
                    socket->send(std::to_string(-1).c_str());
                }
            }
            catch(socketTimeoutException)
            {
                continue;
            }
        }
    }
    catch(std::exception e)
    {
        unit::log("ERROR in slave(): " + std::string(e.what()));
    }
    socket->forceClose();
    delete socket;
}
void unit::launchMaster(tcpSocket* socket)
{
    controlExists = true;
    controlSockets[socket] = new std::thread(master, socket);
}
void unit::launchSlave(tcpSocket* socket)
{
    controlExists = true;
    slaveThread = new std::thread(slave, socket);
}
void unit::terminator()
{
    std::unique_lock<std::mutex> lock(controlMutex);
    unit::log("Termnating control threads");
    controlExists = false;
    for(auto p : controlSockets)
    {
        p.second->join();
        delete p.second;
    }
    controlSockets.clear();
    if(slaveThread != nullptr)
    {
        slaveThread->join();
        delete slaveThread;
        slaveThread = nullptr;
    }
}
void unit::terminateControlThreads()
{
    std::thread* t = new std::thread(terminator);
    t->detach();
    delete t;
}