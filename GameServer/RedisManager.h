#pragma once
#include <iostream>
#include <sw/redis++/redis++.h>
#include <deque>
#include <string>
#include <vector>
#include <thread>
using namespace sw::redis;
using namespace std;

struct RedisTask
{
	enum class TaskType { SET, GET, DEL };
	TaskType type;
	std::string key;
	std::string value; // SET에만 사용
	std::function<void(std::string)> callback; // 결과 처리용
};

class RedisManager
{
public:
	RedisManager() = default;
	RedisManager(const RedisManager&) = delete;
	RedisManager& operator=(const RedisManager&) = delete;
	~RedisManager() = default;

	bool Run(const string& host, const string& port, int threadCount = 4);
	void WorkerLoop();

private:
	std::unique_ptr<Redis> mRedis;
	bool msIsRun = false;
	deque<RedisTask> mRequestQueue;
	deque<RedisTask> mResponseQueue;
	vector<thread> mWorkerThreads;
};
inline RedisManager GRedisManager;
