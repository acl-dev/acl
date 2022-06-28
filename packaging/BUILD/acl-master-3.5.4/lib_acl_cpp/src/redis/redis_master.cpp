#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_master.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_master::redis_master(void)
: port_(0)
, link_pending_commands_(0)
, link_refcount_(0)
, last_ping_sent_(0)
, last_ok_ping_reply_(0)
, last_ping_reply_(0)
, down_after_milliseconds_(0)
, info_refresh_(0)
, role_reported_time_(0)
, config_epoch_(0)
, num_slaves_(0)
, num_other_sentinels_(0)
, quorum_(0)
, failover_timeout_(0)
, parallel_syncs_(0)
{
}

}

#endif // ACL_CLIENT_ONLY
