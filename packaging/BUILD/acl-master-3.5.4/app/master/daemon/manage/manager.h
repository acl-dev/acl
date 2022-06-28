#pragma once

#include "http_server.h"

class manager : public acl::singleton<manager>
{
public:
	manager(void);
	~manager(void);

	void init(ACL_EVENT* event, const char* addr, int timeout);
	ACL_AIO* get_aio(void) const
	{
		return aio_;
	}

private:
	ACL_EVENT*       event_;
	ACL_AIO*         aio_;
	acl::aio_handle* handle_;
	http_server*     server_;
};
