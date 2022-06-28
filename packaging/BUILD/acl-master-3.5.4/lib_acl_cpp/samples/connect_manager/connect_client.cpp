#include "stdafx.h"
#include "connect_client.h"

connect_client::connect_client(const char* addr,
	int conn_timeout, int rw_timeout)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
}

connect_client::~connect_client()
{
}

bool connect_client::open()
{
	return true;
}
