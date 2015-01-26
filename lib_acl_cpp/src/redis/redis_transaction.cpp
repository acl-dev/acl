#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_transaction.hpp"

namespace acl
{

redis_transaction::redis_transaction(redis_client* conn /* = NULL */)
: redis_command(conn)
{

}

redis_transaction::~redis_transaction()
{

}

bool redis_transaction::watch(const std::vector<string>& keys)
{
	conn_->build("WATCH", NULL, keys);
	return conn_->get_status();
}

bool redis_transaction::unwatch(const std::vector<string>& keys)
{
	conn_->build("UNWATCH", NULL, keys);
	return conn_->get_status();
}

bool redis_transaction::multi()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "MULTI";
	lens[0] = sizeof("MULTI") - 1;

	conn_->build_request(1, argv, lens);
	return conn_->get_status();
}

bool redis_transaction::exec()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "EXEC";
	lens[0] = sizeof("EXEC") - 1;

	conn_->build_request(1, argv, lens);
	const redis_result* result = conn_->run();
	if(result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size = result->get_size();
	if (size != cmds_.size())
		return false;
	return true;
}

bool redis_transaction::discard()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "DISCARD";
	lens[0] = sizeof("DISCARD") - 1;

	conn_->build_request(1, argv, lens);
	return conn_->get_status();
}

bool redis_transaction::queue_cmd(const char* cmd, const char* argv[],
	const size_t lens[], size_t argc)
{
	conn_->build(cmd, NULL, argv, lens, argc);
	if (conn_->get_status("QUEUED") == false)
		return false;
	cmds_.push_back(cmd);
	return true;
}

size_t redis_transaction::get_size() const
{
	return conn_->get_size();
}

const redis_result* redis_transaction::get_child(size_t i, string* cmd) const
{
	if (cmd != NULL)
	{
		if (i < cmds_.size())
			*cmd = cmds_[i];
	}
	return conn_->get_child(i);
}

} // namespace acl
