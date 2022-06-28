#include "stdafx.hpp"
#include "keeper.hpp"
#include "keeper_conns.hpp"
#include "keeper_conn.hpp"

namespace acl {

keeper_conn::keeper_conn(const keeper_config& config, const char* addr,
	keeper_link* lk, keeper_conns& pool)
: conn_(NULL)
, status_(KEEPER_T_IDLE)
, last_ctime_(0)
, config_(config)
, addr_(addr)
, lk_(lk)
, pool_(pool)
, task_(NULL)
, conn_cost_(-1.0)
{
}

keeper_conn::~keeper_conn(void)
{
	delete conn_;
}

void keeper_conn::set_task(task_req& task)
{
	task_ = &task;
}

socket_stream* keeper_conn::dup_stream(socket_stream& from)
{
	// move the fd from current fiber to other thread,
	// we should dup the fd first and delete the origin
	// fd in the current fiber.

	socket_stream* to = new socket_stream;
	(void) to->open(dup(from.sock_handle()));
	const char* local_addr = from.get_local(true);
	const char* peer_addr  = from.get_peer(true);

	if (local_addr == NULL || *local_addr == 0) {
		logger_error("local_addr null");
		delete to;
		return NULL;
	}
	if (peer_addr == NULL || *peer_addr == 0) {
		logger_error("peer_addr null");
		delete to;
		return NULL;
	}

	to->set_local(local_addr);
	to->set_peer(peer_addr);
	return to;
}

socket_stream* keeper_conn::peek(double& cost)
{
	cost = conn_cost_;
	if (is_ready()) {
		assert(conn_);
		socket_stream* conn = dup_stream(*conn_);
		delete conn_;
		conn_   = NULL;
		status_ = KEEPER_T_IDLE;
		return conn;
	}

	return NULL;
}

void keeper_conn::stop(void)
{
	if (is_busy()) {
		logger_warn("fiber is busy, kill it");
		this->kill();
	}
	ask_req* ask = new ask_req(ASK_T_STOP);
	box_.push(ask);
}

void keeper_conn::ask_open(void)
{
	ask_req* ask = new ask_req(ASK_T_CONN);
	box_.push(ask);
}

void keeper_conn::ask_close(void)
{
	ask_req* ask = new ask_req(ASK_T_CLOSE);
	box_.push(ask);
}

void keeper_conn::join(void)
{
	(void) tbox_ctl_.pop();
}

void keeper_conn::print_status(void) const
{
	switch (status_) {
	case KEEPER_T_BUSY:
		printf("fiber status: KEEPER_T_BUSY\r\n");
		break;
	case KEEPER_T_IDLE:
		printf("fiber status: KEEPER_T_IDLE\r\n");
		break;
	case KEEPER_T_READY:
		printf("fiber status: KEEPER_T_READY\r\n");
		break;
	default:
		printf("fiber status: unknown(%d)\r\n", status_);
		break;
	}
}

void keeper_conn::run(void)
{
	// if the task has been set before the fiber started, we should
	// handle the task directly, without waiting task from box_.
	if (task_) {
		double n = task_->get_cost();
		if (n >= 50) {
			logger_warn("task schedule cost %.2f", n);
		}
		handle_task(*task_);
		delete this;
		return;
	}

	// connect_one();

	// loop popping task message for connecting one addr
	while (true) {
		ask_req* ask = box_.pop();

		assert(ask != NULL);
		ask_type_t type = ask->get_type();
		delete ask;

		if (type == ASK_T_STOP) {
			break;
		}

		if (type == ASK_T_CONN) {
			if (conn_ == NULL) {
				connect_one();
			} else {
				logger_debug(FIBER_DEBUG_KEEPER, 1,
					"[debug]: connecting ...");
			}
		} else if (type == ASK_T_CLOSE) {
			if (conn_ != NULL) {
				conn_->set_tcp_solinger(true, 0);
				delete conn_;
				conn_ = NULL;
				status_ = KEEPER_T_IDLE;
			}
		} else {
			logger_fatal("invalid ask=%d", type);
		}
	}

	done();
}

void keeper_conn::handle_task(task_req& task)
{
	if (conn_ == NULL) {
		connect_one();
	}

	socket_stream* conn;
	if (conn_) {
		// must dup the sock fd for moving the connection from
		// one thread started in fiber schedule mode to another.
		conn = dup_stream(*conn_);
		delete conn_;
		conn_   = NULL;
		status_ = KEEPER_T_IDLE;
	} else {
		conn    = NULL;
		status_ = KEEPER_T_IDLE;
	}

	// notify the request task with the connection
	task.set_conn_cost(conn_cost_);
	task.put(conn);
}

void keeper_conn::connect_one(void)
{
	// this process can be called only when none conn_ was created
	assert(conn_ == NULL);
	status_ = KEEPER_T_BUSY;

	conn_ = new socket_stream;
	struct timeval begin;
	gettimeofday(&begin, NULL);

	// connecting one server with the given addr
	bool ret = conn_->open(addr_, config_.conn_timeo, config_.rw_timeo);
	struct timeval end;
	gettimeofday(&end, NULL);
	conn_cost_ = acl::stamp_sub(end, begin);

	if (conn_cost_ >= 1000) {
		logger_warn("cost: %.2f ms, addr=%s", conn_cost_, addr_.c_str());
	}

	if (ret) {
		status_     = KEEPER_T_READY;
		last_ctime_ = time(NULL);
		// add the connection into the connections pool
		pool_.on_connect(*conn_, lk_);
	} else {
		if (this->killed()) {
			logger_warn("I've been killed");
		}
		delete conn_;
		conn_   = NULL;
		status_ = KEEPER_T_IDLE;
	}
}

void keeper_conn::done(void)
{
	tbox_ctl_.push(NULL);
}

} // namespace acl
