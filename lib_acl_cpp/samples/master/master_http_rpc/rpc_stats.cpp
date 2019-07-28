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
	logger(">> rpc_count: %d <<", __count);
}

//////////////////////////////////////////////////////////////////////////

static acl::locker* __lock;
static int  __req = 0;

void rpc_stats_init()
{
	__lock = new acl::locker();
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
	logger(">> req_count: %d <<", __req);
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
	logger(">> read wait count: %d <<", __read_wait);
}
