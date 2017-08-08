#pragma once
#include "../acl_cpp_define.hpp"
#include "connect_client.hpp"

namespace acl
{

class socket_stream;
class tcp_sender;
class tcp_reader;
class string;

class ACL_CPP_API tcp_client : public connect_client
{
public:
	tcp_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
	virtual ~tcp_client(void);

	bool send(const void* data, unsigned int len, string* out = NULL);

protected:
	// @override
	virtual bool open(void);

private:
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;

	socket_stream* conn_;
	tcp_sender*    sender_;
	tcp_reader*    reader_;

	bool try_open(bool* reuse_conn);
};

} // namespace acl
