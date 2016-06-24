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

} // namespace acl
