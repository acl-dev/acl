#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_role4slave
{
public:
	redis_role4slave(void) : port_(0), off_(0) {}
	~redis_role4slave(void) {}

	void set_ip(const char* ip)
	{
		ip_ = ip;
	}
	void set_port(int port)
	{
		port_ = port;
	}
	void set_status(const char* status)
	{
		status_ = status;
	}
	void set_offset(long long off)
	{
		off_ = off;
	}
	const char* get_ip(void) const
	{
		return ip_.c_str();
	}
	int get_port(void) const
	{
		return port_;
	}
	const char* get_status(void) const
	{
		return status_.c_str();
	}
	long long get_offset(void) const
	{
		return off_;
	}

private:
	string ip_;
	int port_;
	long long off_;
	string status_;
};

class ACL_CPP_API redis_role4master
{
public:
	redis_role4master(void) : off_(0) {}
	~redis_role4master(void) {}

	void set_offset(long long off)
	{
		off_ = off;
	}
	long long get_offset(void) const
	{
		return off_;
	}
	void add_slave(const redis_role4slave& slave)
	{
		slaves_.push_back(slave);
	}
	const std::vector<redis_role4slave>& get_slaves(void) const
	{
		return slaves_;
	}

private:
	long long off_;
	std::vector<redis_role4slave> slaves_;
};

class ACL_CPP_API redis_role : virtual public redis_command
{
public:
	redis_role(void);
	redis_role(redis_client* conn);
	virtual ~redis_role(void) {}

	bool role(void);
	const redis_role4master& get_role4master(void) const
	{
		return role4master_;
	}
	const redis_role4slave& get_role4slave(void) const
	{
		return role4slave_;
	}

	const char* get_role_name(void) const
	{
		return role_name_.c_str();
	}

private:
	string role_name_;
	std::vector<string> masters_;
	redis_role4master role4master_;
	redis_role4slave role4slave_;

	bool role_sentinel(const redis_result** a, size_t n);
	bool role_master(const redis_result** a, size_t n);
	bool role_slave(const redis_result** a, size_t n);

	bool add_one_slave(const redis_result* a, redis_role4master& out);
};

}

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
