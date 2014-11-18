#pragma once

class host_status;
class ping_callback;
class ping_store : public acl::rpc_request
{
public:
	ping_store(std::vector<host_status*>* host_list,
		ping_callback* callback);
	~ping_store();
protected:
	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();
private:
	std::vector<host_status*>* host_list_;

	acl::string dbpath_;
	ping_callback* callback_;
	bool create_tbl(acl::db_handle& db);
	void insert_tbl(acl::db_handle& db);
	void insert_one(acl::db_handle& db, const host_status* status);
};