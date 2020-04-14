#include "unit.h"
#include "../socket/socket.h"
#include <chrono>
#include <queue>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <ctime>

std::queue<std::string> q;
std::mutex outputMutex;
std::condition_variable cv;
std::thread *outputThread = nullptr;
std::string myLogId;
bool terminateOutputThread = false;

void output()
{
    std::unique_lock<std::mutex> lock(outputMutex, std::defer_lock);
    std::string payload;
    std::stringstream ss;
    char* currentTime;
    udpSocket socket;
    lock.lock();
    while(!terminateOutputThread)
    {
        if(q.empty())
        {
            cv.wait(lock);
        }
        while(!q.empty())
        {
            auto s = q.front();
            q.pop();
            lock.unlock();
            auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            currentTime = std::ctime(&time);
            currentTime[strlen(currentTime) - 1] = '\0';
            ss.str("");
            ss << '[' << currentTime << "][" << myLogId << "]: " << s;
            payload = ss.str();
            std::cout << payload << '\n';
            socket.sendTo(UNIT_MULTICAST_IP, UNIT_LOG_PORT, payload.c_str());
            lock.lock();
        }
    }
}
void unit::intializeLogger(int id)
{
    myLogId = std::to_string(id);
    outputThread = new std::thread(output);
}
void unit::terminateLogger()
{
    terminateOutputThread = true;
    cv.notify_all();
    outputThread->join();
    delete outputThread;
}
void unit::log(const std::string& msg)
{
    std::unique_lock<std::mutex> lock(outputMutex);
    q.push(msg);
    cv.notify_all();
}
void unit::log(const char* msg)
{
    unit::log(std::string(msg));
}