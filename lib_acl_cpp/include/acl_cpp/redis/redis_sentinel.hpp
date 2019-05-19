#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "redis_command.hpp"
#include "redis_master.hpp"
#include "redis_slave.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class ACL_CPP_API redis_sentinel : virtual public redis_command
{
public:
	redis_sentinel(void);
	redis_sentinel(redis_client* conn);
	virtual ~redis_sentinel(void);

	bool sentinel_master(const char* name, redis_master& out);
	bool sentinel_masters(std::vector<redis_master>& out);
	bool sentinel_slaves(const char* master_name,
		std::vector<redis_slave>& out);

	bool sentinel_get_master_addr_by_name(const char* master_name,
		string& ip, int& port);
	int sentinel_reset(const char* pattern);
	bool sentinel_failover(const char* master_name);

	bool sentinel_flushconfig(void);
	bool sentinel_remove(const char* master_name);
	bool sentinel_monitor(const char* master_name, const char* ip,
		int port, int quorum);
	bool sentinel_set(const char* master_name, const char* name,
		const char* value);
	bool sentinel_set(const char* master_name, const char* name,
		unsigned value);
};

}

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
