#include "stdafx.h"
#include "manager.h"

manager::manager(void)
: event_(NULL)
, aio_(NULL)
, handle_(NULL)
, server_(NULL)
{
}

manager::~manager(void)
{
	delete server_;
	delete handle_;
	if (aio_)
		acl_aio_free2(aio_, 1);
}

void manager::init(ACL_EVENT* event, const char* addr, int rw_timeout)
{
	acl_assert(event);

	if (addr == NULL || *addr == 0)
		return;

	event_  = event;
	aio_    = acl_aio_create3(event);
	handle_ = new acl::aio_handle(aio_);
	server_ = new http_server(*handle_, rw_timeout);

	server_->open(addr);
}
