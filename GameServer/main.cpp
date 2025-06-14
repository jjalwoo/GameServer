#include "GameServer.h"
#include "DBManager.h"
#include "DBConfig.h"
#include <thread>
#include <iostream>

int main()
{
	DBConfig dbConfig;
    if (!dbConfig.LoadConfig("config.json"))
    {
        std::cerr << "Failed to load DB config!" << std::endl;
        return -1;
    }

	DBManager dbManager;
    dbManager.Connect(dbConfig.host, dbConfig.user, dbConfig.password, dbConfig.dbname, dbConfig.port);
    

    dbManager.Insert(5, "ccw_pay", 125.00);
    dbManager.Delete(115);
    dbManager.AllSelect();

    GameServer server;
    server.Init(4);  
    server.BindandListen(8080);
    server.StartServer(100);
    while (true)
        ;
        
    return 0;
}