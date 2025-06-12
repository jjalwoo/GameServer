#pragma once

#include <iostream>
#include <string>
#include <mysql.h>
using namespace std;

class DBManager
{
public:
	DBManager();
	~DBManager();

	bool Connect(const string& host, const string& user, const string& pw, const string& db, unsigned int port);
	void Close();
	bool ExecuteQuery(const string& query);

private:
	MYSQL* mConn;
};

