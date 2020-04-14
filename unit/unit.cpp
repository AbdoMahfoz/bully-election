#include "unit.h"
#include "../socket/socket.h"
#include <vector>
#include <map>
#include <mutex>

struct unitData
{
public:
    int id;
    std::string address;
    std::string port;
    unitData(){}
    unitData(std::string& address, std::string& port)
    {
        this->address = address;
        this->port = port;
    }
};
bool operator<(const unitData& lhs, const unitData& rhs)
{
    if(lhs.address == rhs.address)
    {
        return lhs.port < rhs.port;
    }
    else
    {
        return lhs.address < rhs.address;
    }
}

std::map<unitData, std::thread*> others;
std::string acceptPort, discoverPort, myId;
tcpSocket acceptSocket;
std::thread *acceptThread, *offerThread;
std::mutex othersMutex;

void communicate(unitData data, tcpSocket* socket)
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
void discover()
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
            catch(socketException)
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
void tcpAccept()
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
void offer()
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

void unit::main(int id)
{
    unit::intializeLogger(id);
    myId = std::to_string(id);
    acceptSocket.bind("0");
    acceptPort = acceptSocket.getPort();
    acceptThread = new std::thread(tcpAccept);
    offerThread = new std::thread(offer);
    unit::log("Hello, world!");
    discover();
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}