#include "StdAfx.h"
#include "db_host.h"

db_host::db_host(unsigned int id, const char* addr,
	long long int count)
: id_(id)
, count_(count)
{
	addr_ = acl_mystrdup(addr);
}

db_host::~db_host(void)
{
	acl_myfree(addr_);
}

//////////////////////////////////////////////////////////////////////////

idx_host::idx_host(unsigned int id, const char* addr,
	long long int count)
: db_host(id, addr, count)
{
}

idx_host::~idx_host()
{
}

//////////////////////////////////////////////////////////////////////////

dat_host::dat_host(unsigned int id, const char* addr,
	long long int count, int priority)
: db_host(id, addr, count)
{
	priority_ = priority;
}

dat_host::~dat_host()
{
}
