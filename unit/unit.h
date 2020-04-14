#ifndef UNIT_CLASS
#define UNIT_CLASS

#define UNIT_MULTICAST_IP "239.125.125.125"
#define UNIT_DISCOVER_PORT "8330"
#define UNIT_LOG_PORT "8331"
#define UNIT_ELECTIONS_PORT "8332"

#include <string>
#include <map>
#include <mutex>
#include "../socket/socket.h"

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
bool operator<(const unitData& lhs, const unitData& rhs);

class unit
{
private:
    static std::map<unitData, std::thread*> others;
    static std::string acceptPort, discoverPort, myId, coordId;
    static tcpSocket acceptSocket;
    static std::thread *acceptThread, *offerThread, *electionsThread;
    static std::mutex othersMutex, coordMutex;
    static bool startElections;
    //discovery
    static void discover();
    static void offer();
    static void tcpAccept();
    //communications
    static void communicate(unitData data, tcpSocket* socket);
    static void elections();
    //logger
    static void output();
    static void intializeLogger();
    static void log(const std::string& msg);
    static void log(const char* msg);
    static void terminateLogger();
    //private ctor
    unit();
public:
    static void main(int id);
};

#endif