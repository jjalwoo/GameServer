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

void DBManager::AllSelect()
{
	std::string query = "SELECT order_id, user_id, payment_method, amount, order_date FROM orders;";

	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Select Failed: " << mysql_error(mConn) << std::endl;
		return;
	}

	MYSQL_RES* result = mysql_store_result(mConn);
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		std::cout << "Order ID: " << row[0]
			<< ", User ID: " << row[1]
			<< ", Payment: " << row[2]
			<< ", Amount: " << row[3]
			<< ", Date: " << row[4] << std::endl;
	}

	mysql_free_result(result);

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

void DBManager::Insert(const int user_id, const string& method, const double amount)
{
	std::string query = "INSERT INTO orders (user_id, payment_method, amount) VALUES (" +
		std::to_string(user_id) + ", '" + method + "', " + std::to_string(amount) + ");";

	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Insert 실패!: " << mysql_error(mConn) << std::endl;
	}
	else
	{
		std::cout << "Insert 성공!" << std::endl;
	}

}

void DBManager::Delete(const int order_id)
{
	std::string query = "DELETE FROM orders WHERE order_id = " + std::to_string(order_id) + ";";

	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Delete 실패!: " << mysql_error(mConn) << std::endl;
	}
	else
	{
		std::cout << "Delete 성공!" << std::endl;
	}
}

