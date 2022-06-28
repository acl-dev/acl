#pragma once

class connect_client : public acl::connect_client
{
public:
	connect_client(const char* addr, int conn_timeout, int rw_timeout);

	virtual ~connect_client();

	const char* get_addr() const
	{
		return addr_;
	}

	void reset() {}

protected:
	virtual bool open();

private:
	const char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
};
