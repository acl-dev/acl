#pragma once
#include "fiber/fiber.hpp"
#include "fiber/fiber_sem.hpp"
#include "fiber/fiber_tbox.hpp"
#include "keeper.hpp"

namespace acl {

class keeper_conns;

// one connection with one fiber
class keeper_conn : public fiber {
public:
	keeper_conn(const keeper_config& config, const char* addr,
		keeper_link* lk, keeper_conns& pool);

	~keeper_conn();

	void set_task(task_req& task);
	socket_stream* peek(double& cost);

public:
	void ask_open();
	void ask_close();

	void stop();
	void join();

	time_t get_last_ctime() const {
		return last_ctime_;
	}

	bool is_ready() const {
		return status_ == KEEPER_T_READY;
	}

	bool is_busy() const {
		return status_ == KEEPER_T_BUSY;
	}

	bool is_idle() const {
		return status_ == KEEPER_T_IDLE;
	}

	void print_status() const;

private:
	// @override
	void run();

	socket_stream* dup_stream(socket_stream& from);
	void handle_task(task_req& task);
	void connect_one();
	void done();

private:
#ifdef USE_SBOX
	fiber_sbox<ask_req>     box_;
#else
	fiber_tbox<ask_req>     box_;
#endif
	fiber_tbox<keeper_conn> tbox_ctl_;

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
