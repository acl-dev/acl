#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

redis_command::redis_command(redis_client* conn /* = NULL */)
: conn_(conn)
{

}

redis_command::~redis_command()
{

}

void redis_command::reset()
{
	if (conn_)
		conn_->reset();
}

void redis_command::set_client(redis_client* conn)
{
	conn_ = conn;
}

const redis_result* redis_command::get_result() const
{
	return conn_ ? conn_->get_result() : NULL;
}

} // namespace acl
