#include "stdafx.h"
#include "gc_timer.h"

gc_timer::gc_timer(acl::aio_handle& handle)
: acl::aio_timer_callback(true)
, handle_(handle)
{
}

gc_timer::~gc_timer()
{
}

void gc_timer::destroy()
{
	delete this;
}

void gc_timer::start(int delay)
{
	// 设置定时器，将秒转换为微妙
	handle_.set_timer(this, delay * 1000000);
}

void gc_timer::stop()
{
	handle_.del_timer(this);
}

void gc_timer::timer_callback(unsigned int)
{
	int n = acl_mem_slice_gc();
	if (n > 0)
		logger("slice_gc: %d", n);
}
