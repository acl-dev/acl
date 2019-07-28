#pragma once
#include <list>

class user_client
{
public:
	user_client(acl::socket_stream& conn, const char* user, int max_loop)
		: conn_(conn), name_(user), max_loop_(max_loop) {}
	~user_client(void)
	{
	}

	acl::socket_stream& get_stream(void) const
	{
		return conn_;
	}

	const char* get_name(void) const
	{
		return name_.c_str();
	}

	int get_max_loop(void) const
	{
		return max_loop_;
	}

	bool existing(void) const
	{
		return existing_;
	}

	void set_existing(void)
	{
		existing_ = true;
	}

	void wait_exit(void)
	{
		int mtype;
		chan_exit_.pop(mtype);
	}

	void notify_exit(void)
	{
		int mtype = 1;
		chan_exit_.put(mtype);
	}

private:
	acl::socket_stream& conn_;
	acl::channel<int> chan_exit_;
	acl::string name_;
	int  max_loop_;
	bool existing_ = false;
};
