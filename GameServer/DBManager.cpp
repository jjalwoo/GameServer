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

void DBManager::Insert(const int user_id, const string& method, const double amount)
{
	std::string query = "INSERT INTO orders (user_id, payment_method, amount) VALUES (" +
		std::to_string(user_id) + ", '" + method + "', " + std::to_string(amount) + ");";

	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Insert ����!: " << mysql_error(mConn) << std::endl;
	}
	else
	{
		std::cout << "Insert ����!" << std::endl;
	}

}

void DBManager::Delete(const int order_id)
{
	std::string query = "DELETE FROM orders WHERE order_id = " + std::to_string(order_id) + ";";

	if (mysql_query(mConn, query.c_str()))
	{
		std::cerr << "Delete ����!: " << mysql_error(mConn) << std::endl;
	}
	else
	{
		std::cout << "Delete ����!" << std::endl;
	}
}

