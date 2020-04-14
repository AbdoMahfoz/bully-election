#include "unit.h"

void unit::discover()
{
    unit::log("Started discovering");
    udpSocket s;
    s.setTimeOut(3000);
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
                std::string port = msg.substr(msg.rfind(' ') + 1);
                unitData l(addr, port);
                lock.lock();
                if(others.find(l) == others.end())
                {
                    tcpSocket* tcp = new tcpSocket();
                    if(tcp->connect(addr.c_str(), port.c_str()))
                    {   
                        l.id = stoi(tcp->receive());
                        tcp->send(myId.c_str());
                        unit::log("Discoverd " + std::to_string(l.id));
                        std::thread *t = new std::thread(communicate, l, tcp);
                        others[l] = t;
                    }
                }
                lock.unlock();
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
                s.sendTo(addr.c_str(), port.c_str(), (std::string("BULLY OFFER ") + acceptPort).c_str());
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
            unitData l(client->getAddress(), client->getPort());
            client->send(myId.c_str());
            l.id = stoi(client->receive());
            unit::log("Discoverd " + std::to_string(l.id));
            std::thread* t = new std::thread(communicate, l, client);
            lock.lock();
            others[l] = t;
            lock.unlock();
        }
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR on tcpAccept(): ") + e.what());
    }
}