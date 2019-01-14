#include "stdafx.hpp"
#include "keeper.hpp"
#include "keeper_conn.hpp"
#include "keeper_conns.hpp"

namespace acl {

keeper_conns::keeper_conns(const keeper_config& config, const char* addr)
: last_peek_(0)
, config_(config)
, addr_(addr)
{
	acl_ring_init(&linker_);

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

void keeper_conns::on_connect(socket_stream& conn, keeper_link* lk)
{
	if (lk) {
		ACL_RING_DETACH(&lk->me);
		ACL_RING_APPEND(&linker_, &lk->me);
	} else {
		conn.set_tcp_solinger(true, 0);
	}
}

void keeper_conns::add_task(task_req& task)
{
	keeper_conn* fb = peek_ready();
	if (fb) {
		task.set_hit(true);
		double cost;
		socket_stream* conn = fb->peek(cost);
		assert(conn);
		task.set_conn_cost(cost);
		task.put(conn);
		fb->ask_connect();
		if (0)
			printf(">>>ready hit\r\n");
	} else {
		task.set_hit(false);
		fb = new keeper_conn(config_, addr_, NULL, *this);
		task.set_stamp();
		fb->set_task(task);
		fb->start();

		if (0)
			printf(">>>>trigger connect one, hit=%d\r\n",
					this->debug_check());
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
	keeper_link* lk;
	acl_ring_foreach(iter, &linker_) {
		lk = acl_ring_to_appl(iter.ptr, keeper_link, me);
		if (lk->fb->is_ready()) {
			return false;
		}
	}

	return true;
}

void keeper_conns::run(void)
{
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
	}

	stop_all();
	done();
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
