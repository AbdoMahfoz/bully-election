#include "unit.h"
#include <sstream>
#include <iostream>

void unit::communicate(unitData data, tcpSocket* socket)
{
    bool isCoord = false;
    try
    {
        socket->setTimeOut(1000);
        std::unique_lock<std::mutex> coordLock(coordMutex, std::defer_lock);
        while(*data.killSwitch)
        {
            coordLock.lock();
            isCoord = (data.id == coordId);
            coordLock.unlock();
            try
            {
                socket->send("ALIVE");
                std::string msg = socket->receive();
                if(msg != "ALIVE")
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
        if(coordLock.owns_lock()) coordLock.unlock();
        socket->forceClose();
        delete socket;
        if(*(data.killSwitch))
        {
            if(isCoord)
            {
                unit::log("Lost connection to coordinator, reinitiating elections");
                initElections();
            }
            else
            {
                unit::log("Lost connection to " + std::to_string(data.id));
            }
            initDiscover();
            std::thread t(killConnection, data.id);
            t.detach();
        }
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR in communicate(): ") + e.what());
    }
}
void unit::initElections()
{
    startElections = true;
}
void unit::elections()
{
    int id = std::stoi(myId);
    udpSocket socket;
    socket.setTimeOut(1000);
    socket.joinMulticast(UNIT_MULTICAST_IP, UNIT_ELECTIONS_PORT);
    std::stringstream ss;
    std::unique_lock<std::mutex> lock(coordMutex, std::defer_lock);
    bool electing = false;
    bool discoverAfterElections = false;
    int lastCord;
    while(true)
    {
        std::string msg;
        if(!startElections)
        {
            try
            {
                msg = socket.receiveFrom(NULL, NULL);
            }
            catch(socketTimeoutException)
            {
                continue;
            }
            if(msg.find("VICTORY ") == 0)
            {
                int newCoordId = std::stoi(msg.substr(msg.rfind(' ') + 1));
                if(discoverAfterElections)
                {
                    initDiscover();
                    discoverAfterElections = false;
                }
                lock.lock();
                if(newCoordId != coordId)
                {
                    coordId = newCoordId;
                    lock.unlock();
                    if(lastCord != coordId)
                    {
                        if(coordId != std::stoi(myId))
                        {
                            unit::log("New coordinator: " + std::to_string(coordId));
                        }
                    }
                }
                else
                {
                    lock.unlock();
                }
            }
        }
        if(startElections || msg.find("ELECT ") == 0)
        {
            lock.lock();
            lastCord = coordId;
            coordId = -1;
            lock.unlock();
            ss = std::stringstream(msg);
            while(true)
            {
                if(startElections)
                {
                    ss = std::stringstream();
                    ss << "ELECT " << id;
                    unit::log("Starting elections");
                    electing = true;
                    socket.sendToMultiCast(ss.str().c_str());
                    startElections = false;
                }
                else
                {
                    std::string key;
                    int electId;
                    ss >> key >> electId;
                    if(key == "ELECT")
                    {
                        if(id != electId)
                        {
                            if(others.find(unitData(electId, "", "")) == others.end())
                            {
                                unit::log("Unknown Id " + std::to_string(electId) + " is electing, discover scheduled after elections");
                                discoverAfterElections = true;
                            }
                            if(id > electId)
                            {
                                ss = std::stringstream();
                                ss << "ELECT " << id;
                                electing = true;
                                unit::log("Electing myself over " + std::to_string(electId));
                                socket.sendToMultiCast(ss.str().c_str());
                            }
                            else
                            {
                                if(electing)
                                {
                                    electing = false;
                                    unit::log("Lost elections to " + std::to_string(electId));
                                }
                                break;
                            }
                        }
                    }
                }
                try
                {
                    ss = socket.receiveStreamFrom(NULL, NULL);
                }
                catch(socketTimeoutException)
                {
                    electing = false;
                    ss = std::stringstream();
                    ss << "VICTORY " << id;
                    unit::log("Won the elections");
                    socket.sendToMultiCast(ss.str().c_str());
                    break;
                }
            }
        }
    }
}