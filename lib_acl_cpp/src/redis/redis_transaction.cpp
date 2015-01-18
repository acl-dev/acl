#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_transaction.hpp"

namespace acl
{

redis_transaction::redis_transaction(redis_client* conn /* = NULL */)
: conn_(conn)
, result_(NULL)
{

}

redis_transaction::~redis_transaction()
{

}

void redis_transaction::reset()
{
	if (conn_)
		conn_->reset();
	cmds_.clear();
}

void redis_transaction::set_client(redis_client* conn)
{
	conn_ = conn;
}

bool redis_transaction::watch(const std::vector<string>& keys)
{
	const string& req = conn_->build("WATCH", NULL, keys);
	result_ = conn_->run(req);
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0)
		return false;
	else
		return true;
}

bool redis_transaction::unwatch(const std::vector<string>& keys)
{
	const string& req = conn_->build("UNWATCH", NULL, keys);
	result_ = conn_->run(req);
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0)
		return false;
	else
		return true;
}

bool redis_transaction::multi()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "MULTI";
	lens[0] = sizeof("MULTI") - 1;

	const string& req = conn_->build_request(1, argv, lens);
	result_ = conn_->run(req);
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0)
		return false;
	return true;
}

bool redis_transaction::exec()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "EXEC";
	lens[0] = sizeof("EXEC") - 1;

	const string& req = conn_->build_request(1, argv, lens);
	result_ = conn_->run(req);
	if(result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size = result_->get_size();
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

	const string& req = conn_->build_request(1, argv, lens);
	result_ = conn_->run(req);
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "OK") != 0)
		return false;
	return true;
}

bool redis_transaction::queue_cmd(const char* cmd, const char* argv[],
	const size_t lens[], size_t argc)
{
	const string& req = conn_->build(cmd, NULL, argv, lens, argc);
	result_ = conn_->run(req);
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result_->get_status();
	if (status == NULL || strcasecmp(status, "QUEUED") != 0)
		return false;
	cmds_.push_back(cmd);
	return true;
}

size_t redis_transaction::get_size()
{
	if (result_ == NULL)
		return 0;
	return result_->get_size();
}

const redis_result* redis_transaction::get_child(size_t i, string* cmd)
{
	if (cmd != NULL)
	{
		if (i < cmds_.size())
			*cmd = cmds_[i];
	}

	if (result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY)
		return NULL;
	return result_->get_child(i);
}

} // namespace acl
