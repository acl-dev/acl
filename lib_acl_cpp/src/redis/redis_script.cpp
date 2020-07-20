#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_script.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define LONG_LEN	21

redis_script::redis_script()
{
}

redis_script::redis_script(redis_client* conn)
: redis_command(conn)
{
}

redis_script::redis_script(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_script::redis_script(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_script::~redis_script()
{
}

bool redis_script::eval_status(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	const char* success /* = "OK" */)
{
	const redis_result* result = eval_cmd("EVAL", script, keys, args);
	if (result == NULL)
		return false;
	const char* status = result->get_status();
	if (status == NULL || strcasecmp(status, success) != 0)
		return false;
	return true;
}

bool redis_script::eval_number(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	int& out)
{
	const redis_result* result = eval_cmd("EVAL", script, keys, args);
	if (result == NULL)
		return false;

	bool success;
	out = result->get_integer(&success);
	return success;
}

bool redis_script::eval_number64(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	long long int& out)
{
	const redis_result* result = eval_cmd("EVAL", script, keys, args);
	if (result == NULL)
		return false;

	bool success;
	out = result->get_integer64(&success);
	return success;
}

int redis_script::eval_string(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	string& out)
{
	const redis_result* result = eval_cmd("EVAL", script, keys, args);
	if (result == NULL)
		return -1;

	return result->argv_to_string(out);
}

bool redis_script::evalsha_status(const char* script,
	const std::vector<string>& keys, const std::vector<string>& args,
	const char* success /* = "OK" */)
{
	const redis_result* result = eval_cmd("EVALSHA", script, keys, args);
	if (result == NULL)
		return false;
	const char* status = result->get_status();
	if (status == NULL || strcasecmp(status, success) != 0)
		return false;
	return true;
}

bool redis_script::evalsha_number(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	int& out)
{
	const redis_result* result = eval_cmd("EVALSHA", script, keys, args);
	if (result == NULL)
		return false;

	bool success;
	out = result->get_integer(&success);
	return success;
}

bool redis_script::evalsha_number64(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	long long int& out)
{
	const redis_result* result = eval_cmd("EVALSHA", script, keys, args);
	if (result == NULL)
		return false;

	bool success;
	out = result->get_integer64(&success);
	return success;
}

int redis_script::evalsha_string(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	string& out)
{
	const redis_result* result = eval_cmd("EVALSHA", script, keys, args);
	if (result == NULL)
		return -1;

	return result->argv_to_string(out);
}

int redis_script::eval_status(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<bool>& out,
	const char* success /* = "OK" */)
{
	return eval_status("EVAL", script, keys, args, out, success);
}

int redis_script::eval_number(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<int>& out,
	std::vector<bool>& status)
{
	return eval_number("EVAL", script, keys, args, out, status);
}

long long int redis_script::eval_number64(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<long long int>& out,
	std::vector<bool>& status)
{
	return eval_number64("EVAL", script, keys, args, out, status);
}

int redis_script::eval_strings(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<string>& out)
{
	return eval_strings("EVAL", script, keys, args, out);
}

int redis_script::evalsha_status(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<bool>& out,
	const char* success /* = "OK" */)
{
	return eval_status("EVALSHA", script, keys, args, out, success);
}

int redis_script::evalsha_number(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<int>& out,
	std::vector<bool>& status)
{
	return eval_number("EVALSHA", script, keys, args, out, status);
}

long long int redis_script::evalsha_number64(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<long long int>& out,
	std::vector<bool>& status)
{
	return eval_number64("EVALSHA", script, keys, args, out, status);
}

int redis_script::evalsha_strings(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<string>& out)
{
	return eval_strings("EVALSHA", script, keys, args, out);
}

int redis_script::eval_status(const char* cmd, const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<bool>& out,
	const char* success /* = "OK" */)
{
	const redis_result* result = eval_cmd(cmd, script, keys, args);
	if (result == NULL)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return -1;

	out.reserve(size);

	const redis_result* rr;
	const char* status;
	out.clear();

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		status = rr->get_status();
		if (status != NULL && strcasecmp(status, success) == 0)
			out.push_back(true);
		else
			out.push_back(false);
	}

	return (int) size;
}

int redis_script::eval_number(const char* cmd, const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<int>& out,
	std::vector<bool>& status)
{
	const redis_result* result = eval_cmd(cmd, script, keys, args);
	if (result == NULL)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;

	out.reserve(size);

	const redis_result* rr;
	int number;
	bool success;
	out.clear();
	status.clear();

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		number = rr->get_integer(&success);
		out.push_back(number);
		status.push_back(success);
	}

	return (int) size;
}

long long int redis_script::eval_number64(const char* cmd, const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<long long int>& out,
	std::vector<bool>& status)
{
	const redis_result* result = eval_cmd(cmd, script, keys, args);
	if (result == NULL)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;

	out.clear();
	out.reserve(size);
	status.clear();

	const redis_result* rr;
	long long int number;
	bool success;

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		number = rr->get_integer64(&success);
		out.push_back(number);
		status.push_back(success);
	}

	return (long long int) size;
}

int redis_script::eval_strings(const char* cmd, const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args,
	std::vector<string>& out)
{
	const redis_result* result = eval_cmd(cmd, script, keys, args);
	if (result == NULL)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;

	out.clear();
	out.reserve(size);

	const redis_result* rr;
	string buf;

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		rr->argv_to_string(buf);
		out.push_back(buf);
		buf.clear();
	}

	return (int) size;
}

const redis_result* redis_script::eval(const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args)
{
	return eval_cmd("EVAL", script, keys, args);
}

const redis_result* redis_script::eval(const char* script,
	const std::vector<const char*>& keys,
	const std::vector<const char*>& args)
{
	return eval_cmd("EVAL", script, keys, args);
}

const redis_result* redis_script::evalsha(const char* sha1,
	const std::vector<string>& keys,
	const std::vector<string>& args)
{
	return eval_cmd("EVALSHA", sha1, keys, args);
}

const redis_result* redis_script::evalsha(const char* sha1,
	const std::vector<const char*>& keys,
	const std::vector<const char*>& args)
{
	return eval_cmd("EVALSHA", sha1, keys, args);
}

const redis_result* redis_script::eval_cmd(const char* cmd,
	const char* script,
	const std::vector<string>& keys,
	const std::vector<string>& args)
{
	size_t argc = 3 + keys.size() + args.size();
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = script;
	lens[1] = strlen(script);

	char buf[LONG_LEN];
	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) keys.size());
	argv[2] = buf;
	lens[2] = strlen(buf);

	size_t i = 3;
	std::vector<string>::const_iterator cit;

	for (cit = keys.begin(); cit != keys.end(); ++cit) {
		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
		i++;
	}

	for (cit = args.begin(); cit != args.end(); ++cit) {
		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
		i++;
	}

	acl_assert(i == argc);

	if (keys.size() == 1)
		hash_slot(keys[0].c_str());

	build_request(argc, argv, lens);
	return run();
}

const redis_result* redis_script::eval_cmd(const char* cmd,
	const char* script,
	const std::vector<const char*>& keys,
	const std::vector<const char*>& args)
{
	size_t argc = 3 + keys.size() + args.size();
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = script;
	lens[1] = strlen(script);

	char buf[LONG_LEN];
	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) keys.size());
	argv[2] = buf;
	lens[2] = strlen(buf);

	size_t i = 3;
	std::vector<const char*>::const_iterator cit;

	for (cit = keys.begin(); cit != keys.end(); ++cit) {
		argv[i] = *cit;
		lens[i] = strlen(argv[i]);
		i++;
	}

	for (cit = args.begin(); cit != args.end(); ++cit) {
		argv[i] = *cit;
		lens[i] = strlen(argv[i]);
		i++;
	}

	acl_assert(i == argc);

	build_request(argc, argv, lens);
	return run();
}

int redis_script::script_exists(const std::vector<string>& scripts,
	std::vector<bool>& out)
{
	build("SCRIPT", "EXISTS", scripts);
	int ret = get_status(out);
	if (ret != (int) scripts.size())
		return -1;
	return ret;
}

int redis_script::script_exists(const std::vector<const char*>& scripts,
	std::vector<bool>& out)
{
	build("SCRIPT", "EXISTS", scripts);
	int ret = get_status(out);
	if (ret != (int) scripts.size())
		return -1;
	return ret;
}

bool redis_script::script_flush()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SCRIPT";
	lens[0] = sizeof("SCRIPT") - 1;

	argv[1] = "FLUSH";
	lens[1] = sizeof("FLUSH") - 1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_script::script_load(const string& script, string& out)
{
	out.clear();

	const char* argv[3];
	size_t lens[3];

	argv[0] = "SCRIPT";
	lens[0] = sizeof("SCRIPT") - 1;

	argv[1] = "LOAD";
	lens[1] = sizeof("LOAD") - 1;

	argv[2] = script;
	lens[2] = strlen(script);

	build_request(3, argv, lens);
	return get_string(out) > 0 ? true : false;
}

bool redis_script::script_kill()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SCRIPT";
	lens[0] = sizeof("SCRIPT") - 1;

	argv[1] = "KILL";
	lens[1] = sizeof("KILL") - 1;

	build_request(2, argv, lens);
	return check_status();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
