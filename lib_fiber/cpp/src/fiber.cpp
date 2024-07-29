#include "stdafx.hpp"
#include "fiber/fiber.hpp"
#include "winapi_hook.hpp"

namespace acl {

fiber::fiber()
{
	f_ = NULL;
}

fiber::fiber(ACL_FIBER *f)
{
	f_ = f;
}

fiber::fiber(bool running)
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

fiber::~fiber()
{
}

unsigned int fiber::get_id() const
{
	return f_ ? acl_fiber_id(f_) : 0;
}

unsigned int fiber::self()
{
	return acl_fiber_self();
}

unsigned int fiber::fiber_id(const fiber& fb)
{
	ACL_FIBER *f = fb.get_fiber();
	return f ? acl_fiber_id(f) : 0;
}

int fiber::get_errno() const
{
	return f_ ? acl_fiber_errno(f_) : -1;
}

bool fiber::is_ready() const
{
	return f_ ? acl_fiber_status(f_) == FIBER_STATUS_READY : false;
}

bool fiber::is_suspended() const
{
	return f_ ? acl_fiber_status(f_) == FIBER_STATUS_SUSPEND : false;
}

void fiber::set_errno(int errnum)
{
	if (f_) {
		acl_fiber_set_errno(f_, errnum);
	}
}

void fiber::clear()
{
	ACL_FIBER *curr = acl_fiber_running();
	if (curr) {
		acl_fiber_clear(curr);
	}
}


const char* fiber::last_serror()
{
	return acl_fiber_last_serror();
}

int fiber::last_error()
{
	return acl_fiber_last_error();
}

const char* fiber::strerror(int errnum, char* buf, size_t size)
{
	return acl_fiber_strerror(errnum, buf, size);
}

void fiber::yield()
{
	(void) acl_fiber_yield();
}

void fiber::switch_to_next()
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

size_t fiber::delay(size_t milliseconds)
{
	return acl_fiber_delay(milliseconds);
}

unsigned fiber::alive_number()
{
	return acl_fiber_number();
}

unsigned fiber::dead_number()
{
	return acl_fiber_ndead();
}

void fiber::set_non_blocking(bool yes)
{
	acl_fiber_set_non_blocking(yes ? 1 : 0);
}

void fiber::set_shared_stack_size(size_t size)
{
	acl_fiber_set_shared_stack_size(size);
}

size_t fiber::get_shared_stack_size()
{
	return acl_fiber_get_shared_stack_size();
}

ACL_FIBER *fiber::get_fiber() const
{
	return f_;
}

void fiber::acl_io_hook()
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

void fiber::acl_io_unlock()
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

#include "winapi_hook.hpp"

bool fiber::winapi_hook() {
	return ::winapi_hook();
}

void fiber::run()
{
	acl_msg_fatal("%s(%d), %s: base function be called",
		__FILE__, __LINE__, __FUNCTION__);
}

void fiber::start(size_t stack_size /* 64000 */, bool share_stack /* false */)
{
	if (f_ != NULL) {
		acl_msg_fatal("%s(%d), %s: fiber-%u, already running!",
			__FILE__, __LINE__, __FUNCTION__, self());
	}

	ACL_FIBER_ATTR attr;
	acl_fiber_attr_init(&attr);
	acl_fiber_attr_setstacksize(&attr, stack_size);
	acl_fiber_attr_setsharestack(&attr, share_stack ? 1 : 0);

	acl_fiber_create2(&attr, fiber_callback, this);
}

void fiber::fiber_callback(ACL_FIBER *f, void *ctx)
{
	fiber* me = (fiber *) ctx;
	me->f_ = f;
	me->run();
}

bool fiber::kill(bool sync)
{
	if (f_ == NULL) {
		return false;
	} else if (acl_fiber_killed(f_)) {
		return true;
	}
	if (sync) {
		acl_fiber_kill_wait(f_);
	} else {
		acl_fiber_kill(f_);
	}
	return true;
}

bool fiber::killed() const
{
	if (f_ != NULL) {
		return acl_fiber_killed(f_) != 0;
	}
	acl_msg_error("%s(%d), %s: f_ NULL", __FILE__, __LINE__, __FUNCTION__);
	return true;
}

bool fiber::self_killed()
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
	case FIBER_EVENT_T_IO_URING:
		etype = FIBER_EVENT_IO_URING;
		break;
	case FIBER_EVENT_T_KERNEL:
	default:
		etype = FIBER_EVENT_KERNEL;
		break;
	}

	acl_fiber_schedule_init(schedule_auto ? 1 : 0);
	acl_fiber_schedule_set_event(etype);
}

void fiber::schedule(fiber_event_t type /* FIBER_EVENT_T_KERNEL */)
{
	schedule_with(type);
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
	case FIBER_EVENT_T_IO_URING:
		etype = FIBER_EVENT_IO_URING;
		break;
	case FIBER_EVENT_T_KERNEL:
	default:
		etype = FIBER_EVENT_KERNEL;
		break;
	}

	if (!winapi_hook()) {
		perror("hook API for windows error");
	}
	acl_fiber_schedule_with(etype);
}

bool fiber::scheduled()
{
	return acl_fiber_scheduled() != 0;
}

void fiber::schedule_stop()
{
	acl_fiber_schedule_stop();
}

int fiber::get_sys_errno()
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

int fiber::set_fdlimit(int max)
{
	return acl_fiber_set_fdlimit(max);
}

ACL_FIBER* fiber::fiber_create(void (*fn)(ACL_FIBER*, void*), void* ctx,
	size_t stack_size, bool share_stack /* false */)
{
	ACL_FIBER_ATTR attr;
	acl_fiber_attr_init(&attr);
	acl_fiber_attr_setstacksize(&attr, stack_size);
	acl_fiber_attr_setsharestack(&attr, share_stack ? 1 : 0);

	return acl_fiber_create2(&attr, fn, ctx);
}

void fiber::stacktrace(const fiber& fb, std::vector<fiber_frame>& out, size_t max)
{
	ACL_FIBER *f = fb.get_fiber();
	ACL_FIBER_STACK *stack = acl_fiber_stacktrace(f, max);
	if (stack == NULL) {
		return;
	}

	for (size_t i = 0; i < stack->count; i++) {
		fiber_frame frame;
		frame.func = stack->frames[i].func;
		frame.pc   = stack->frames[i].pc;
		frame.off  = stack->frames[i].off;
		out.push_back(frame);
	}

	acl_fiber_stackfree(stack);
}

void fiber::stackshow(const fiber& fb, size_t max /* = 50 */)
{
	std::vector<fiber_frame> stack;
	stacktrace(fb, stack, max);

	for (std::vector<fiber_frame>::const_iterator cit = stack.begin();
		cit != stack.end(); ++cit) {

		printf("0x%lx(%s)+0x%lx\r\n",
			(*cit).pc, (*cit).func.c_str(), (*cit).off);
	}
}

//////////////////////////////////////////////////////////////////////////////

fiber_timer::fiber_timer()
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
