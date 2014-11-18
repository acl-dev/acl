#pragma once

//////////////////////////////////////////////////////////////////////////

class net_store_callback
{
public:
	net_store_callback() {}
	virtual ~net_store_callback() {}

	virtual void load_db_callback(const char* smtp_addr, int smtp_port,
		const char* pop3_addr, int pop3_port,
		const char* user, const char* pass,
		const char* recipients, bool store) = 0;
private:
};
//////////////////////////////////////////////////////////////////////////

class net_store : public acl::rpc_request
{
public:
	net_store(const char* smtp_addr, int smtp_port,
		const char* pop3_addr, int pop3_port,
		const char* user, const char* pass,
		const char* recipients, net_store_callback* callback,
		bool store = false);

	static bool get_key(const char* name, acl::string& out);
	static bool set_key(const char* name, const char* value);
protected:
	~net_store();

	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();
private:
	acl::string smtp_addr_;
	int smtp_port_;
	acl::string pop3_addr_;
	int pop3_port_;
	acl::string user_;
	acl::string pass_;
	acl::string recipients_;
	net_store_callback* callback_;
	bool store_;

	acl::string dbpath_;

	void load_mail_db(acl::db_handle& db);
	void save_mail_db(acl::db_handle& db);
	bool create_mail_tbl(acl::db_handle& db, const char* tbl_name,
		const char* sql_create);
};