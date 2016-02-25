#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_transaction.hpp"
#endif

namespace acl
{

redis_transaction::redis_transaction()
: redis_command(NULL)
{
}

redis_transaction::redis_transaction(redis_client* conn)
: redis_command(conn)
{
}

redis_transaction::redis_transaction(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_transaction::~redis_transaction()
{
}

bool redis_transaction::watch(const std::vector<string>& keys)
{
	build("WATCH", NULL, keys);
	return check_status();
}

bool redis_transaction::unwatch()
{
	const char* argv[1];
	size_t lens[1];

	build_request(1, argv, lens);
	return check_status();
}

bool redis_transaction::multi()
{
	cmds_.clear();

	const char* argv[1];
	size_t lens[1];

	argv[0] = "MULTI";
	lens[0] = sizeof("MULTI") - 1;

	build_request(1, argv, lens);
	return check_status();
}

bool redis_transaction::exec()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "EXEC";
	lens[0] = sizeof("EXEC") - 1;

	build_request(1, argv, lens);
	const redis_result* result = run();
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

	build_request(1, argv, lens);
	return check_status();
}

bool redis_transaction::run_cmd(const char* cmd, const char* argv[],
	const size_t lens[], size_t argc)
{
	build(cmd, NULL, argv, lens, argc);
	if (check_status("QUEUED") == false)
		return false;

	cmds_.push_back(cmd);
	return true;
}

bool redis_transaction::run_cmd(const char* cmd,
	const std::vector<string>& args)
{
	build(cmd, NULL, args);
	if (check_status("QUEUED") == false)
		return false;

	cmds_.push_back(cmd);
	return true;
}

size_t redis_transaction::get_size() const
{
	return result_size();
}

const redis_result* redis_transaction::get_child(size_t i, string* cmd) const
{
	if (cmd != NULL)
	{
		if (i < cmds_.size())
			*cmd = cmds_[i];
	}
	return result_child(i);
}

} // namespace acl
