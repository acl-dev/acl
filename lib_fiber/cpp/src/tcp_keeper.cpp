#include "stdafx.hpp"
#include "fiber/tcp_keeper.hpp"
#include "keeper/keeper.hpp"
#include "keeper/keeper_waiter.hpp"

namespace acl {

tcp_keeper::tcp_keeper(void)
: rtt_min_(5.00)
{
	waiter_ = new keeper_waiter;
	lock_   = new thread_mutex;
}

tcp_keeper::~tcp_keeper(void)
{
	delete waiter_;
	delete lock_;
}

tcp_keeper& tcp_keeper::set_conn_timeout(int n)
{
	waiter_->set_conn_timeout(n);
	return *this;
}

tcp_keeper& tcp_keeper::set_rw_timeout(int n)
{
	waiter_->set_rw_timeout(n);
	return *this;
}

tcp_keeper& tcp_keeper::set_conn_min(int n)
{
	assert(n >= 0);
	waiter_->set_conn_min(n);
	return *this;
}

tcp_keeper& tcp_keeper::set_conn_max(int n)
{
	assert(n >= 0);
	waiter_->set_conn_max(n);
	return *this;
}

tcp_keeper& tcp_keeper::set_conn_ttl(int ttl_ms)
{
	waiter_->set_conn_ttl(ttl_ms);
	return *this;
}

tcp_keeper& tcp_keeper::set_pool_ttl(int ttl_ms)
{
	waiter_->set_pool_ttl(ttl_ms);
	return *this;
}

tcp_keeper& tcp_keeper::set_rtt_min(double rtt)
{
	rtt_min_ = rtt;
	return *this;
}

void* tcp_keeper::run(void)
{
	waiter_->start();
	fiber::schedule();
	return NULL;
}

void tcp_keeper::stop(void)
{
	waiter_->stop();
	waiter_->join();
	this->wait();
}

bool tcp_keeper::direct(const char* addr, bool& found)
{
	std::map<std::string, double>::iterator it;
	thread_mutex_guard guard(*lock_);
	it = addrs_.find(addr);
	if (it == addrs_.end()) {
		found = false;
		return false;
	}

	found = true;
	return it->second <= rtt_min_;
}

void tcp_keeper::remove(const char* addr)
{
	std::map<std::string, double>::iterator it;
	thread_mutex_guard guard(*lock_);
	it = addrs_.find(addr);
	if (it != addrs_.end()) {
		addrs_.erase(it);
	}
}

void tcp_keeper::update(const char* addr, double cost)
{
	std::map<std::string, double>::iterator it;
	thread_mutex_guard guard(*lock_);
	addrs_[addr] = cost;
}

socket_stream* tcp_keeper::peek(const char* addr, bool* hit /* = NULL */,
	bool sync /* = false */)
{
	bool found;

	if (addr == NULL || *addr == 0) {
		logger_fatal("addr null, addr=%s", addr ? addr : "null");
	}

	// if the rtt from the given addr is short, we should just connect
	// the addr directivly.
	// in direct-connect mode, using the caller's thread or fiber running
	// space to connect the server addr.
	if (sync || direct(addr, found)) {
		if (hit) {
			*hit = false;
		}

		const keeper_config& config = waiter_->get_config();

		struct timeval begin;
		gettimeofday(&begin, NULL);
		socket_stream* conn = new socket_stream;

		if (!conn->open(addr, config.conn_timeo, config.rw_timeo)) {
			delete conn;
			remove(addr);
			return NULL;
		}

		struct timeval end;
		gettimeofday(&end, NULL);
		double cost = stamp_sub(end, begin);

		// if this rtt cost by this connecting process is long, we
		// should remove the addr from the direct-connect set to
		// avoid directivly connect in the next connect.
		if (cost > rtt_min_) {
			//printf("remove %s, cost=%.2f > %.2f\r\n",
			//	addr, cost, rtt_min_);
			if (found) {
				remove(addr);
			}
		}
		return conn;
	}

	// then, we should peek one connection to the given addr from
	// the keeper connection pool.
	// we create one connection request and push it to the task handler.
	task_req task;
	task.set_addr(addr);

	task.set_stamp();
	waiter_->add_task(&task);
	socket_stream* conn = task.pop();
	if (hit) {
		*hit = task.is_hit();
	}

	// if the connect time cost less the given min rtt, put the addr into
	// the directivly connect set.
	double cost = task.get_conn_cost();
	if (cost < rtt_min_) {
		//printf("update cost =%.2f < rtt=%.2f, addr=%s\r\n", 
		//	cost, rtt_min_, addr);
		update(addr, cost);
	}

	if (conn == NULL) {
		logger_error("connection null for addr=%s", addr);
	}

	return conn;
}

} // namespace acl
