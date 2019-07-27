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
	// 璁剧疆瀹氭椂鍣紝灏嗙杞崲涓哄井濡�
	handle_.set_timer(this, delay * 1000000);
}

void rpc_timer::stop()
{
	handle_.del_timer(this);
}

void rpc_timer::timer_callback(unsigned int)
{
	rpc_out(); // 杈撳嚭褰撳墠 rpc 闃熷垪鐨勬暟閲�
	rpc_req_out();
	rpc_read_wait_out();
}
