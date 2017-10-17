#include "stdafx.hpp"
#include "acl_cpp/stdlib/trigger.hpp"
#include "fiber/fiber.hpp"

namespace acl {

fiber::fiber(bool running /* = false */)
{
	if (running)
	{
		f_ = acl_fiber_running();
		if (f_ == NULL)
			acl_msg_fatal("%s(%d), %s: current fiber not running!",
				__FILE__, __LINE__, __FUNCTION__);
	}
	else
		f_ = NULL;
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
	if (f_)
		acl_fiber_set_errno(f_, errnum);
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

	if (fb)
		acl_fiber_ready(f.get_fiber());
}

unsigned int fiber::delay(unsigned int milliseconds)
{
	return acl_fiber_delay(milliseconds);
}

void fiber::hook_api(bool on)
{
	acl_fiber_hook_api(on ? 1 : 0);
}

ACL_FIBER *fiber::get_fiber(void) const
{
	return f_;
}

void fiber::run(void)
{
	acl_msg_fatal("%s(%d), %s: base function be called",
		__FILE__, __LINE__, __FUNCTION__);
}

void fiber::start(size_t stack_size /* = 64000 */)
{
	if (f_ != NULL)
		acl_msg_fatal("%s(%d), %s: fiber-%u, already running!",
			__FILE__, __LINE__, __FUNCTION__, self());

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
	if (f_ == NULL)
		return false;
	else if (acl_fiber_killed(f_))
		return true;
	acl_fiber_kill(f_);
	return true;
}

bool fiber::killed(void) const
{
	if (f_ != NULL)
		return acl_fiber_killed(f_) != 0;

	acl_msg_error("%s(%d), %s: f_ NULL", __FILE__, __LINE__, __FUNCTION__);
	return true;
}

bool fiber::self_killed(void)
{
	ACL_FIBER* curr = acl_fiber_running();
	if (curr == NULL)
		return false;
	return acl_fiber_killed(curr);
}

void fiber::schedule(void)
{
	acl_fiber_schedule();
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
	return acl_fiber_sys_errno();
}

void fiber::set_sys_errno(int errnum)
{
	acl_fiber_sys_errno_set(errnum);
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

//////////////////////////////////////////////////////////////////////////////

#ifdef	ACL_USE_CPP11

namespace acl
{

class fiber_ctx
{
public:
	fiber_ctx(std::function<void()> fn) : fn_(fn) {}
	~fiber_ctx() = default;

	std::function<void()> fn_;
};

static void fiber_main(ACL_FIBER*, void* ctx)
{
	fiber_ctx* fc = (fiber_ctx *) ctx;
	std::function<void()> fn = fc->fn_;
	delete fc;

	fn();
}

void go_fiber::operator=(std::function<void()> fn)
{
	fiber_ctx* ctx = new fiber_ctx(fn);
	acl_fiber_create(fiber_main, (void*) ctx, stack_size_);
}

} // namespace acl

#endif // ACL_USE_CPP11
