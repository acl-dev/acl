#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pipeline.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

redis_reader::redis_reader(tbox<redis_command>& box)
: box_(box)
{
}

redis_reader::~redis_reader(void) {}

void* redis_reader::run(void)
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

redis_client_pipeline::redis_client_pipeline(const char* addr, int conn_timeout,
	int rw_timeout, bool retry)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, retry_(retry)
{
}

redis_client_pipeline::~redis_client_pipeline(void) {}

void redis_client_pipeline::push(redis_command* cmd)
{
	box_.push(cmd);
}

void* redis_client_pipeline::run(void)
{
	reader_ = NEW redis_reader(box_);
	reader_->start();

	std::vector<redis_command*> cmds;
	int timeout = -1;
	bool found;
	while (true) {
		redis_command* cmd = box_.pop(timeout, &found);
		if (cmd != NULL) {
			cmds.push_back(cmd);
			timeout = 0;
		} else if (found) {
			break;
		} else {
			timeout = -1;
			send(cmds);
			cmds.clear();
		}
	}

	return NULL;
}

void redis_client_pipeline::send(std::vector<redis_command*>& cmds)
{
	(void) cmds;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
