#include "unit.h"
#include <condition_variable>

std::mutex sleepMutex;
std::condition_variable sleep;

void unit::master(tcpSocket* socket)
{
    socket->setTimeOut(100);
    std::unique_lock<std::mutex> sleepLock(sleepMutex, std::defer_lock);
    try
    {
        while(controlExists)
        {
            socket->send("ADD 1 3");
            int n;
            while(true)
            {
                try
                {
                    n = std::stoi(socket->receive());
                }
                catch(socketTimeoutException)
                {
                    if(!controlExists)
                    {
                        socket->forceClose();
                        delete socket;
                        return;
                    }
                }
            }
            sleepLock.lock();
            sleep.wait_for(sleepLock, std::chrono::seconds(3));
            sleepLock.unlock();
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
    controlThreads.push_back(new std::thread(master, socket));
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
    sleep.notify_all();
    for(auto p : controlThreads)
    {
        p->join();
        delete p;
    }
    controlThreads.clear();
    if(slaveThread != nullptr)
    {
        slaveThread->join();
        delete slaveThread;
        slaveThread = nullptr;
    }
}
void unit::terminateControlThreads()
{
    std::thread t(terminator);
    t.detach();
}