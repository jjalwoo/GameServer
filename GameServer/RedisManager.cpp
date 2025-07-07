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
        return true; // ���� ���� ����
    }
    catch (const std::exception& e)
    {
        std::cerr << "Redis ���� ����!:  " << e.what() << std::endl;
        return false; // ���� ����
    }
}

void RedisManager::WorkerLoop()
{
}
   


