#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

class ACL_CPP_API disque_node
{
public:
	disque_node() : port_(0), priority_(0) {}
	~disque_node() {}

	void set_id(const char* id)
	{
		id_ = id;
	}

	void set_ip(const char* ip)
	{
		ip_ = ip;
	}

	void set_port(int port)
	{
		port_ = port;
	}

	void set_priority(int n)
	{
		priority_ = n;
	}

	const char* get_id() const
	{
		return id_.c_str();
	}

	const char* get_ip() const
	{
		return ip_.c_str();
	}

	int get_port() const
	{
		return port_;
	}

	int get_priority() const
	{
		return priority_;
	}

private:
	string id_;
	string ip_;
	int port_;
	int priority_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
