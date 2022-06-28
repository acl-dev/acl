#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class ACL_CPP_API redis_master
{
public:
	redis_master(void);
	~redis_master(void) {}

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
	time_t config_epoch_;
	unsigned num_slaves_;
	unsigned num_other_sentinels_;
	unsigned quorum_;
	unsigned failover_timeout_;
	unsigned parallel_syncs_;
};

}

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
