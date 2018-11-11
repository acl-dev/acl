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

int redis_stream::xlen(const char* key)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "XLEN";
	lens[0] = sizeof("XLEN") - 1;
	argv[1] = key;
	lens[1] = strlen(key);

	build_request(2, argv, lens);
	return get_number();
}

//////////////////////////////////////////////////////////////////////////////

void redis_stream::build(const std::map<string, string>& streams,
	size_t count, size_t block, size_t i)
{
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

void redis_stream::xread_build(const std::map<string, string>& streams,
	size_t count, size_t block)
{
	argc_ = 6 + streams.size() * 2;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XREAD";
	argv_lens_[i] = sizeof("XREAD") - 1;
	i++;

	build(streams, count, block, i);
}

void redis_stream::xreadgroup_build(const char* group, const char* consumer,
	const std::map<string, string>& streams, size_t count, size_t block)
{
	argc_ = 9 + streams.size() * 2;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XREADGROUP";
	argv_lens_[i] = sizeof("XREADGROUP") - 1;
	i++;

	argv_[i] = group;
	argv_lens_[i] = strlen(group);
	i++;

	argv_[i] = consumer;
	argv_lens_[i] = strlen(consumer);
	i++;

	build(streams, count, block, i);
}

bool redis_stream::xread(redis_stream_messages& messages,
	const std::map<string, string>& streams,
	size_t count /* = 0 */, size_t block /* = 0 */)
{
	xread_build(streams, count, block);
	return get_results(messages);
}

bool redis_stream::xreadgroup(redis_stream_messages& messages,
	const char* group, const char* consumer,
	const std::map<string, string>& streams,
	size_t count /* = 0 */, size_t block /* = 0 */)
{
	xreadgroup_build(group, consumer, streams, count, block);
	return get_results(messages);
}

bool redis_stream::xrange(redis_stream_messages& messages, const char* key,
	const char* start, const char* end, size_t count /* = 0 */)
{
	return range(messages, "XRANGE", key, start, end, count);
}

bool redis_stream::xrevrange(redis_stream_messages& messages, const char* key,
	const char* start, const char* end, size_t count /* = 0 */)
{
	return range(messages, "XREVRANGE", key, start, end, count);
}

bool redis_stream::range(redis_stream_messages& messages, const char* cmd,
	const char* key, const char* start, const char* end, size_t count)
{
	const char* argv[6];
	size_t lens[6];
	size_t i = 0;

	argv[i] = cmd;
	lens[i] = strlen(cmd);
	i++;

	argv[i] = key;
	lens[i] = strlen(key);
	i++;

	argv[i] = start;
	lens[i] = strlen(start);
	i++;

	argv[i] = end;
	lens[i] = strlen(end);
	i++;

	char count_s[LONG_LEN];
	if (count > 0) {
		argv[i] = "COUNT";
		lens[i] = sizeof("COUNT") - 1;
		i++;

		safe_snprintf(count_s, sizeof(count_s), "%lu",
			(unsigned long) count);
		argv[i] = count_s;
		lens[i] = strlen(count_s);
		i++;
	}

	build_request(i, argv, lens);

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	messages.key = key;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0) {
		return true;
	}

	for (i = 0; i < size; i++) {
		const redis_result* child = children[i];
		if (child->get_type() != REDIS_RESULT_ARRAY) {
			continue;
		}

		redis_stream_message message;
		bool ret = get_one_message(*child, message);
		if (ret && !message.fields.empty()) {
			messages.messages[message.id] = message;
		}
	}
	return true;
}

bool redis_stream::get_results(redis_stream_messages& messages)
{
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
		get_messages(*child, messages);
	}

	return true;
}

bool redis_stream::get_messages(const redis_result& rr,
	redis_stream_messages& messages)
{
	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (size != 2) {
		return false;
	}

	const redis_result* child = children[0];
	if (child->get_type() != REDIS_RESULT_STRING) {
		return false;
	}
	child->argv_to_string(messages.key);

	child = children[1];
	if (child->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	children = child->get_children(&size);
	if (size == 0 || children == NULL) {
		return false;
	}

	for (size_t i = 0; i < size; i++) {
		child = children[i];
		if (child->get_type() != REDIS_RESULT_ARRAY) {
			continue;
		}

		redis_stream_message message;
		bool ret = get_one_message(*child, message);
		if (ret && !message.fields.empty()) {
			messages.messages[message.id] = message;
		}
	}
	return true;
}

bool redis_stream::get_one_message(const redis_result& rr,
	redis_stream_message& message)
{
	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (size == 0 || children == NULL) {
		return false;
	}
	if (size != 2) {
		return false;
	}

	const redis_result* child = children[0];
	if (child->get_type() != REDIS_RESULT_STRING) {
		return false;
	}

	child->argv_to_string(message.id);

	child = children[1];
	children = child->get_children(&size);
	if (size == 0 || children == NULL) {
		return false;
	}

	for (size_t i = 0; i < size; i++) {
		child = children[i];
		if (child->get_type() != REDIS_RESULT_ARRAY) {
			continue;
		}

		redis_stream_field field;
		if (!get_one_field(*child, field)) {
			continue;
		}

		message.fields.push_back(field);
	}

	return true;
}

bool redis_stream::get_one_field(const redis_result& rr,
	redis_stream_field& field)
{
	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (children == NULL || size != 0) {
		return false;
	}

	const redis_result* child = children[0];
	child->argv_to_string(field.name);
	child = children[1];
	child->argv_to_string(field.value);
	return true;
}

int redis_stream::xack(const char* key, const char* group, const char* id)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "XACK";
	lens[0] = sizeof("XACK") - 1;
	argv[1] = group;
	lens[1] = strlen(group);
	argv[2] = key;
	lens[2] = strlen(key);
	argv[3] = id;
	lens[3] = strlen(id);
	build_request(4, argv, lens);
	return get_number();
}

int redis_stream::xack(const char* key, const char* group,
	const std::vector<string>& ids)
{
	if (ids.empty()) {
		logger_error("ids empty");
		return -1;
	}

	argc_ = 3 + ids.size();
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XACK";
	argv_lens_[i] = sizeof("XACK") - 1;
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = group;
	argv_lens_[i] = strlen(group);
	i++;

	for (std::vector<string>::const_iterator cit = ids.begin();
		cit != ids.end(); ++cit) {

		argv_[i] = (*cit).c_str();
		argv_lens_[i] = (*cit).size();
		i++;
	}

	build_request(i, argv_, argv_lens_);
	return get_number();
}

int redis_stream::xack(const char* key, const char* group,
	const std::vector<const char*>& ids)
{
	if (ids.empty()) {
		logger_error("ids empty");
		return -1;
	}

	argc_ = 3 + ids.size();
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XACK";
	argv_lens_[i] = sizeof("XACK") - 1;
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = group;
	argv_lens_[i] = strlen(group);
	i++;

	for (std::vector<const char*>::const_iterator cit = ids.begin();
		cit != ids.end(); ++cit) {

		argv_[i] = *cit;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(i, argv_, argv_lens_);
	return get_number();
}

int redis_stream::xack(const char* key, const char* group,
	const std::list<string>& ids, size_t size)
{
	if (ids.empty() || size == 0) {
		logger_error("ids empty or size 0");
		return -1;
	}

	argc_ = 3 + size;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XACK";
	argv_lens_[i] = sizeof("XACK") - 1;
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = group;
	argv_lens_[i] = strlen(group);
	i++;

	for (std::list<string>::const_iterator cit = ids.begin();
		cit != ids.end(); ++cit) {

		argv_[i] = (*cit).c_str();
		argv_lens_[i] = (*cit).size();
		i++;
	}

	build_request(i, argv_, argv_lens_);
	return get_number();
}

int redis_stream::xack(const char* key, const char* group,
	const std::list<const char*>& ids, size_t size)
{
	if (ids.empty() || size == 0) {
		logger_error("ids empty or size 0");
		return -1;
	}

	argc_ = 3 + size;
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = "XACK";
	argv_lens_[i] = sizeof("XACK") - 1;
	i++;

	argv_[i] = key;
	argv_lens_[i] = strlen(key);
	i++;

	argv_[i] = group;
	argv_lens_[i] = strlen(group);
	i++;

	for (std::list<const char*>::const_iterator cit = ids.begin();
		cit != ids.end(); ++cit) {

		argv_[i] = *cit;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(i, argv_, argv_lens_);
	return get_number();
}

//////////////////////////////////////////////////////////////////////////////

#if 0

bool redis_stream::xpending(std::map<string, redis_pending_message>& result,
	const char* key, const char* group, const char* start_id /* = "-" */,
	const char* end_id /* = "+" */, size_t count /* = 1 */,
	const char* consumer /* = NULL */)
{
	const char* argv[7];
	size_t lens[7];
	size_t i = 0;

	argv[i] = "XPENDING";
	lens[i] = sizeof("XPENDING") - 1;
	i++;

	argv[i] = key;
	lens[i] = strlen(key);
	i++;

	argv[i] = group;
	lens[i] = strlen(group);
	i++;

	char count_s[LONG_LEN];

	if (start_id && *start_id) {
		argv[i] = start_id;
		lens[i] = strlen(start_id);
		i++;

		if (end_id == NULL || *end_id == 0) {
			logger_error("end_id null");
			return false;
		}
		argv[i] = end_id;
		lens[i] = strlen(end_id);
		i++;

		safe_snprintf(count_s, sizeof(count_s), "%lu",
			(unsigned long) count);
		argv[i] = count_s;
		lens[i] = strlen(count_s);
		i++;

		if (consumer && *consumer) {
			argv[i] = consumer;
			lens[i] = strlen(consumer);
			i++;
		}
	} else if (end_id || consumer) {
		logger_error("start_id null");
		return false;
	}

	build_request(i, argv, lens);

	const redis_result* rr = run();
	if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL || size == 0) {
		return true;
	}

	for (size_t i = 0; i < size; i++) {
		const redis_result* child = children[i];

		redis_pending_message message;
		if (get_pending_messsage(*child, message)) {
			result[message.id] = message;
		}
	}

	return true;
}

bool redis_stream::get_pending_message(const redis_result& rr,
	redis_pending_message& message)
{
	if (rr.get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (children == NULL || size == 0) {
		return false;
	}

	return true;
}

#endif

//////////////////////////////////////////////////////////////////////////////

int redis_stream::xdel(const char* key, const std::vector<string>& ids)
{
	redis_command::build("XDEL", key, ids);
	return get_number();
}

int redis_stream::xdel(const char* key, const std::vector<const char*>& ids)
{
	redis_command::build("XDEL", key, ids);
	return get_number();
}

int redis_stream::xtrim(const char* key, size_t maxlen, bool tilde)
{
	const char* argv[5];
	size_t lens[5];
	size_t i = 0;

	argv[i] = "XTRIM";
	lens[i] = sizeof("XTRIM") - 1;
	i++;

	argv[i] = key;
	lens[i] = strlen(key);
	i++;

	argv[i] = "MAXLEN";
	lens[i] = sizeof("MAXLEN") - 1;
	i++;

	if (tilde) {
		argv[i] = "~";
		lens[i] = 1;
		i++;
	}

	char buf[LONG_LEN];
	safe_snprintf(buf, sizeof(buf), "%lu", (unsigned long) maxlen);
	argv[i] = buf;
	lens[i] = strlen(buf);
	i++;

	build_request(i, argv, lens);
	return get_number();
}

//////////////////////////////////////////////////////////////////////////////

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
		cit != fields.end(); ++cit) {

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
	if (names.size() != values.size()) {
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
	for (size_t j = 0; j < size; j++) {
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
	if (names.size() != values.size()) {
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
	for (size_t j = 0; j < size; j++) {
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
	if (argc == 0) {
		logger_fatal("argc = 0");
	}

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

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = names_len[j];
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = values_len[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

//////////////////////////////////////////////////////////////////////////////

bool redis_stream::xinfo_help(std::vector<string>& result)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "XINFO";
	lens[0] = sizeof("XINFO") - 1;
	argv[1] = "HELP";
	lens[1] = sizeof("HELP") - 1;

	build_request(2, argv, lens);
	return get_strings(result) >= 0 ? true : false;
}

#define IS_INTEGER(x)	((x) == REDIS_RESULT_INTEGER)
#define IS_STRING(x)	((x) == REDIS_RESULT_STRING)
#define IS_ARRAY(x)	((x) == REDIS_RESULT_ARRAY)

#define EQ(x, y) ((x).equal((y), false))

bool redis_stream::xinfo_consumers(const char* key, const char* group,
	std::map<string, redis_xinfo_consumer>& result)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "XINFO";
	lens[0] = sizeof("XINFO") - 1;
	argv[1] = "CONSUMERS";
	lens[1] = sizeof("CONSUMERS") - 1;
	argv[2] = key;
	lens[2] = strlen(key);
	argv[3] = group;
	lens[3] = strlen(group);

	build_request(4, argv, lens);
	const redis_result* rr = run();
	if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL || size == 0) {
		return true;
	}

	for (size_t i = 0; i < size; i++) {
		const redis_result* child = children[i];

		redis_xinfo_consumer consumer;
		if (get_one_consumer(*child, consumer)) {
			result[consumer.name] = consumer;
		}
	}

	return true;
}

bool redis_stream::get_one_consumer(const redis_result& rr,
	redis_xinfo_consumer& consumer)
{
	if (rr.get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (children == NULL || size == 0 || size % 2 != 0) {
		return false;
	}

	for (size_t i = 0; i < size;) {
		const redis_result* first = children[i++];
		if (first->get_type() != REDIS_RESULT_STRING) {
			i++;
			continue;
		}

		string value;
		size_t n = 0;

		const redis_result* second = children[i++];
		redis_result_t type = second->get_type();
		if (type == REDIS_RESULT_STRING) {
			second->argv_to_string(value);
			if (value.empty()) {
				continue;
			}
		} else if (type == REDIS_RESULT_INTEGER) {
			bool ok;
			n = (size_t) second->get_integer(&ok);
			if (!ok) {
				continue;
			}
		} else {
			continue;
		}

		string name;
		first->argv_to_string(name);

		if (IS_STRING(type) && EQ(name, "name")) {
			consumer.name = value;
		} else if (IS_INTEGER(type) && EQ(name, "pending")) {
			consumer.pending = n;
		} else if (IS_INTEGER(type) && EQ(name, "idle")) {
			consumer.idle = n;
		}
	}

	return !consumer.name.empty();
}

bool redis_stream::xinfo_groups(const char* key,
	std::map<string, redis_xinfo_group>& result)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "XINFO";
	lens[0] = sizeof("XINFO") - 1;
	argv[1] = "GROUPS";
	lens[1] = sizeof("GROUPS") - 1;
	argv[2] = key;
	lens[2] = strlen(key);

	build_request(3, argv, lens);
	const redis_result* rr = run();
	if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL || size == 0) {
		return true;
	}

	for (size_t i = 0; i < size; i++) {
		const redis_result* child = children[i];

		redis_xinfo_group group;
		if (get_one_group(*child, group)) {
			result[group.name] = group;
		}
	}

	return true;
}

bool redis_stream::get_one_group(const redis_result& rr,
	redis_xinfo_group& group)
{
	if (rr.get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (children == NULL || size == 0 || size % 2 != 0) {
		return false;
	}

	for (size_t i = 0; i < size;) {
		const redis_result* first = children[i++];
		if (first->get_type() != REDIS_RESULT_STRING) {
			i++;
			continue;
		}

		string value;
		size_t n = 0;

		const redis_result* second = children[i++];
		redis_result_t type = second->get_type();
		if (type == REDIS_RESULT_STRING) {
			second->argv_to_string(value);
			if (value.empty()) {
				continue;
			}
		} else if (type == REDIS_RESULT_INTEGER) {
			bool ok;
			n = (size_t) second->get_integer(&ok);
			if (!ok) {
				continue;
			}
		} else {
			continue;
		}

		string name;
		first->argv_to_string(name);

		if (IS_STRING(type) && EQ(name, "name")) {
			group.name = value;
		} else if (IS_INTEGER(type) && EQ(name, "consumers")) {
			group.consumers = n;
		} else if (IS_INTEGER(type) && EQ(name, "pending")) {
			group.pending = n;
		} else if (IS_STRING(type) && EQ(name, "last-delivered-id")) {
			group.last_delivered_id = value;
		}
	}

	return !group.name.empty();
}

bool redis_stream::xinfo_stream(const char* key, redis_stream_info& info)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "XINFO";
	lens[0] = sizeof("XINFO") - 1;
	argv[1] = "STREAM";
	lens[1] = sizeof("STREAM") - 1;
	argv[2] = key;
	lens[2] = strlen(key);

	build_request(3, argv, lens);
	const redis_result* rr = run();
	if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL || size == 0) {
		return false;
	}

	if (size % 2 != 0) {
		return false;
	}

	for (size_t i = 0; i < size;) {
		const redis_result* first = children[i++];
		if (first->get_type() != REDIS_RESULT_STRING) {
			i++;
			continue;
		}

		string name, value;
		size_t n = 0;

		const redis_result* second = children[i++];
		redis_result_t type = second->get_type();
		if (type == REDIS_RESULT_STRING) {
			second->argv_to_string(value);
			if (value.empty()) {
				continue;
			}
		} else if (type == REDIS_RESULT_INTEGER) {
			bool ok;
			n = (size_t) second->get_integer(&ok);
			if (!ok) {
				continue;
			}
		} else if (type != REDIS_RESULT_ARRAY) {
			continue;
		}

		first->argv_to_string(name);

		if (IS_INTEGER(type) && EQ(name, "length")) {
			info.length = n;
		} else if (IS_INTEGER(type) && EQ(name, "radix-tree-keys")) {
			info.radix_tree_keys = n;
		} else if (IS_INTEGER(type) && EQ(name, "radix-tree-nodes")) {
			info.radix_tree_nodes = n;
		} else if (IS_INTEGER(type) && EQ(name, "groups")) {
			info.groups = n;
		} else if (IS_STRING(type) && EQ(name, "last-generated-id")) {
			info.last_generated_id = value;
		} else if (IS_ARRAY(type) && EQ(name, "first-entry")) {
			(void) get_one_message(*second, info.first_entry);
		} else if (IS_ARRAY(type) && EQ(name, "last-entry")) {
			(void) get_one_message(*second, info.last_entry);
		}
	}

	return true;
}

} // namespace acl
