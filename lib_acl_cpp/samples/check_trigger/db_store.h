#pragma once

class http_thread;

class db_store
{
public:
	db_store();
	~db_store();

	// 更新数据表
	bool db_update(const http_thread& http);

	// 创建数据表
	bool db_create();

private:
};
