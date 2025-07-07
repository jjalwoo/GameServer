#include "RedisManager.h"

bool RedisManager::Run(const string& host, const string& port, int threadCount)
{
    ConnectionOptions options;
    options.host = host;
    options.port = std::stoi(port);
    options.socket_timeout = std::chrono::milliseconds(200);

    try
    {
        mRedis = Redis(options);
        return true; // 정상 연결 성공
    }
    catch (const std::exception& e)
    {
        std::cerr << "Redis 연결 실패!:  " << e.what() << std::endl;
        return false; // 연결 실패
    }
}

void RedisManager::WorkerLoop()
{
}
   


