#include "stdafx.h"
#include "rpc_stats.h"
#include "rpc_timer.h"

rpc_timer::rpc_timer(acl::aio_handle& handle)
: acl::aio_timer_callback(true)
, handle_(handle)
{
}

rpc_timer::~rpc_timer()
{
}

void rpc_timer::destroy()
{
	delete this;
}

void rpc_timer::start(int delay)
{
	// 设置定时器，将秒转换为微妙
	handle_.set_timer(this, delay * 1000000);
}

void rpc_timer::stop()
{
	handle_.del_timer(this);
}

void rpc_timer::timer_callback(unsigned int)
{
	rpc_out(); // 输出当前 rpc 队列的数量
	rpc_req_out();
	rpc_read_wait_out();
}
