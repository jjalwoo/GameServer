#include "GameServer.h"
#include <thread>
#include <iostream>

int main()
{
    int maxIOWorkerThreadCount = 10;
    int maxClientCount = 100;

    GameServer server;    
    server.Init(maxIOWorkerThreadCount);

    if (!server.StartServer(maxClientCount))
    {
        std::cout << "���� ���� ����!" << std::endl;
        return -1;
    }

    std::cout << "���� ���� ���� ��... �����Ϸ��� Ctrl + C�� ��������." << std::endl;
   
    // ���� ���� (���� ����)
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000)); // CPU ������ ����
    }

    return 0;
}