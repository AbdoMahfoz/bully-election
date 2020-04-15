#include "unit.h"

std::map<unitData, std::thread*> unit::others;
std::string unit::acceptPort, unit::discoverPort, unit::myId;
int unit::coordId;
tcpSocket unit::acceptSocket;
std::thread *unit::acceptThread, *unit::offerThread, *unit::electionsThread;
std::mutex unit::othersMutex, unit::coordMutex, unit::controlMutex;
std::map<tcpSocket*, std::thread*> unit::controlSockets;
std::thread *unit::slaveThread = nullptr;
bool unit::startElections = true, unit::controlExists = false;

bool operator<(const unitData& lhs, const unitData& rhs)
{
    return lhs.id < rhs.id;
}

void unit::main(int id)
{
    myId = std::to_string(id);
    unit::intializeLogger();
    acceptSocket.bind("0");
    acceptPort = acceptSocket.getPort();
    acceptThread = new std::thread(tcpAccept);
    offerThread = new std::thread(offer);
    unit::log("Hello, world!");
    discover();
    electionsThread = new std::thread(elections);
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}