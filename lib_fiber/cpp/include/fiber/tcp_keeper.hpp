#pragma once
#include "fiber_cpp_define.hpp"

namespace acl {

class fiber_waiter;
class socket_stream;

class tcp_keeper : public thread
{
public:
	tcp_keeper(void);
	~tcp_keeper(void);

	tcp_keeper& set_conn_timeout(int n);
	tcp_keeper& set_rw_timeout(int n);
	tcp_keeper& set_conn_max(int n);
	tcp_keeper& set_conn_ttl(int ttl);
	tcp_keeper& set_pool_ttl(int ttl);

	socket_stream* peek(const char* addr);

	void stop(void);

protected:
	// @override
	void* run(void);

private:
	fiber_waiter* waiter_;
};

} // namespace acl
