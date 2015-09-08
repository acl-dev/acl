#include "stdafx.h"

using namespace acl;

int main(void)
{
	db_pool *dp = new mysql_pool("127.0.0.1:3306", "test_db", "zsxxsz",
			"111111");

	std::list<db_handle*> dbs;

	// 设置空闲连接的生存周期
	dp->set_idle(1);
	for (int i = 0; i < 10; i++)
	{
		db_handle* dh = (db_handle*) dp->peek();
		dbs.push_back(dh);
	}

	std::list<db_handle*>::iterator it = dbs.begin();
	for (; it != dbs.end(); ++it)
	{
		dp->put(*it);
		//sleep(1);
	}

	dbs.clear();

	printf("dbpool count: %d\r\n", dp->get_dbcount());
	sleep(2);
	db_handle* dh = dp->peek();
	dp->put(dh);
	printf("dbpool count: %d\r\n", dp->get_dbcount());
	delete dp;

	return 0;
}
