#pragma once
#include "fiber_cpp_define.hpp"
#include <string>

namespace acl {

class keeper_waiter;
class socket_stream;
class thread_mutex;

/**
 * Use independent threads to create some idle connections with the server
 * in advance. The client can directly obtain new connections from the
 * connection pool. This is valuable for long ping rtt (such as more than 10ms)
 * and can effectively reduce the connection time loss caused by network rtt.
 */
class tcp_keeper : public thread {
public:
	tcp_keeper();
	~tcp_keeper();

	/**
	 * Set the timeout (in seconds) for establishing a network connection.
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_timeout(int n);

	/**
	 * Set network socket IO read and write timeout (in seconds).
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_rw_timeout(int n);

	/**
	 * Set the minimum number of idle connections in the connection pool.
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_min(int n);

	/**
	 * Set the maximum number of idle connections in the connection pool.
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_max(int n);

	/**
	 * Set the idle time(in seconds) of the network connection. If the idle
	 * time exceeds this value, the connection will be closed.
	 * @param ttl {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_ttl(int ttl);

	/**
	 * Set the idle time(in seconds) of each connection pool. When the idle
	 * time of connection pool exceeds this value, it will be released, so
	 * that the system can recycle momory resources.
	 * @param ttl {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_pool_ttl(int ttl);

	/**
	 * Set the RTT threshold (in seconds), and only use connections from the
	 * connection pool when the network connection time exceeds this value.
	 * If the network connection time is less than this value, directly
	 * connect to the server.
	 * @param rtt {double}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_rtt_min(double rtt);

	/**
	 * Try to get the network connection corresponding to the address from
	 * the tcp_keeper object.
	 * @param addr {const char*} The server's address; The format is ip:port.
	 * @param hit {bool*} If not NULL, it will store the flag that if the
	 *  connection got is the idle connection from connection pool.
	 * @param sync {bool} Whether to connect server directly, if so, the
	 *  connection pool internal will not be pre-created for this address.
	 * @return {socket_stream*} Return NULL if connecting failed.
	 */
	socket_stream* peek(const char* addr, bool* hit = NULL,
		bool sync = false);

	/**
	 * Stop tcp_keeper thread.
	 */
	void stop();

protected:
	// @override
	void* run();

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
