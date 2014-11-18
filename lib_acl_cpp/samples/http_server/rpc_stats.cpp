#include "stdafx.h"
#include "rpc_stats.h"

static int __count = 0;

void rpc_add()
{
	__count++;
}

void rpc_del()
{
	__count--;
}

void rpc_out()
{
	printf(">> rpc_count: %d\r\n", __count);
}

//////////////////////////////////////////////////////////////////////////

static locker* __lock = NULL;
static int  __req = 0;

void rpc_stats_init()
{
	__lock = new locker();
}

void rpc_stats_finish()
{
	delete __lock;
}

void rpc_req_add()
{
	__lock->lock();
	__req++;
	__lock->unlock();
}

void rpc_req_del()
{
	__lock->lock();
	__req--;
	__lock->unlock();
}

void rpc_req_out()
{
	__lock->lock();
	printf(">> req_count: %d\r\n", __req);
	__lock->unlock();
}

//////////////////////////////////////////////////////////////////////////

static int __read_wait = 0;

void rpc_read_wait_add()
{
	__read_wait++;
}

void rpc_read_wait_del()
{
	__read_wait--;
}

void rpc_read_wait_out()
{
	printf(">> read wait count: %d\r\n", __read_wait);
}
