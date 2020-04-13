#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <conio.h>
#include "controller.h"
#include "socket/socket.h"

struct Process
{
    int id;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
};

std::map<int, Process> processMap;

void controller::createUnit(int id)
{
    Process p;
    p.id = id;
    ZeroMemory(&p.si, sizeof(p.si));
	p.si.cb = sizeof(p.si);
	ZeroMemory(&p.pi, sizeof(p.pi));
    std::wstringstream ss;
    ss << L"a.exe " << id;
	if(CreateProcessW(NULL, &ss.str()[0],
                      NULL, NULL, FALSE, 0, NULL, NULL, &p.si, &p.pi))
    {
        processMap[id] = p;
    }
    else
    {
        throw controllerException(
            std::string("Unable to create process, error: ") + std::to_string(GetLastError())
        );
    }
}

void controller::terminateUnit(int id)
{
    auto itr = processMap.find(id);
    Process& p = (*itr).second;
    TerminateProcess(p.pi.hProcess, 1);
    CloseHandle(p.pi.hProcess);
    CloseHandle(p.pi.hThread);
    processMap.erase(itr);
}

void controller::terminateAllUnits()
{
    for(auto pair : processMap)
    {
        Process& p = pair.second;
        TerminateProcess(p.pi.hProcess, 1);
        CloseHandle(p.pi.hProcess);
        CloseHandle(p.pi.hThread);
    }
    processMap.clear();
}

void clearConsole()
{
    system("cls");
}
void listProcesses()
{
    if(processMap.size() != 0)
    {
        std::cout << "---------------------------------\n";
        std::cout << "Current created proccesses\n";
        int i = 0;
        for(auto pair : processMap)
        {
            std::cout << "Process #" << i << ": " << pair.second.id << '\n';
            i++;
        }
        std::cout << "---------------------------------\n";
    }
}
bool monitorActive;
void monitor()
{
    udpSocket s;
    s.setTimeOut(100);
    s.joinMulticast("239.125.125.125", "8233");
    while(monitorActive)
    {
        try
        {
            std::cout << s.receiveFrom(NULL, NULL) << '\n';
        }
        catch(socketException){}
    }
}
char getChar()
{
    char c = _getch();
    fflush(stdin);
    fflush(stdout);
    return c;
}
void handleMonitor()
{
    clearConsole();
    std::cout << "Monitor activated; press any key to stop monitoring at any time\n";
    monitorActive = true;
    std::thread t(monitor);
    getChar();
    monitorActive = false;
    t.join();
}
void handleCreate()
{
    clearConsole();
    listProcesses();
    int id;
    while(true)
    {
        std::cout << "Enter the id of the new process (it must not conflict with the previous ids): ";
        std::cin >> id;
        if(processMap.find(id) != processMap.end())
        {
            std::cout << "Id already in use";
            continue;
        }
        break;
    }
    controller::createUnit(id);
}
void handleKill()
{
    clearConsole();
    listProcesses();
    int id;
    while(true)
    {
        std::cout << "Enter the id of the process to be killed: ";
        std::cin >> id;
        if(processMap.find(id) == processMap.end())
        {
            std::cout << "Id doesn't exist";
            continue;
        }
        break;
    }
    controller::terminateUnit(id);
}

void controller::main()
{
    bool skip = false;
    while(1)
    {
        if(!skip)
        {
            clearConsole();
            std::cout << "Press 1 to monitor processes\n"
                      << "Press 2 to create new process\n"
                      << "Press 3 to kill an exisiting process\n"
                      << "Press 4 to exit\n";
            listProcesses();
        }
        else
        {
            skip = false;
        }
        char c = getChar();
        switch (c)
        {
        case '1':
            handleMonitor();
            break;
        case '2':
            handleCreate();
            break;
        case '3':
            handleKill();
            break;
        case '4':
            controller::terminateAllUnits();
            return;
        default:
            std::cout << "Unknown command " << c << '\n';
            skip = true;
            break;
        }
    }
}
