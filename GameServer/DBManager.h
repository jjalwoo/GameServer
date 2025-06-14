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

public:
	bool Connect(const string& host, const string& user, const string& pw, const string& db, unsigned int port);
	void Close();
	void AllSelect();
	bool ExecuteQuery(const string& query);
	void Insert(const int user_id, const string& method, const double amount);
	void Delete(const int order_id);

private:
	MYSQL* mConn;
};

