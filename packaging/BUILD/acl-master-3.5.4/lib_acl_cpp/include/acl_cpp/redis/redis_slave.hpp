#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class ACL_CPP_API redis_slave
{
public:
	redis_slave(void);
	~redis_slave(void) {}

	string name_;
	string ip_;
	int port_;
	string runid_;
	string flags_;
	unsigned link_pending_commands_;
	unsigned link_refcount_;
	unsigned last_ping_sent_;
	unsigned last_ok_ping_reply_;
	unsigned last_ping_reply_;
	unsigned down_after_milliseconds_;
	unsigned info_refresh_;
	string role_reported_;
	time_t role_reported_time_;
	time_t master_link_down_time_;
	string master_link_status_;
	string master_host_;
	int master_port_;
	unsigned slave_priority_;
	unsigned long slave_repl_offset_;
};

}

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
