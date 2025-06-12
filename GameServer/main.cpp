#include "GameServer.h"
#include "DBManager.h"
#include <thread>
#include <iostream>

int main()
{
	DBManager dbManager;
    if (dbManager.Connect("127.0.0.1", "root", "wkfdnek1^^", "test_db", 3306))
    {
        dbManager.ExecuteQuery("SELECT * FROM orders");
    }

    GameServer server;
    server.Init(4);  
    server.BindandListen(8080);
    server.StartServer(100);
    while (true)
        ;
        
    return 0;
}