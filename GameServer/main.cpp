#include "GameServer.h"
#include <thread>
#include <iostream>

int main()
{
    GameServer server;
    server.Init(4);  
    server.BindandListen(8080);
    server.StartServer(100);



    while (true)
        ;
        
    return 0;
}