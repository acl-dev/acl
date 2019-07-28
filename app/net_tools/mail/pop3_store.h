#pragma once

struct POP3_METER;
class pop3_callback;

class pop3_store : public acl::rpc_request
{
public:
	pop3_store(const char* user, const char* pop3_ip,
		const POP3_METER& meter, pop3_callback& callback);
protected:
	~pop3_store();

	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();
private:
	bool ok_;
	char* user_;
	char* pop3_ip_;
	POP3_METER* meter_;
	pop3_callback& callback_;
	acl::string dbpath_;

	bool create_tbl(acl::db_handle& db);
	void insert_tbl(acl::db_handle& db);
};