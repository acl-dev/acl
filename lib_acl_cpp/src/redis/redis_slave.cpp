#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_slave.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_slave::redis_slave(void)
: port_(0)
, link_pending_commands_(0)
, link_refcount_(0)
, last_ping_sent_(0)
, last_ok_ping_reply_(0)
, last_ping_reply_(0)
, down_after_milliseconds_(0)
, info_refresh_(0)
, role_reported_time_(0)
, master_link_down_time_(0)
, master_port_(0)
, slave_priority_(0)
, slave_repl_offset_(0)
{
}

}

#endif // ACL_CLIENT_ONLY
