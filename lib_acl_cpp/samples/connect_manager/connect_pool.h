#pragma once

class connect_pool : public acl::connect_pool
{
public:
	connect_pool(const char* addr, size_t max, size_t idx);
	virtual ~connect_pool();

	void set_timeout(int conn_timeout, int rw_timeout);

protected:
	// 基类纯虚函数的实现
	acl::connect_client* create_connect();

private:
	acl::string addr_;
	int   conn_timeout_;
	int   rw_timeout_;
};
