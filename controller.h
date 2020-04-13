
class controllerException : std::exception
{
public:
    controllerException(const std::string& msg) : std::exception(msg.c_str()) {}
};

namespace controller
{
    void createUnit(int id);
    void terminateUnit(int id);
    void terminateAllUnits();
    void main();
}