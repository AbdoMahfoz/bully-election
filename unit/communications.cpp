#include "unit.h"
#include <sstream>
#include <iostream>

void unit::communicate(unitData data, tcpSocket* socket)
{
    bool isCoord = false;
    try
    {
        socket->setTimeOut(2000);
        std::unique_lock<std::mutex> lock(othersMutex, std::defer_lock);
        std::unique_lock<std::mutex> coordLock(coordMutex, std::defer_lock);
        std::unique_lock<std::mutex> controlLock(controlMutex, std::defer_lock);
        while(*data.killSwitch)
        {
            coordLock.lock();
            isCoord = (data.id == coordId);
            coordLock.unlock();
            controlLock.lock();
            if(isCoord && !controlExists)
            {
                tcpSocket s;
                s.bind("0");
                socket->send(("CONTROL " + s.getPort()).c_str());
                tcpSocket* slave = s.accept();
                launchSlave(slave);
                unit::log("Control to coordinator connected");
            }
            controlLock.unlock();
            try
            {
                socket->send("ALIVE");
                std::string msg = socket->receive();
                if(msg != "ALIVE")
                {
                    if(msg.find("CONTROL") == 0)
                    {
                        controlLock.lock();
                        if(coordId == std::stoi(myId))
                        {
                            tcpSocket* master = new tcpSocket();
                            if( master->connect(socket->getTargetAddress().c_str(), 
                                           msg.substr(msg.rfind(' ') + 1).c_str()))
                            {
                                launchMaster(master);
                                unit::log("Controlling " + std::to_string(data.id));
                            }
                            else
                            {
                                unit::log("ERROR: Failed to control " + std::to_string(data.id));
                            }
                        }
                        else
                        {
                            unit::log("WARNING: " + std::to_string(data.id) + 
                                      " is treating me as the coordinator. Reinitating elections");
                            initElections();
                        }
                        controlLock.unlock();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            catch(socketException)
            {
                break;
            }
            if(isCoord)
            {
                //unit::log("Coordinator " + std::to_string(data.id) + " alive");
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        socket->forceClose();
        delete socket;
        if(*(data.killSwitch))
        {
            lock.lock();
            auto itr = others.find(data);
            itr->second->detach();
            delete itr->second;
            others.erase(itr);
            lock.unlock();
            if(isCoord)
            {
                unit::log("Lost connection to coordinator, reinitiating elections");
                initElections();
            }
            else
            {
                unit::log("Lost connection to " + std::to_string(data.id));
            }
            knownIds.erase(knownIds.find(data.id));
            initDiscover();
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
                if(newCoordId != coordId)
                {
                    std::string coordIdStr = msg.substr(msg.rfind(' ') + 1);
                    lock.lock();
                    coordId = std::stoi(coordIdStr);
                    lock.unlock();
                    if(lastCord != coordId)
                    {
                        if(coordIdStr != myId)
                        {
                            unit::log("New coordinator: " + std::to_string(coordId));
                        }
                        terminateControlThreads();
                    }
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
                    if(knownIds.find(electId) == knownIds.end())
                    {
                        unit::log("Unknown Id " + std::to_string(electId) + " is electing, discover scheduled after elections");
                        discoverAfterElections = true;
                    }
                    if(key == "ELECT")
                    {
                        if(id != electId)
                        {
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