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
	fiber_conn(void)
	: conn_(NULL)
	, conn_timeout_(10)
	, rw_timeout_(10) {}

	~fiber_conn(void) { delete conn_; }

	void set_addr(const char* addr)
	{
		addr_ = addr;
	}

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

private:
	// @override
	void run(void)
	{
		while (true) {
			bool found;
			task_req* req = tbox_.pop(-1, &found);
			if (req == NULL) {
				if (found) {
					break;
				}
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
		if (!conn_->open(addr_, conn_timeout_, rw_timeout_)) {
			delete conn_;
			conn_ = NULL;
		}
	}

	void done(void)
	{
		tbox_ctl_.push(NULL);
	}

private:
	string                 addr_;
	socket_stream*         conn_;
	int                    conn_timeout_;
	int                    rw_timeout_;
	fiber_tbox<task_req>   tbox_;
	fiber_tbox<fiber_conn> tbox_ctl_;
};

//////////////////////////////////////////////////////////////////////////////

class fiber_pool : public fiber
{
public:
	fiber_pool(const char* addr) : addr_(addr), max_(10) {}
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

private:
	// @override
	void run(void)
	{
		for (size_t i = 0; i < max_; i++) {
			fiber_conn* fb = new fiber_conn;
			fb->set_addr(addr_);
			fibers_.push_back(fb);
			fb->start();
		}

		std::vector<fiber_conn*>::iterator it = fibers_.begin();

		while (true) {
			bool found;
			task_req* task = tbox_.pop(-1, &found);
			if (task == NULL) {
				if (found) {
					break;
				}
				continue;
			}

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
	string                   addr_;
	size_t                   max_;
};

//////////////////////////////////////////////////////////////////////////////

class fiber_waiter : public fiber
{
public:
	fiber_waiter(void) {}
	~fiber_waiter(void) {}

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

protected:
	// @override
	void run(void)
	{
		while (true) {
			bool found;
			task_req* task = tbox_.pop(-1, &found);
			if (task == NULL) {
				if (found) {
					break;
				}
				continue;
			}

			fiber_pool* pool;

			std::map<string, fiber_pool*>::iterator it =
				manager_.find(task->get_addr());
			if (it == manager_.end()) {
				pool = new fiber_pool(task->get_addr());
				manager_[task->get_addr()] = pool;
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
	fiber_tbox<task_req>          tbox_;
	fiber_tbox<fiber_waiter>      tbox_ctl_;
	std::map<string, fiber_pool*> manager_;
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
