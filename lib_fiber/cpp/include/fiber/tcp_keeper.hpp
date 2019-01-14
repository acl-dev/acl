#pragma once
#include "fiber_cpp_define.hpp"
#include <string>

namespace acl {

class keeper_waiter;
class socket_stream;
class thread_mutex;

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

	tcp_keeper& set_rtt_min(double rtt);

	socket_stream* peek(const char* addr, bool* hit = NULL);

	void stop(void);

protected:
	// @override
	void* run(void);

private:
	double rtt_min_;
	keeper_waiter* waiter_;
	std::map<std::string, double> addrs_;
	thread_mutex* lock_;

	bool direct(const char* addr, bool& found);
	void remove(const char* addr);
	void update(const char* addr, double cost);
};

} // namespace acl
