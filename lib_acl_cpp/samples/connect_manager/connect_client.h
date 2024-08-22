#pragma once
#include <stdlib.h>
#include <stdio.h>

class connect_client : public acl::connect_client
{
public:
	connect_client(const char* addr, int conn_timeout, int rw_timeout);

	virtual ~connect_client();

	bool alive()
	{
		if (conn_.alive()) {
			//printf(">>>alive called: true<<<\r\n");
			return true;
		}

		printf(">>>alive called: false<<<\r\n");
		return false;
	}

	const char* get_addr() const
	{
		return addr_.c_str();
	}

	void reset() {}

protected:
	virtual bool open();

private:
	acl::string addr_;
	acl::socket_stream conn_;
	int   conn_timeout_;
	int   rw_timeout_;
};
