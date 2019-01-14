#include "stdafx.hpp"
#include "keeper_conns.hpp"
#include "keeper_conn.hpp"

namespace acl {

keeper_conn::keeper_conn(const keeper_config& config, const char* addr,
	keeper_link* lk, keeper_conns& pool)
: ask_(ASK_T_NULL)
, conn_(NULL)
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

socket_stream* keeper_conn::peek(double& cost)
{
	cost = conn_cost_;
	if (is_ready()) {
		assert(conn_);
		socket_stream* conn = new socket_stream;
		conn->open(dup(conn_->sock_handle()));
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
	ask_ = ASK_T_STOP;
	box_.push(NULL);
}

void keeper_conn::ask_connect(void)
{
	ask_ = ASK_T_CONN;
	box_.push(NULL);
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
	if (task_) {
		double n = task_->get_cost();
		if (n >= 50) {
			const struct  timeval& last = task_->get_stamp();
			printf(">>>cost %.2f, %p\n", n, task_);
			printf("last=%p, sec=%ld, usec=%ld\r\n",
				&last, last.tv_sec, last.tv_usec);
		}
		handle_task(*task_);
		delete this;
		return;
	}

	connect_one();

#ifndef USE_SBOX
	int timeo = config_.conn_ttl > 0 ? config_.conn_ttl * 1000 : -1;
#endif
	while (true) {
		bool found;
#ifdef USE_SBOX
		task_req* task = box_.pop(&found);
#else
		task_req* task = box_.pop(timeo, &found);
#endif
		assert(task == NULL);

		if (ask_ == ASK_T_STOP) {
			break;
		} else if (ask_ == ASK_T_CONN) {
			if (conn_ == NULL) {
				connect_one();
			} else {
				printf("connecting ...\n");
			}
		} else if (!found) {
			check_idle();
		} else {
			printf("invalid ask=%d, task=%p\n", ask_, task);
			abort();
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
		conn = new socket_stream;
		conn->open(dup(conn_->sock_handle()));
		delete conn_;
		conn_   = NULL;
		status_ = KEEPER_T_IDLE;
	} else {
		conn    = NULL;
		status_ = KEEPER_T_IDLE;
	}

	if (conn_cost_ >= 1000)
		printf("too long const=%.2f\r\n", conn_cost_);
	task.set_conn_cost(conn_cost_);
	task.put(conn);
}

void keeper_conn::connect_one(void)
{
	assert(conn_ == NULL);
	status_ = KEEPER_T_BUSY;
	conn_ = new socket_stream;
	struct timeval begin;
	gettimeofday(&begin, NULL);
	bool ret = conn_->open(addr_, config_.conn_timeo, config_.rw_timeo);
	struct timeval end;
	gettimeofday(&end, NULL);
	conn_cost_ = acl::stamp_sub(end, begin);

	// only for test
	if (conn_cost_ >= 1000) {
		printf("%s(%d): spent: %.2f ms, addr=%s\n",
			__FUNCTION__, __LINE__, conn_cost_, addr_.c_str());
	}

	if (ret) {
		status_     = KEEPER_T_READY;
		last_ctime_ = time(NULL);
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

void keeper_conn::check_idle(void)
{
	if (config_.conn_ttl <= 0) {
		return;
	}
	time_t now = time(NULL);
	if ((now - last_ctime_) >= config_.conn_ttl) {
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
