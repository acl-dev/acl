#pragma once

//////////////////////////////////////////////////////////////////////////

class nslookup_callback
{
public:
	nslookup_callback() {}
	virtual ~nslookup_callback() {}

	virtual void nslookup_report(size_t total, size_t curr) = 0;
	virtual void nslookup_finish(const char* dbpath) = 0;
protected:
private:
};

//////////////////////////////////////////////////////////////////////////

class nslookup;

struct IP_INFO
{
	char  ip[64];
	int   ttl;
};

class domain_info
{
public:
	domain_info(nslookup& ns, const char* domain);
	~domain_info();

	const char* get_domain() const
	{
		return domain_;
	}

	double get_spent() const;

	void set_begin();
	void set_end();
	void add_ip(const char* ip, int ttl);
	const std::vector<IP_INFO*>& get_ip_list() const
	{
		return ip_list_;
	}

private:
	// get_nslookup 函数只可以被 nslookup 类实例调用
	friend class nslookup;
	nslookup& get_nslookup() const
	{
		return ns_;
	}
private:
	nslookup& ns_;
	char domain_[256];
	struct timeval begin_;
	struct timeval end_;

	std::vector<IP_INFO*> ip_list_;
};

class nslookup : public acl::rpc_request
{
public:
	nslookup(const char* filepath, nslookup_callback* callback,
		const char* dns_ip, int dns_port, int timeout);
protected:
	~nslookup();

	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();

	// 基类虚函数：主线程处理过程，收到子线程的通知消息
	virtual void rpc_wakeup(void* ctx);

private:
	acl::string filepath_;
	nslookup_callback* callback_;
	acl::string dns_ip_;
	int   dns_port_;
	int   timeout_;
	std::vector<domain_info*>* domain_list_;
	size_t nresult_;  // 已经返回查询结果的个数

	bool load_file();
	void lookup_all();

	static void dns_result(ACL_DNS_DB *dns_db, void *ctx, int errnum);
};
