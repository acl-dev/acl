#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#endif

#include "acl_cpp/redis/redis_stream.hpp"

#define INT_LEN		11
#define LONG_LEN	21

namespace acl
{

redis_stream::redis_stream(void)
: redis_command(NULL)
{
}

redis_stream::redis_stream(redis_client* conn)
: redis_command(conn)
{
}

redis_stream::redis_stream(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_stream::~redis_stream(void)
{
}

/////////////////////////////////////////////////////////////////////////////

bool redis_stream::xadd(const char* key, const std::map<string, string>& fields,
	string& result, const char* id /* = "*" */)
{
	if (fields.empty()) {
		logger_error("files empty");
		return false;
	}
	if (id == NULL || *id == 0) {
		logger_error("id null");
		return false;
	}

	build("XADD", key, id, fields);
	return get_string(result) >= 0 ? true : false;
}

bool redis_stream::xadd(const char* key, const std::vector<string>& names,
	const std::vector<string>& values, string& result,
	const char* id /* = "*" */)
{
	if ((names.size() != values.size()) || names.size() == 0) {
		logger_error("fields size invalid, names=%lu, values=%lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
		return false;
	}
	if (id == NULL || *id == 0) {
		logger_error("id null");
		return false;
	}

	build("XADD", key, id, names, values);
	return get_string(result) >= 0 ? true : false;
}

bool redis_stream::xadd(const char* key, const std::vector<const char*>& names,
	const std::vector<const char*>& values, string& result,
	const char* id /* = "*" */)
{
	if ((names.size() != values.size()) || names.size() == 0) {
		logger_error("fields size invalid, names=%lu, values=%lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
		return false;
	}
	if (id == NULL || *id == 0) {
		logger_error("id null");
		return false;
	}

	build("XADD", key, id, names, values);
	return get_string(result) >= 0 ? true : false;
}

bool redis_stream::xadd(const char* key, const char* names[],
	const size_t names_len[], const char* values[],
	const size_t values_len[], size_t argc, string& result,
	const char* id /* = "*" */)
{
	if (argc == 0) {
		logger_error("invalid argc=%lu", (unsigned long) argc);
		return false;
	}

	if (id == NULL || *id == 0) {
		logger_error("id null");
		return false;
	}

	build("XADD", key, id, names, names_len, values, values_len, argc);
	return get_string(result) >= 0 ? true : false;
}

void redis_stream::xread_build(const std::map<string, string>& streams,
	size_t count, size_t block)
{
	argc_ = 6 + streams.size();
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XREAD";
	argv_lens_[i] = sizeof("XREAD") - 1;
	i++;

	char count_s[LONG_LEN];
	if (count > 0) {
		argv_[i] = "COUNT";
		argv_lens_[i] = sizeof("COUNT") - 1;
		i++;

		safe_snprintf(count_s, sizeof(count_s), "%lu",
			(unsigned long) count);
		argv_[i] = count_s;
		argv_lens_[i] = strlen(count_s);
		i++;
	}

	char block_s[LONG_LEN];
	if (block > 0) {
		argv_[i] = "BLOCK";
		argv_lens_[i] = sizeof("BLOCK") - 1;
		i++;

		safe_snprintf(block_s, sizeof(block_s), "%lu",
			(unsigned long) block);
		argv_[i] = block_s;
		argv_lens_[i] = strlen(block_s);
		i++;
	}

	argv_[i] = "STREAMS";
	argv_lens_[i] = sizeof("STREAMS") - 1;
	i++;

	for (std::map<string, string>::const_iterator cit = streams.begin();
		cit != streams.end(); ++cit) {

		argv_[i] = cit->first.c_str();
		argv_lens_[i] = cit->first.size();
		i++;

		argv_[i] = cit->second.c_str();
		argv_lens_[i] = cit->second.size();
		i++;
	}

	build_request(i, argv_, argv_lens_);
}

bool redis_stream::xread(const std::map<string, string>& streams,
	redis_stream_messages& messages, size_t count /* = 0 */,
	size_t block /* = 0 */)
{
	xread_build(streams, count, block);

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (size == 0 || children == NULL) {
		return false;
	}

	for (size_t i = 0; i < size; i++) {
		const redis_result* child = children[i];
		if (child->get_type() != REDIS_RESULT_ARRAY) {
			continue;
		}
		xread_streams_results(*child, messages);
	}

	return true;
}

void redis_stream::xread_streams_results(const redis_result& res,
	redis_stream_messages& messages)
{
	if (res.get_type() != REDIS_RESULT_ARRAY) {
		return;
	}

	size_t size;
	const redis_result** children = res.get_children(&size);
	if (size != 2) {
		return;
	}

	const redis_result* rr = children[0];
	if (rr->get_type() != REDIS_RESULT_STRING) {
		return;
	}
	rr->argv_to_string(messages.key);

	rr = children[1];
	if (rr->get_type() != REDIS_RESULT_ARRAY) {
		return;
	}

	children = rr->get_children(&size);
	if (size == 0 || children == NULL) {
		return;
	}

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		xread_stream_messages(*rr, messages);
	}
}

void redis_stream::xread_stream_messages(const redis_result& child,
	redis_stream_messages& messages)
{
	if (child.get_type() != REDIS_RESULT_ARRAY) {
		return;
	}

	size_t size;
	const redis_result** children = child.get_children(&size);
	if (size == 0 || children == NULL) {
		return;
	}
	if (size != 2) {
		return;
	}

	const redis_result* rr = children[0];
	if (rr->get_type() != REDIS_RESULT_STRING) {
		return;
	}
	string id;
	rr->argv_to_string(id);

	rr = children[1];
	children = rr->get_children(&size);
	if (size == 0 || children == NULL) {
		return;
	}

	std::vector<redis_stream_message> msgs;
	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_ARRAY) {
			continue;
		}

		redis_stream_message msg;
		if (!xread_stream_message(*rr, msg.name, msg.value)) {
			continue;
		}

		msgs.push_back(msg);
	}

	if (!msgs.empty()) {
		messages.messages[id] = msgs;
	}
}

bool redis_stream::xread_stream_message(const redis_result& res,
	string& name, string& value)
{
	if (res.get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}
	size_t size;
	const redis_result** children = res.get_children(&size);
	if (children == NULL || size != 0) {
		return false;
	}

	const redis_result* rr = children[0];
	rr->argv_to_string(name);
	rr = children[1];
	rr->argv_to_string(value);
	return true;
}

bool redis_stream::xgroup_create(const char* key, const char* group,
	const char* id /* = "$" */)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "XGROUP";
	lens[0] = sizeof("XGROUP") - 1;
	argv[1] = "CREATE";
	lens[1] = sizeof("CREATE") - 1;
	argv[2] = key;
	lens[2] = strlen(key);
	argv[3] = group;
	lens[3] = strlen(group);
	argv[4] = id;
	lens[4] = strlen(id);
	build_request(5, argv, lens);
	return check_status();
}

int redis_stream::xgroup_destroy(const char* key, const char* group)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "XGROUP";
	lens[0] = sizeof("XGROUP") - 1;
	argv[1] = "DESTROY";
	lens[1] = sizeof("DESTROY") - 1;
	argv[2] = key;
	lens[2] = strlen(key);
	argv[3] = group;
	lens[3] = strlen(group);
	build_request(4, argv, lens);
	return get_number();
}

bool redis_stream::xgroup_setid(const char* key, const char* group,
	const char* id /* = "$" */)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "XGROUP";
	lens[0] = sizeof("XGROUP") - 1;
	argv[1] = "SETID";
	lens[1] = sizeof("SETID") - 1;
	argv[2] = key;
	lens[2] = strlen(key);
	argv[3] = group;
	lens[3] = strlen(group);
	argv[4] = id;
	lens[4] = strlen(id);
	build_request(5, argv, lens);
	return check_status();
}

int redis_stream::xgroup_delconsumer(const char* key, const char* group,
	const char* consumer)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "XGROUP";
	lens[0] = sizeof("XGROUP") - 1;
	argv[1] = "DELCONSUMER";
	lens[1] = sizeof("DELCONSUMER") - 1;
	argv[2] = key;
	lens[2] = strlen(key);
	argv[3] = group;
	lens[3] = strlen(group);
	argv[4] = consumer;
	lens[4] = strlen(consumer);
	build_request(5, argv, lens);
	return get_number();
}

/////////////////////////////////////////////////////////////////////////////

void redis_stream::build(const char* cmd, const char* key, const char* id,
	const std::map<string, string>& fields)
{
	argc_ = fields.size() * 2 + 3;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = id;
	argv_lens_[i] = strlen(id);
	i++;

	for (std::map<string, string>::const_iterator cit = fields.begin();
		cit != fields.end(); ++cit)
	{
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = cit->first.size();
		i++;

		argv_[i] = cit->second.c_str();
		argv_lens_[i] = cit->second.size();
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_stream::build(const char* cmd, const char* key, const char* id,
	const std::vector<string>& names, const std::vector<string>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = names.size() * 2 + 3;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = id;
	argv_lens_[i] = strlen(id);
	i++;

	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		argv_[i] = names[j].c_str();
		argv_lens_[i] = names[j].size();
		i++;

		argv_[i] = values[j].c_str();
		argv_lens_[i] = values[j].size();
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_stream::build(const char* cmd, const char* key, const char* id,
	const std::vector<const char*>& names,
	const std::vector<const char*>& values)
{
	if (names.size() != values.size())
	{
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = names.size() * 2 + 3;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = id;
	argv_lens_[i] = strlen(id);
	i++;

	size_t size = names.size();
	for (size_t j = 0; j < size; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_stream::build(const char* cmd, const char* key, const char* id,
	const char* names[], const size_t names_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	if (argc == 0)
		logger_fatal("argc = 0");

	argc_ = argc * 2 + 3;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = id;
	argv_lens_[i] = strlen(id);
	i++;

	for (size_t j = 0; j < argc; j++)
	{
		argv_[i] = names[j];
		argv_lens_[i] = names_len[j];
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = values_len[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

} // namespace acl
