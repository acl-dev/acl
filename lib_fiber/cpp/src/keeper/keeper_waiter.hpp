#pragma once
#include <map>
#include "fiber/fiber.hpp"
#include "fiber/fiber_tbox.hpp"

namespace acl {

class keeper_config;
class task_req;
class keeper_conns;

// waiting for connection task request and lookup the connection pool
// for the request's addr, then put the task to the pool.
class keeper_waiter : public fiber {
public:
	keeper_waiter();
	~keeper_waiter();

	void add_task(task_req* task);

public:
	const keeper_config& get_config() const;

	keeper_waiter& set_conn_timeout(int n);
	keeper_waiter& set_rw_timeout(int n);
	keeper_waiter& set_conn_min(int n);
	keeper_waiter& set_conn_max(int n);
	keeper_waiter& set_conn_ttl(int ttl);
	keeper_waiter& set_pool_ttl(int ttl);

public:
	// called by tcp_keeper to stop the waiter fiber
	void stop();

	// called by tcp_keeper to wait the waiter fiber to exit
	void join();

protected:
	// @override
	void run();

	// notify tcp_keeper the waiter fiber will exit.
	void done();

private:
	void check_idle();

private:
	fiber_tbox<task_req>            tbox_;
	fiber_tbox<keeper_waiter>       tbox_ctl_;
	std::map<string, keeper_conns*> manager_;
	keeper_config*                  config_;
	time_t last_check_;
};

} // namespace acl
