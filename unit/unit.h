
#define UNIT_MULTICAST_IP "239.125.125.125"
#define UNIT_DISCOVER_PORT "8330"
#define UNIT_LOG_PORT "8331"
#include <string>

namespace unit
{
    void main(int id);
    void intializeLogger(int id);
    void log(const std::string& msg);
    void log(const char* msg);
    void terminateLogger();
}