#include "unit.h"
#include "../socket/socket.h"
#include <vector>
#include <map>

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

std::map<unitData, tcpSocket*> others;
std::string acceptPort, myId;
tcpSocket acceptSocket;
std::thread *acceptThread, *offerThread;
std::mutex othersMutex;

void discover()
{
    udpSocket s;
    s.setTimeOut(3000);
    std::string addr;
    std::string msg;
    std::unique_lock<std::mutex> lock(othersMutex, std::defer_lock);
    for(int i = 0; i < 5; i++)
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
                        others[l] = tcp;
                    }
                }
                lock.unlock();
            }
        }
    }
}
void tcpAccept()
{
    std::unique_lock<std::mutex> lock(othersMutex, std::defer_lock);
    while(true)
    {
        tcpSocket* client = acceptSocket.accept();
        unitData l(client->getAddress(), client->getPort());
        client->send(myId.c_str());
        l.id = stoi(client->receive());
        lock.lock();
        others[l] = client;
        lock.unlock();
    }
}
void offer()
{
    udpSocket s;
    s.joinMulticast(UNIT_MULTICAST_IP, UNIT_DISCOVER_PORT);
    std::string addr, port, msg;
    while(true)
    {
        msg = s.receiveFrom(&addr, &port);
        if(msg == "BULLY DISCOVER")
        {
            s.sendTo(addr.c_str(), port.c_str(), (std::string("BULLY OFFER ") + acceptPort).c_str());
        }
    }
}

void unit::main(int id)
{
    myId = std::to_string(id);
    acceptSocket.bind("0");
    acceptPort = acceptSocket.getPort();
    acceptThread = new std::thread(tcpAccept);
    offerThread = new std::thread(offer);
    discover();
}