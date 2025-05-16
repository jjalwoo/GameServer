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
        std::cout << "서버 시작 실패!" << std::endl;
        return -1;
    }

    std::cout << "게임 서버 실행 중... 종료하려면 Ctrl + C를 누르세요." << std::endl;
   
    // 서버 유지 (무한 루프)
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1000)); // CPU 점유율 방지
    }

    return 0;
}