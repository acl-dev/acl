#pragma once
#include "fiber/fiber.hpp"
#include "fiber/fiber_sem.hpp"
#include "fiber/fiber_tbox.hpp"
#include "keeper.hpp"

namespace acl {

class keeper_conns;

class keeper_conn : public fiber
{
public:
	keeper_conn(const keeper_config& config, const char* addr,
		keeper_link* lk, keeper_conns& pool);

	~keeper_conn(void);

	void set_task(task_req& task);
	socket_stream* peek(double& cost);

public:
	void stop(void);
	void ask_connect(void);
	void join(void);

	bool is_ready(void) const
	{
		return status_ == KEEPER_T_READY;
	}

	bool is_busy(void) const
	{
		return status_ == KEEPER_T_BUSY;
	}

	bool is_idle(void) const
	{
		return status_ == KEEPER_T_IDLE;
	}

	void print_status(void) const;

private:
	// @override
	void run(void);
	void handle_task(task_req& task);
	void connect_one(void);
	void check_idle(void);
	void done(void);

private:
#ifdef USE_SBOX
	fiber_sbox<task_req>    box_;
#else
	fiber_tbox<task_req>    box_;
#endif
	fiber_tbox<keeper_conn> tbox_ctl_;

	ask_type_t              ask_;
	socket_stream*          conn_;
	keeper_status_t         status_;
	time_t                  last_ctime_;

	const keeper_config&    config_;
	string                  addr_;
	keeper_link*            lk_;
	keeper_conns&           pool_;
	task_req*               task_;
	double                  conn_cost_;
};

} // namespace acl
