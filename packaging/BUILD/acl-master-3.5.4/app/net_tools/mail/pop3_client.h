#pragma once

//////////////////////////////////////////////////////////////////////////

struct POP3_METER
{
	double pop3_nslookup_elapsed;
	double pop3_connect_elapsed;
	double pop3_banner_elapsed;
	double pop3_auth_elapsed;
	double pop3_uidl_elapsed;
	double pop3_list_elapsed;
	double pop3_recv_elapsed;
	double pop3_quit_elapsed;
	double pop3_total_elapsed;
	size_t total_list;
	size_t total_uidl;
	size_t total_size;
	size_t recved_limit;
	size_t recved_count;
	size_t recved_size;
	size_t recved_speed;
};

//////////////////////////////////////////////////////////////////////////

class pop3_callback
{
public:
	pop3_callback() {}
	virtual ~pop3_callback() {}

	virtual void pop3_finish(const char* dbpath) = 0;
	virtual void pop3_report(const char* msg, size_t total,
		size_t curr, const POP3_METER& meter) = 0;
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

class pop3_client : public acl::rpc_request
{
public:
	pop3_client();
	~pop3_client();

	pop3_client& set_callback(pop3_callback*);
	pop3_client& set_account(const char*);
	pop3_client& set_passwd(const char*);
	pop3_client& set_conn_timeout(int);
	pop3_client& set_rw_timeout(int);
	pop3_client& set_pop3(const char*, int);
	pop3_client& set_recv_count(int);
	pop3_client& set_recv_save(bool on);
protected:
	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();

	// 基类虚函数：主线程处理过程，收到子线程的通知消息
	virtual void rpc_wakeup(void* ctx);
private:
	POP3_METER meter_;
private:
	pop3_callback* callback_;
	int connect_timeout_;
	int rw_timeout_;
	acl::string auth_account_;
	acl::string auth_passwd_;

	acl::string pop3_ip_;
	acl::string pop3_addr_;
	int pop3_port_;
	int recv_limit_;
	time_t recv_begin_;
	bool resv_save_;

	bool get_ip();

private:
	bool pop3_get_banner(acl::socket_stream&);
	bool pop3_auth(acl::socket_stream&, const char*, const char*);
	bool pop3_uidl(acl::socket_stream&, std::vector<acl::string>&);
	bool pop3_list(acl::socket_stream&, std::vector<size_t>&);
	bool pop3_retr(acl::socket_stream& conn,
		const std::vector<size_t>& size_list);
	bool pop3_retr_one(acl::socket_stream& conn, size_t idx);
	bool pop3_quit(acl::socket_stream& conn);
};