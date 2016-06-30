#include "stdafx.hpp"
#include "fiber/fiber.hpp"

namespace acl {

fiber::fiber(void)
	: f_(NULL)
{
}

fiber::~fiber(void)
{
}

int fiber::get_id(void) const
{
	return f_ ? fiber_id(f_) : -1;
}

int fiber::self(void)
{
	return fiber_self();
}

int fiber::get_errno(void) const
{
	return f_ ? fiber_errno(f_) : -1;
}

void fiber::set_errno(int errnum)
{
	if (f_)
		fiber_set_errno(f_, errnum);
}

void fiber::yield(void)
{
	(void) fiber_yield();
}

void fiber::switch_to_next(void)
{
	fiber_switch();
}

void fiber::ready(fiber& f)
{
	FIBER *fb = f.get_fiber();

	if (fb)
		fiber_ready(f.get_fiber());
}

void fiber::hook_api(bool on)
{
	fiber_hook_api(on ? 1 : 0);
}

FIBER *fiber::get_fiber(void) const
{
	return f_;
}

void fiber::start(size_t stack_size /* = 64000 */)
{
	fiber_create(fiber_callback, this, stack_size);
}

void fiber::fiber_callback(FIBER *f, void *ctx)
{
	fiber* me = (fiber *) ctx;
	me->f_ = f;

	me->run();
}

void fiber::schedule(void)
{
	fiber_schedule();
}

void fiber::stop(void)
{
	fiber_io_stop();
}

} // namespace acl
