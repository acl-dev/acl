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

socket_stream* tcp_keeper::peek(const char* addr, bool* hit /* = NULL */)
{
	bool found;

	if (direct(addr, found)) {
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
		if (cost > rtt_min_) {
			if (0)
			printf("remove %s, cost=%.2f > %.2f\r\n", addr,
				cost, rtt_min_);
			if (found) {
				remove(addr);
			}
		}
		return conn;
	}

	task_req task;
	task.set_addr(addr);

	task.set_stamp();
	waiter_->add_task(&task);
	socket_stream* conn = task.pop();
	if (hit) {
		*hit = task.is_hit();
	}

	double cost = task.get_conn_cost();
	if (cost < rtt_min_) {
		if (0)
		printf("update cost =%.2f < rtt=%.2f, addr=%s\r\n", 
			cost, rtt_min_, addr);
		update(addr, cost);
	}

	return conn;
}

} // namespace acl
