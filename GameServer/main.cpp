#include "GameServer.h"
#include <thread>
#include <iostream>

int main()
{
    GameServer server;
    server.Init(4);  
    server.BindandListen(8080);
    if (!server.StartServer(100))
    {
        std::cout << "Server Start Fail!" << std::endl;
    }

    while (true)
        ;
        
    return 0;
}