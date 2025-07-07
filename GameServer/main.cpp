#include "GameServer.h"
#include "DBManager.h"
#include "DBConfig.h"
#include <thread>
#include <iostream>
#include "example.pb.h"
#include <sw/redis++/redis++.h>
using namespace sw::redis;

int main()
{
	DBConfig dbConfig;
    if (!dbConfig.LoadConfig("config.json"))
    {
        std::cerr << "Failed to load DB config!" << std::endl;
        return -1;
    }

    auto redis = Redis("tcp://127.0.0.1:6379");
    redis.set("name", "changwoo");
    auto val = redis.get("name");
    cout << *val << endl;    
    
  /*  ExampleMessage message;
	message.set_id("12345");
	message.set_name("Test User");
	message.set_value(42);
    std::cout << "ID: " << message.id() << std::endl;
    std::cout << "Name: " << message.name() << std::endl;
    std::cout << "Value: " << message.value() << std::endl;
	
	std::string buffer;
    message.ParseFromString(buffer);

    LoginResponse res;
	res.set_result(1);*/


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