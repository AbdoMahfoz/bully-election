#include "unit.h"

void unit::communicate(unitData data, tcpSocket* socket)
{
    try
    {
        socket->setTimeOut(2000);
        std::unique_lock<std::mutex> lock(othersMutex, std::defer_lock);
        while(true)
        {
            try
            {
                socket->send("ALIVE");
                if(socket->receive() != "ALIVE")
                {
                    break;
                }
            }
            catch(socketException)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        socket->forceClose();
        delete socket;
        lock.lock();
        auto itr = others.find(data);
        itr->second->detach();
        delete itr->second;
        others.erase(itr);
        lock.unlock();
        unit::log("Lost connection to " + std::to_string(data.id));
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR in communicate(): ") + e.what());
    }
}