#pragma once

class connect_pool : public acl::connect_pool
{
public:
	connect_pool(const char* addr, size_t count, size_t idx);
	virtual ~connect_pool();

	void set_timeout(int conn_timeout, int rw_timeout);

protected:
	// 基类纯虚函数的实现
	acl::connect_client* create_connect();

private:
	acl::string addr_;
	size_t count_;
	size_t idx_;
	int   conn_timeout_;
	int   rw_timeout_;
};
