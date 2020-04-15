#include "unit.h"

void unit::discover()
{
    unit::log("Started discovering");
    udpSocket s;
    s.setTimeOut(1000);
    s.bind("0");
    discoverPort = s.getPort();
    std::string addr;
    std::string msg;
    std::unique_lock<std::mutex> lock(othersMutex, std::defer_lock);
    try
    {
        s.sendTo(UNIT_MULTICAST_IP, UNIT_DISCOVER_PORT, "BULLY DISCOVER");
        while(true)
        {
            try
            {
                msg = s.receiveFrom(&addr, NULL);
            }
            catch(socketTimeoutException)
            {
                break;
            }
            if(msg.find("BULLY OFFER") == 0)
            {
                std::stringstream ss(msg.substr(12));
                std::string port;
                int offerId;
                ss >> offerId >> port;
                unitData l(offerId, addr, port);
                lock.lock();
                if(others.find(l) != others.end())
                {
                    unit::log("Rejected offer of already known id");
                    continue;
                }
                lock.unlock();
                unit::log("Discoverd " + std::to_string(l.id));
                tcpSocket* tcp = new tcpSocket();
                if(tcp->connect(addr.c_str(), port.c_str()))
                {   
                    tcp->send(myId.c_str());
                    addConnection(l, tcp);
                }
            }
        }
        unit::log("Stopped discovering");
        unit::log("Number of discoverd clients: " + std::to_string(others.size()));
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR on discover(): ") + e.what());
    }
}
void unit::offer()
{
    udpSocket s;
    try
    {
        s.joinMulticast(UNIT_MULTICAST_IP, UNIT_DISCOVER_PORT);
        std::string addr, port, msg;
        while(true)
        {
            msg = s.receiveFrom(&addr, &port);
            if(msg == "BULLY DISCOVER")
            {
                if(port == discoverPort)
                {
                    continue;
                }
                std::stringstream ss;
                ss << "BULLY OFFER " << myId << ' ' << acceptPort;
                s.sendTo(addr.c_str(), port.c_str(), ss.str().c_str());
            }
        }
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR on offer(): ") + e.what());
    }
}
void unit::tcpAccept()
{
    try
    {
        std::unique_lock<std::mutex> lock(othersMutex, std::defer_lock);
        while(true)
        {
            tcpSocket* client = acceptSocket.accept();
            int conId = std::stoi(client->receive());
            unitData l(conId, client->getTargetAddress(), client->getTargetPort());
            lock.lock();
            if (others.find(l) != others.end())
            {
                unit::log("Connection of already known id");
                killConnection(l.id);
            }
            else
            {
                unit::log("Discoverd " + std::to_string(l.id));
            }
            lock.unlock();
            addConnection(l, client);
        }
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR on tcpAccept(): ") + e.what());
    }
}

void unit::addConnection(const unitData& l, tcpSocket* socket)
{
    std::unique_lock<std::mutex> lock(othersMutex);
    std::thread* t = new std::thread(communicate, l, socket);
    others[l] = t;
}
void unit::killConnection(int id)
{
    unitData l;
    l.id = id;
    auto itr = others.find(l);
    *(itr->first.killSwitch) = false;
    itr->second->join();
    delete itr->second;
    delete itr->first.killSwitch;
    others.erase(itr);
}
void unit::initDiscover()
{
    std::thread* t = new std::thread(discover);
    t->detach();
    delete t;
}