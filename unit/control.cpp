#include "unit.h"
#include <queue>

std::string executeCommand(const std::string& cmd)
{
    std::stringstream ss(cmd);
    int n, res;
    ss >> n;
    for(int i = 0; i < n; i++)
    {
        int x;
        ss >> x;
        if(i == 0) res = x;
        else res = min(res, x);
    }
    return std::to_string(res);
}
void unit::control(bool* running, bool isCoordinator, tcpSocket* socket)
{
    try
    {
        socket->setTimeOut(20);
        std::string buff;
        while(*running)
        {
            try
            {
                if(isCoordinator)
                {
                    std::stringstream ss;
                    ss << 100 << ' ';
                    int exepctedOutput = 0;
                    for(int i = 0; i < 100; i++)
                    {
                        int x = (rand() % ((int)1e9));
                        if(i == 0) exepctedOutput = x;
                        else exepctedOutput = min(exepctedOutput, x);
                        ss << (rand() % ((int)1e9)) << ' ';
                    }
                    ss << "\n\r";
                    socket->send(ss.str().c_str());
                    int output = std::stoi(socket->receive());
                    if(output != exepctedOutput)
                    {
                        ss = std::stringstream();
                        ss << "Control ERROR: Expected " << exepctedOutput << ", Found " << output;
                        unit::log(ss.str());
                    }
                    unit::log("Operation successfull");
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
                else
                {
                    buff.append(socket->receive());
                    if(buff[buff.size() - 2] == '\n' && buff[buff.size() - 1] == '\r')
                    {
                        socket->send(executeCommand(buff).c_str());
                    }
                }
            }
            catch(socketTimeoutException)
            {
                continue;
            }
            catch(socketException)
            {
                break;
            }
        }
    }
    catch(std::exception e)
    {
        unit::log(std::string("ERROR in control(): ") + e.what());
    }
    *running = false;
    socket->forceClose();
    delete socket;
}