#include "stdafx.hpp"
#include "keeper.hpp"
#include "keeper_conn.hpp"
#include "keeper_conns.hpp"

namespace acl {

keeper_conns::keeper_conns(const keeper_config& config, const char* addr)
: last_peek_(0)
, last_trigger_(0)
, config_(config)
, addr_(addr)
{
	acl_ring_init(&linker_);

	// pre-create fibers to connect server first
	for (int i = 0; i < config_.conn_max; i++) {
		keeper_link* lk = new keeper_link;
		lk->fb = new keeper_conn(config_, addr_, lk, *this);
		ACL_RING_APPEND(&linker_, &lk->me);
		lk->fb->start();
	}
}

keeper_conns::~keeper_conns(void)
{
}

void keeper_conns::on_connect(socket_stream&, keeper_link* lk)
{
	if (lk) {
		ACL_RING_DETACH(&lk->me);
		ACL_RING_APPEND(&linker_, &lk->me);
	}
}

void keeper_conns::trigger_more(void)
{
	time(&last_trigger_);

	int n = 0;
	ACL_RING_ITER iter;
	acl_ring_foreach_reverse(iter, &linker_) {
		keeper_link* lk = acl_ring_to_appl(iter.ptr, keeper_link, me);
		if (!lk->fb->is_idle()) {
			// lk->fb->print_status();
			continue;
		}
		lk->fb->ask_open();
		n++;
		if (config_.conn_min > 0 && n >= config_.conn_min) {
			break;
		}
	}
	logger_debug(FIBER_DEBUG_KEEPER, 1, "[debug] trigger open count=%d", n);
}

// this method will be called by keeper_waiter for adding one request task
// for the keeper_conns with one given addr.
void keeper_conns::add_task(task_req& task)
{
	// try to peek one connection from the connections pool, if found
	// one, move it the task and set hit flag with true, or create one
	// fiber to connect the server addr and set hit flag with false.

	keeper_conn* fb = peek_ready();
	if (fb) {
		task.set_hit(true);
		double cost;
		// peek the real connection from fiber
		socket_stream* conn = fb->peek(cost);
		if (conn == NULL) {
			logger_error("maybe the connection was closed!");
		}
		task.set_conn_cost(cost);

		// notify request task the connection got now
		task.put(conn);
		// notify keeper fiber re-connect the server
		fb->ask_open();
		logger_debug(FIBER_DEBUG_KEEPER, 3, "[debug] addr=%s, hit",
			addr_.c_str());
	} else {
		// if no connection got, create one fiber to real connect
		// the remote server addr now
		task.set_hit(false);
		fb = new keeper_conn(config_, addr_, NULL, *this);
		task.set_stamp();

		// set the request task, when the fiber started,
		// the request task will be executed directly.
		fb->set_task(task);
		fb->start();

		logger_debug(FIBER_DEBUG_KEEPER, 3, "[debug] addr=%s, miss",
			addr_.c_str());

		if (time(NULL) - last_trigger_ >= 1) {
			trigger_more();
		}
	}
}

void keeper_conns::stop(void)
{
	tbox_.push(NULL);
}

void keeper_conns::join(void)
{
	(void) tbox_ctl_.pop();
}

bool keeper_conns::empty(void) const
{
	ACL_RING_ITER iter;
	acl_ring_foreach(iter, &linker_) {
		keeper_link* lk = acl_ring_to_appl(iter.ptr, keeper_link, me);
		if (lk->fb->is_ready()) {
			return false;
		}
	}

	return true;
}

void keeper_conns::run(void)
{
	//int timeo = config_.conn_ttl > 0 ? config_.conn_ttl * 1000 : -1;
	int timeo = 1000;

	while (true) {
		bool found;
		task_req* task = tbox_.pop(timeo, &found);
		if (task == NULL) {
			if (found) {
				break;
			}
		}

		check_idle();
	}

	stop_all();
	done();
}

void keeper_conns::check_idle(void)
{
	int n = 0;
	ACL_RING_ITER iter;
	acl_ring_foreach(iter, &linker_) {
		keeper_link* lk = acl_ring_to_appl(iter.ptr, keeper_link, me);
		if (!lk->fb->is_ready()) {
			break;
		}

		time_t now = time(NULL);
		time_t ctime = lk->fb->get_last_ctime();
		if (ctime > 0 && (now - ctime) >= config_.conn_ttl) {
			lk->fb->ask_close();
			n++;
		}
	}

	logger_debug(FIBER_DEBUG_KEEPER, 2, "[debug] addr=%s, closed count=%d",
		addr_.c_str(), n);
}

int keeper_conns::debug_check(void)
{
	ACL_RING_ITER iter;
	keeper_link*  lk;

	int i = 0;
	acl_ring_foreach(iter, &linker_) {
		lk = acl_ring_to_appl(iter.ptr, keeper_link, me);
		i++;
		if (lk->fb->is_ready()) {
			return i;
		}
	}
	return -1;
}

keeper_conn* keeper_conns::peek_ready(void)
{
#if 0
	keeper_link* lk = NULL;
	ACL_RING_ITER iter;
	acl_ring_foreach(iter, &linker_) {
		lk = acl_ring_to_appl(iter.ptr, keeper_link, me);
		if (lk->fb->is_ready()) {
			break;
		}
	}
#else
	keeper_link* lk = ACL_RING_FIRST_APPL(&linker_, keeper_link, me);
#endif

	if (lk == NULL) {
		logger_error("first keeper_conn null");
		return NULL;
	}

	if (!lk->fb->is_ready()) {
		//lk->fb->print_status();
		return NULL;
	}

	// return the first keeper fiber and re-append it to the tail
	ACL_RING_DETACH(&lk->me);
	ACL_RING_PREPEND(&linker_, &lk->me);

	return lk->fb;
}

void keeper_conns::stop_all(void)
{
	while (true) {
		ACL_RING* hdr = acl_ring_pop_head(&linker_);
		if (hdr == NULL) {
			break;
		}
		keeper_link* lk = acl_ring_to_appl(hdr, keeper_link, me);
		lk->fb->stop();
		lk->fb->join();
		delete lk->fb;
		delete lk;
	}

	acl_ring_init(&linker_);
}

void keeper_conns::done(void)
{
	tbox_ctl_.push(NULL);
}

//////////////////////////////////////////////////////////////////////////////

keeper_killer::keeper_killer(keeper_conns* pool)
: pool_(pool)
{
}

keeper_killer::~keeper_killer(void) {}

void keeper_killer::run(void)
{
	pool_->stop();
	pool_->join();
	delete pool_;
	delete this;
}

} // namespace acl
