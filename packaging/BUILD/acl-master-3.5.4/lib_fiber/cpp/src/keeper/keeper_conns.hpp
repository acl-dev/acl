#pragma once
#include "fiber/fiber.hpp"
#include "fiber/fiber_tbox.hpp"

namespace acl {

class  keeper_config;
class  task_req;
class  keeper_conn;
struct keeper_link;

class keeper_conns : public fiber
{
public:
	keeper_conns(const keeper_config& config, const char* addr);
	~keeper_conns(void);

	void add_task(task_req& task);

public:
	void stop(void);
	void join(void);
	bool empty(void) const;
	time_t last_peek(void) const
	{
		return last_peek_;
	}

	void on_connect(socket_stream& conn, keeper_link* lk);

private:
	// @override
	void run(void);
	int debug_check(void);
	keeper_conn* peek_ready(void);
	void stop_all(void);
	void done(void);
	void check_idle(void);
	void trigger_more(void);

private:
	fiber_tbox<task_req>     tbox_;
	fiber_tbox<keeper_conns> tbox_ctl_;
	ACL_RING                 linker_;
	time_t                   last_peek_;
	time_t                   last_trigger_;

	const keeper_config&     config_;
	string                   addr_;
};

//////////////////////////////////////////////////////////////////////////////

class keeper_killer : public fiber
{
public:
	keeper_killer(keeper_conns* pool);

private:
	~keeper_killer(void);

	// @override
	void run(void);

private:
	keeper_conns* pool_;
};

} // namespace acl
