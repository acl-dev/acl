#include "stdafx.hpp"
#include "keeper.hpp"
#include "keeper_conns.hpp"
#include "keeper_waiter.hpp"

namespace acl {

keeper_waiter::keeper_waiter(void)
: last_check_(0)
{
	config_ = new keeper_config;
}

keeper_waiter::~keeper_waiter(void)
{
	delete config_;
}

keeper_waiter& keeper_waiter::set_conn_timeout(int n)
{
	config_->conn_timeo = n;
	return *this;
}

keeper_waiter& keeper_waiter::set_rw_timeout(int n)
{
	config_->rw_timeo = n;
	return *this;
}

keeper_waiter& keeper_waiter::set_conn_min(int n)
{
	config_->conn_min = n;
	return *this;
}

keeper_waiter& keeper_waiter::set_conn_max(int n)
{
	config_->conn_max = n;
	return *this;
}

keeper_waiter& keeper_waiter::set_conn_ttl(int ttl)
{
	config_->conn_ttl = ttl;
	return *this;
}

keeper_waiter& keeper_waiter::set_pool_ttl(int ttl)
{
	config_->pool_ttl = ttl;
	return *this;
}

const keeper_config& keeper_waiter::get_config(void) const
{
	return *config_;
}

void keeper_waiter::stop(void)
{
	add_task(NULL);
}

void keeper_waiter::join(void)
{
	(void) tbox_ctl_.pop();
}

void keeper_waiter::add_task(task_req* task)
{
	tbox_.push(task);
}

void keeper_waiter::run(void)
{
	int timeo = config_->pool_ttl > 0 ? config_->pool_ttl * 1000 : -1;

	while (true) {
		bool found;
		task_req* task = tbox_.pop(timeo, &found);

		check_idle();

		if (task == NULL) {
			if (found) {
				break;
			}

			continue;
		}

		keeper_conns* pool;

		const char* addr = task->get_addr();
		assert(addr && *addr);

		// lookup the correspond keeper_conns from the manager,
		// if not exist, then create one,
		// then put the connection request to the keeper_conns.

		std::map<string, keeper_conns*>::iterator it =
			manager_.find(addr);
		if (it == manager_.end()) {
			pool = new keeper_conns(*config_, addr);
			manager_[addr] = pool;
			pool->start();
		} else {
			pool = it->second;
		}

		task->set_stamp();

		// push the request task to the keeper_conns to get one
		// connection for the given addr.
		pool->add_task(*task);
	}

	for (std::map<string, keeper_conns*>::iterator it =
		manager_.begin(); it != manager_.end(); ++it) {

		it->second->stop();
		it->second->join();
		delete it->second;
	}

	done();
}

void keeper_waiter::done(void)
{
	tbox_ctl_.push(this);
}

void keeper_waiter::check_idle(void)
{
	if (config_->pool_ttl <= 0) {
		return;
	}

	time_t now = time(NULL);
	if (now - last_check_ <= 10) {
		return;
	}

	std::map<string, keeper_conns*>::iterator it, next;
	it = manager_.begin();
	for (next = it; it != manager_.end(); it = next) {
		++next;
		time_t n = (now - it->second->last_peek());
		if (n >= config_->pool_ttl && it->second->empty()) {
			fiber* fb = new keeper_killer(it->second);
			fb->start();

			manager_.erase(it);
		}
	}

	last_check_ = time(NULL);
}

} // namespace acl
