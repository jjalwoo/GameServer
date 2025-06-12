#include "DBManager.h"

DBManager::DBManager() 
{
	// MySQL �ʱ�ȭ	
	mConn = mysql_init(nullptr);
	if(mConn == nullptr)
	{
		cerr << "MySQL init ����!" << endl;
	}
}

DBManager::~DBManager()
{
	Close();
}

bool DBManager::Connect(const string& host, const string& user, const string& pw, const string& db, unsigned int port)
{
	if (mysql_real_connect(mConn, host.c_str(), user.c_str(), pw.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
	{
		cerr << "MySQL Connect ����: " << mysql_error(mConn) << endl;
		return false;
	}

	cout << "DB Connected ����!" << endl;
	return true;
}

void DBManager::Close()
{
	if (mConn)
	{
		mysql_close(mConn);
		mConn = nullptr;
		cout << "DB Connection closed!" << endl;
	}
}

bool DBManager::ExecuteQuery(const string& query)
{
	// ���� ����
	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Query Failed: " << mysql_error(mConn) << std::endl;
		return false;
	}

	// ��� ����
	MYSQL_RES* res = mysql_store_result(mConn);
	MYSQL_ROW row;
	int numFields = mysql_num_fields(res);  // ��(�÷�) ���� ���ϱ�

	// ��� ���
	while ((row = mysql_fetch_row(res)))
	{
		for (int i = 0; i < numFields; ++i)
		{
			std::cout << row[i] << " ";  // �� ���� �� ���
		}
		std::cout << std::endl;

	}

	mysql_free_result(res);  // �޸� ����
	return true;
}

