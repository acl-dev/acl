#include "stdafx.hpp"
#include "fiber/fiber.hpp"

namespace acl {

fiber::fiber(bool running /* = false */)
{
	if (running) {
		f_ = acl_fiber_running();
		if (f_ == NULL) {
			acl_msg_fatal("%s(%d), %s: current fiber not running!",
				__FILE__, __LINE__, __FUNCTION__);
		}
	} else {
		f_ = NULL;
	}
}

fiber::~fiber(void)
{
}

unsigned int fiber::get_id(void) const
{
	return f_ ? acl_fiber_id(f_) : 0;
}

unsigned int fiber::self(void)
{
	return acl_fiber_self();
}

int fiber::get_errno(void) const
{
	return f_ ? acl_fiber_errno(f_) : -1;
}

void fiber::set_errno(int errnum)
{
	if (f_) {
		acl_fiber_set_errno(f_, errnum);
	}
}

const char* fiber::last_serror(void)
{
	return acl_fiber_last_serror();
}

int fiber::last_error(void)
{
	return acl_fiber_last_error();
}

const char* fiber::strerror(int errnum, char* buf, size_t size)
{
	return acl_fiber_strerror(errnum, buf, size);
}

void fiber::yield(void)
{
	(void) acl_fiber_yield();
}

void fiber::switch_to_next(void)
{
	acl_fiber_switch();
}

void fiber::ready(fiber& f)
{
	ACL_FIBER *fb = f.get_fiber();

	if (fb) {
		acl_fiber_ready(f.get_fiber());
	}
}

unsigned int fiber::delay(unsigned int milliseconds)
{
	return acl_fiber_delay(milliseconds);
}

unsigned fiber::alive_number(void)
{
	return acl_fiber_number();
}

unsigned fiber::dead_number(void)
{
	return acl_fiber_ndead();
}

void fiber::hook_api(bool on)
{
	acl_fiber_hook_api(on ? 1 : 0);
}

ACL_FIBER *fiber::get_fiber(void) const
{
	return f_;
}

void fiber::acl_io_hook(void)
{
	acl_set_accept(acl_fiber_accept);
	acl_set_connect(acl_fiber_connect);
	acl_set_recv(acl_fiber_recv);
	acl_set_send(acl_fiber_send);
	acl_set_poll(acl_fiber_poll);
	acl_set_select(acl_fiber_select);
	acl_set_close_socket(acl_fiber_close);
}

#if !defined(_WIN32) && !defined(_WIN64)
#include <poll.h>
#endif

void fiber::acl_io_unlock(void)
{
	acl_set_accept(accept);
	acl_set_connect(connect);
	acl_set_recv((acl_recv_fn) recv);
	acl_set_send((acl_send_fn) send);
#if defined(_WIN32) || defined(_WIN64)
	acl_set_poll(WSAPoll);
	acl_set_close_socket(closesocket);
#else
	acl_set_poll(poll);
	acl_set_close_socket(close);
#endif
	acl_set_select(select);
}

void fiber::run(void)
{
	acl_msg_fatal("%s(%d), %s: base function be called",
		__FILE__, __LINE__, __FUNCTION__);
}

void fiber::start(size_t stack_size /* = 64000 */)
{
	if (f_ != NULL) {
		acl_msg_fatal("%s(%d), %s: fiber-%u, already running!",
			__FILE__, __LINE__, __FUNCTION__, self());
	}
	acl_fiber_create(fiber_callback, this, stack_size);
}

void fiber::fiber_callback(ACL_FIBER *f, void *ctx)
{
	fiber* me = (fiber *) ctx;
	me->f_ = f;
	me->run();
}

bool fiber::kill(void)
{
	if (f_ == NULL) {
		return false;
	} else if (acl_fiber_killed(f_)) {
		return true;
	}
	acl_fiber_kill(f_);
	return true;
}

bool fiber::killed(void) const
{
	if (f_ != NULL) {
		return acl_fiber_killed(f_) != 0;
	}
	acl_msg_error("%s(%d), %s: f_ NULL", __FILE__, __LINE__, __FUNCTION__);
	return true;
}

bool fiber::self_killed(void)
{
	ACL_FIBER* curr = acl_fiber_running();
	if (curr == NULL) {
		return false;
	}
	return acl_fiber_killed(curr) ? true : false;
}

void fiber::init(fiber_event_t type, bool schedule_auto /* = false */)
{
	int etype;

	switch (type) {
	case FIBER_EVENT_T_POLL:
		etype = FIBER_EVENT_POLL;
		break;
	case FIBER_EVENT_T_SELECT:
		etype = FIBER_EVENT_SELECT;
		break;
	case FIBER_EVENT_T_WMSG:
		etype = FIBER_EVENT_WMSG;
		break;
	case FIBER_EVENT_T_KERNEL:
	default:
		etype = FIBER_EVENT_KERNEL;
		break;
	}

	acl_fiber_schedule_init(schedule_auto ? 1 : 0);
	acl_fiber_schedule_set_event(etype);
}

void fiber::schedule(void)
{
	acl_fiber_schedule();
}

void fiber::schedule_with(fiber_event_t type)
{
	int etype;

	switch (type) {
	case FIBER_EVENT_T_POLL:
		etype = FIBER_EVENT_POLL;
		break;
	case FIBER_EVENT_T_SELECT:
		etype = FIBER_EVENT_SELECT;
		break;
	case FIBER_EVENT_T_WMSG:
		etype = FIBER_EVENT_WMSG;
		break;
	case FIBER_EVENT_T_KERNEL:
	default:
		etype = FIBER_EVENT_KERNEL;
		break;
	}

	acl_fiber_schedule_with(etype);
}

bool fiber::scheduled(void)
{
	return acl_fiber_scheduled() != 0;
}

void fiber::schedule_stop(void)
{
	acl_fiber_schedule_stop();
}

int fiber::get_sys_errno(void)
{
	return acl_fiber_last_error();
}

void fiber::set_sys_errno(int errnum)
{
	acl_fiber_set_error(errnum);
}

void fiber::stdout_open(bool on)
{
	acl_fiber_msg_stdout_enable(on ? 1 : 0);
}

void fiber::fiber_create(void (*fn)(ACL_FIBER*, void*), void* ctx, size_t size)
{
	acl_fiber_create(fn, (void*) ctx, size);
}

//////////////////////////////////////////////////////////////////////////////

fiber_timer::fiber_timer(void)
{
}

void fiber_timer::start(unsigned int milliseconds, size_t stack_size)
{
	acl_fiber_create_timer(milliseconds, stack_size, timer_callback, this);
}

void fiber_timer::timer_callback(ACL_FIBER* f, void* ctx)
{
	fiber_timer* me = (fiber_timer *) ctx;
	me->f_ = f;
	me->run();
}

} // namespace acl
