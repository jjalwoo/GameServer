#include "DBManager.h"

DBManager::DBManager() 
{
	// MySQL 초기화	
	mConn = mysql_init(nullptr);
	if(mConn == nullptr)
	{
		cerr << "MySQL init 실패!" << endl;
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
		cerr << "MySQL Connect 실패: " << mysql_error(mConn) << endl;
		return false;
	}

	cout << "DB Connected 성공!" << endl;
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
	// 쿼리 실행
	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Query Failed: " << mysql_error(mConn) << std::endl;
		return false;
	}

	// 결과 저장
	MYSQL_RES* res = mysql_store_result(mConn);
	MYSQL_ROW row;
	int numFields = mysql_num_fields(res);  // 열(컬럼) 개수 구하기

	// 결과 출력
	while ((row = mysql_fetch_row(res)))
	{
		for (int i = 0; i < numFields; ++i)
		{
			std::cout << row[i] << " ";  // 각 열의 값 출력
		}
		std::cout << std::endl;

	}

	mysql_free_result(res);  // 메모리 해제
	return true;
}

