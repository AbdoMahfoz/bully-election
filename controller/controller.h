#include "../unit/unit.h"

class controllerException : std::exception
{
public:
    controllerException(const std::string& msg) : std::exception(msg.c_str()) {}
};

struct Process;

class controller
{
private:
    static std::map<int, Process> processMap;
    static bool monitorActive;
    static void clearConsole();
    static void listProcesses();
    static void monitor();
    static void handleMonitor();
    static void handleCreate();
    static void handleKill();
    controller();
public:
    static void createUnit(int id);
    static void terminateUnit(int id);
    static void terminateAllUnits();
    static void main();
};