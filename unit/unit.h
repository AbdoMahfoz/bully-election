#ifndef UNIT_CLASS
#define UNIT_CLASS

#define UNIT_MULTICAST_IP "239.125.125.125"
#define UNIT_DISCOVER_PORT "8330"
#define UNIT_LOG_PORT "8331"
#define UNIT_ELECTIONS_PORT "8332"

#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <set>
#include "../socket/socket.h"

struct unitData
{
public:
    int id;
    std::string address;
    std::string port;
    bool* killSwitch;
    unitData(){}
    unitData(int id, const std::string& address, const std::string& port)
    {
        this->id = id;
        this->address = address;
        this->port = port;
        this->killSwitch = new bool(true);
    }
};
bool operator<(const unitData& lhs, const unitData& rhs);

class unit
{
private:
    static std::map<unitData, std::thread*> others;
    static std::vector<std::thread*> controlThreads;
    static std::thread *slaveThread;
    static std::string acceptPort, discoverPort, myId;
    static int coordId;
    static tcpSocket acceptSocket;
    static std::thread *acceptThread, *offerThread, *electionsThread;
    static std::mutex othersMutex, coordMutex, controlMutex;
    static bool startElections, controlExists;
    //discovery
    static void initDiscover();
    static void discover();
    static void offer();
    static void tcpAccept();
    static void killConnection(int id);
    static void addConnection(const unitData& l, tcpSocket* socket);
    //communications
    static void communicate(unitData data, tcpSocket* socket);
    static void initElections();
    static void elections();
    //control
    static void launchMaster(tcpSocket* socket);
    static void launchSlave(tcpSocket* socket);
    static void master(tcpSocket* socket);
    static void slave(tcpSocket* socket);
    static void terminator();
    static void terminateControlThreads();
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