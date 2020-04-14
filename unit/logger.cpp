#include "unit.h"
#include "../socket/socket.h"
#include <chrono>
#include <queue>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>

std::queue<std::pair<std::string, std::string>> q;
std::mutex m;
std::condition_variable cv;
std::thread *outputThread = nullptr;
bool terminate = false;

void output()
{
    std::unique_lock<std::mutex> lock(m);
    std::string payload;
    std::stringstream ss;
    time_t currentTime;
    udpSocket socket;
    while(!terminate)
    {
        if(q.empty())
        {
            cv.wait(lock);
        }
        while(!q.empty())
        {
            auto& s = q.front();
            currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            ss.str("");
            ss << '[' << currentTime << "][" << s.first << "]: " << s.second;
            payload = ss.str();
            std::cout << payload << '\n';
            socket.sendTo(UNIT_MULTICAST_IP, UNIT_LOG_PORT, payload.c_str());
            q.pop();
        }
    }
}
void unit::intializeLogger()
{
    outputThread = new std::thread(output);
}
void unit::terminateLogger()
{
    terminate = true;
    cv.notify_all();
    outputThread->join();
    delete outputThread;
}
void unit::log(const std::string& id, const std::string& msg)
{
    std::unique_lock<std::mutex> lock(m);
    q.push({id, msg});
    cv.notify_all();
}
void unit::log(const char* id, const char* msg)
{
    unit::log(std::string(id), std::string(msg));
}