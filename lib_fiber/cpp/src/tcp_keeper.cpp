#include "stdafx.hpp"
#include <vector>
#include <map>
#include "acl_cpp/stdlib/trigger.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/trigger.hpp"
#include "fiber/fiber.hpp"
#include "fiber/fiber_tbox.hpp"
#include "fiber/tcp_keeper.hpp"

//////////////////////////////////////////////////////////////////////////////

namespace acl {

class tcp_keeper_config
{
public:
	tcp_keeper_config(void)
	: conn_timeo(10)
	, rw_timeo(10)
	, conn_max(10)
	, conn_ttl(10)
	, pool_ttl(20) {}

	~tcp_keeper_config(void) {}

	int conn_timeo;
	int rw_timeo;
	int conn_max;
	int conn_ttl;
	int pool_ttl;
};

class task_req
{
public:
	task_req(void) {}
	~task_req(void) {}

	void set_addr(const char* addr)
	{
		addr_ = addr;
	}

	const char* get_addr(void) const
	{
		return addr_.c_str();
	}

public:
	socket_stream* pop(void)
	{
		return tbox_.pop();
	}

	void put(socket_stream* conn)
	{
		tbox_.push(conn);
	}

private:
	string addr_;
	fiber_tbox<socket_stream> tbox_;
};

//////////////////////////////////////////////////////////////////////////////

class fiber_conn : public fiber
{
public:
	fiber_conn(const tcp_keeper_config& config, const char* addr)
	: conn_(NULL)
	, last_ctime_(0)
	, config_(config)
	, addr_(addr) {}

	~fiber_conn(void) { delete conn_; }

	void add_task(task_req* task)
	{
		tbox_.push(task);
	}

public:
	void stop(void)
	{
		add_task(NULL);
	}

	void join(void)
	{
		(void) tbox_ctl_.pop();
	}

	bool connected(void) const
	{
		return conn_ ? true : false;
	}

private:
	// @override
	void run(void)
	{
		int timeo = config_.conn_ttl > 0 ? config_.conn_ttl * 1000 : -1;

		while (true) {
			bool found;
			task_req* req = tbox_.pop(timeo, &found);
			if (req == NULL) {
				if (found) {
					break;
				}

				check_idle();
				continue;
			}

			if (conn_ == NULL) {
				connect_one();
			}

			socket_stream* conn;
			if (conn_) {
				conn = new socket_stream;
				conn->open(dup(conn_->sock_handle()));
				delete conn_;
			} else {
				conn = NULL;
			}

			req->put(conn);
			connect_one();
		}

		done();
	}

	void connect_one(void)
	{
		conn_ = new socket_stream;
		if (conn_->open(addr_, config_.conn_timeo, config_.rw_timeo)) {
			last_ctime_ = time(NULL);
		} else {
			delete conn_;
			conn_ = NULL;
		}
	}

	void check_idle(void)
	{
		if (config_.conn_ttl <= 0) {
			return;
		}
		time_t now = time(NULL);
		if ((now - last_ctime_) >= config_.conn_ttl) {
			delete conn_;
			conn_ = NULL;
		}
	}

	void done(void)
	{
		tbox_ctl_.push(NULL);
	}

private:
	fiber_tbox<task_req>   tbox_;
	fiber_tbox<fiber_conn> tbox_ctl_;
	socket_stream*         conn_;
	time_t                 last_ctime_;

	const tcp_keeper_config& config_;
	string addr_;
};

//////////////////////////////////////////////////////////////////////////////

class fiber_pool : public fiber
{
public:
	fiber_pool(const tcp_keeper_config& config, const char* addr)
	: last_peek_(0)
	, config_(config)
	, addr_(addr)
	{}

	~fiber_pool(void) {}

	void add_task(task_req* task)
	{
		tbox_.push(task);
	}

public:
	void stop(void)
	{
		add_task(NULL);
	}

	void join(void)
	{
		(void) tbox_ctl_.pop();
	}

	bool empty(void) const
	{
		for (std::vector<fiber_conn*>::const_iterator cit =
			fibers_.begin(); cit != fibers_.end(); ++cit) {

			if ((*cit)->connected()) {
				return false;
			}
		}

		return true;
	}

	time_t last_peek(void) const
	{
		return last_peek_;
	}

private:
	// @override
	void run(void)
	{
		for (int i = 0; i < config_.conn_max; i++) {
			fiber_conn* fb = new fiber_conn(config_, addr_);
			fibers_.push_back(fb);
			fb->start();
		}

		std::vector<fiber_conn*>::iterator it = fibers_.begin();

		int timeo = config_.conn_ttl > 0 ? config_.conn_ttl * 1000 : -1;

		while (true) {
			bool found;
			task_req* task = tbox_.pop(timeo, &found);
			if (task == NULL) {
				if (found) {
					break;
				}
				continue;
			}

			last_peek_ = time(NULL);

			(*it)->add_task(task);
			if (++it == fibers_.end()) {
				it = fibers_.begin();
			}
		}

		for (it = fibers_.begin(); it != fibers_.end(); ++it) {
			(*it)->stop();
			(*it)->join();
			delete *it;
		}

		done();
	}

	void done(void)
	{
		tbox_ctl_.push(NULL);
	}

private:
	fiber_tbox<task_req>     tbox_;
	fiber_tbox<fiber_pool>   tbox_ctl_;
	std::vector<fiber_conn*> fibers_;
	time_t last_peek_;

	const tcp_keeper_config& config_;
	string addr_;
};

//////////////////////////////////////////////////////////////////////////////

class fiber_pool_killer : public fiber
{
public:
	fiber_pool_killer(fiber_pool* pool)
	: pool_(pool)
	{
	}

private:
	~fiber_pool_killer(void) {}

	// @override
	void run(void)
	{
		pool_->stop();
		pool_->join();
		delete pool_;
		delete this;
	}

private:
	fiber_pool* pool_;
};

class fiber_waiter : public fiber
{
public:
	fiber_waiter(void)
	: last_check_(0)
	{
		config_ = new tcp_keeper_config;
	}

	~fiber_waiter(void)
	{
		delete config_;
	}

	void add_task(task_req* task)
	{
		tbox_.push(task);
	}

public:
	fiber_waiter& set_conn_timeout(int n)
	{
		config_->conn_timeo = n;
		return *this;
	}

	fiber_waiter& set_rw_timeout(int n)
	{
		config_->rw_timeo = n;
		return *this;
	}

	fiber_waiter& set_conn_max(int n)
	{
		config_->conn_max = n;
		return *this;
	}

	fiber_waiter& set_conn_ttl(int ttl)
	{
		config_->conn_ttl = ttl;
		return *this;
	}

	fiber_waiter& set_pool_ttl(int ttl)
	{
		config_->pool_ttl = ttl;
		return *this;
	}

public:
	void stop(void)
	{
		add_task(NULL);
	}

	void join(void)
	{
		(void) tbox_ctl_.pop();
	}

protected:
	// @override
	void run(void)
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

			fiber_pool* pool;

			const char* addr = task->get_addr();
			std::map<string, fiber_pool*>::iterator
				it = manager_.find(addr);
			if (it == manager_.end()) {
				pool = new fiber_pool(*config_, addr);
				manager_[addr] = pool;
				pool->start();
			} else {
				pool = it->second;
			}

			pool->add_task(task);
		}

		for (std::map<string, fiber_pool*>::iterator it =
			manager_.begin(); it != manager_.end(); ++it) {

			it->second->stop();
			it->second->join();
			delete it->second;
		}

		done();
	}

	void done(void)
	{
		tbox_ctl_.push(this);
	}

private:
	void check_idle(void)
	{
		if (config_->pool_ttl <= 0) {
			return;
		}

		time_t now = time(NULL);
		if (now - last_check_ <= 10) {
			return;
		}

		std::map<string, fiber_pool*>::iterator it, next;
		it = manager_.begin();
		for (next = it; it != manager_.end(); it = next) {
			++next;
			time_t n = (now - it->second->last_peek());
			if (n >= config_->pool_ttl && it->second->empty()) {
				fiber* fb = new fiber_pool_killer(it->second);
				fb->start();

				manager_.erase(it);
			}
		}

		last_check_ = time(NULL);
	}

private:
	fiber_tbox<task_req>          tbox_;
	fiber_tbox<fiber_waiter>      tbox_ctl_;
	std::map<string, fiber_pool*> manager_;
	tcp_keeper_config*            config_;
	time_t last_check_;
};

//////////////////////////////////////////////////////////////////////////////

tcp_keeper::tcp_keeper(void)
{
	waiter_ = new fiber_waiter;
}

tcp_keeper::~tcp_keeper(void)
{
	delete waiter_;
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

tcp_keeper& tcp_keeper::set_conn_max(int n)
{
	assert (n >= 0);
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
	fiber::schedule_stop();
	this->wait();
}

socket_stream* tcp_keeper::peek(const char* addr)
{
	task_req task;
	task.set_addr(addr);

	waiter_->add_task(&task);
	return task.pop();
}

} // namespace acl
