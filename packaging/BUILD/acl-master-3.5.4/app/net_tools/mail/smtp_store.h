#pragma once

struct SMTP_METER;
class smtp_callback;
class smtp_store : public acl::rpc_request
{
public:
	smtp_store(const char* user, const char* smtp_ip,
		const SMTP_METER& meter, smtp_callback& callback);
protected:
	~smtp_store();

	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();

private:
	bool ok_;
	char* user_;
	char* smtp_ip_;
	SMTP_METER* meter_;
	smtp_callback& callback_;
	acl::string dbpath_;
	bool create_tbl(acl::db_handle& db);
	void insert_tbl(acl::db_handle& db);
};