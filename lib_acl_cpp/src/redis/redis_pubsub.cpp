#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_pubsub.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_pubsub::redis_pubsub(void)
{
}

redis_pubsub::redis_pubsub(redis_client* conn)
: redis_command(conn)
{
}

redis_pubsub::redis_pubsub(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_pubsub::redis_pubsub(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_pubsub::~redis_pubsub(void)
{
}

int redis_pubsub::publish(const char* channel, const char* msg, size_t len)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "PUBLISH";
	lens[0] = sizeof("PUBLISH") - 1;
	argv[1] = channel;
	lens[1] = strlen(channel);
	argv[2] = msg;
	lens[2] = len;

	hash_slot(channel);
	build_request(3, argv, lens);
	return get_number();
}

int redis_pubsub::subscribe(const char* first_channel, ...)
{
	std::vector<const char*> channels;
	channels.push_back(first_channel);

	va_list ap;
	va_start(ap, first_channel);
	const char* channel;
	while ((channel = va_arg(ap, const char*)) != NULL)
		channels.push_back(channel);
	va_end(ap);

	return subscribe(channels);
}

int redis_pubsub::subscribe(const std::vector<const char*>& channels)
{
	return subop("SUBSCRIBE", channels);
}

int redis_pubsub::subscribe(const std::vector<string>& channels)
{
	return subop("SUBSCRIBE", channels);
}

int redis_pubsub::unsubscribe(const char* first_channel, ...)
{
	std::vector<const char*> channels;
	channels.push_back(first_channel);
	va_list ap;
	va_start(ap, first_channel);
	const char* channel;
	while ((channel = va_arg(ap, const char*)) != NULL)
		channels.push_back(channel);
	va_end(ap);

	return unsubscribe(channels);
}

int redis_pubsub::unsubscribe(const std::vector<const char*>& channels)
{
	return subop("UNSUBSCRIBE", channels);
}

int redis_pubsub::unsubscribe(const std::vector<string>& channels)
{
	return subop("UNSUBSCRIBE", channels);
}

int redis_pubsub::psubscribe(const char* first_pattern, ...)
{
	std::vector<const char*> patterns;
	patterns.push_back(first_pattern);
	va_list ap;
	va_start(ap, first_pattern);
	const char* pattern;
	while ((pattern = va_arg(ap, const char*)) != NULL)
		patterns.push_back(pattern);
	va_end(ap);

	return psubscribe(patterns);
}

int redis_pubsub::psubscribe(const std::vector<const char*>& patterns)
{
	return subop("PSUBSCRIBE", patterns);
}

int redis_pubsub::psubscribe(const std::vector<string>& patterns)
{
	return subop("PSUBSCRIBE", patterns);
}

int redis_pubsub::punsubscribe(const char* first_pattern, ...)
{
	std::vector<const char*> patterns;
	patterns.push_back(first_pattern);
	va_list ap;
	va_start(ap, first_pattern);
	const char* pattern;
	while ((pattern = va_arg(ap, const char*)) != NULL)
		patterns.push_back(pattern);
	va_end(ap);

	return punsubscribe(patterns);
}

int redis_pubsub::punsubscribe(const std::vector<const char*>& patterns)
{
	return subop("PUNSUBSCRIBE", patterns);
}

int redis_pubsub::punsubscribe(const std::vector<string>& patterns)
{
	return subop("PUNSUBSCRIBE", patterns);
}

int redis_pubsub::subop_result(const char* cmd,
	const std::vector<const char*>& channels)
{
	int nchannels = 0, ret;
	size_t i = 0;
	do {
		const redis_result* res = run();
		if (res == NULL || res->get_type() != REDIS_RESULT_ARRAY)
			return -1;

		// clear request, so in next loop we just read the data from
		// redis-server that the data maybe the message to be skipped
		clear_request();

		const redis_result* o = res->get_child(0);
		if (o == NULL || o->get_type() != REDIS_RESULT_STRING)
			return -1;

		string tmp;
		o->argv_to_string(tmp);
		// just skip message in subscribe process
		if (tmp.equal("message", false) || tmp.equal("pmessage", false))
			continue;

		if ((ret = check_channel(res, cmd, channels[i])) < 0)
			return -1;

		if (ret > nchannels)
			nchannels = ret;

		i++;

	} while (i < channels.size());

	return nchannels;
}

int redis_pubsub::subop(const char* cmd, const std::vector<const char*>& channels)
{
	size_t argc = 1 + channels.size();
	const char** argv = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t *) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	std::vector<const char*>::const_iterator cit = channels.begin();
	for (size_t i = 1; cit != channels.end(); ++cit, ++i) {
		argv[i] = *cit;
		lens[i] = strlen(argv[i]);
	}

	if (channels.size() == 1)
		hash_slot(channels[0]);

	build_request(argc, argv, lens);

	return subop_result(cmd, channels);
}

int redis_pubsub::subop_result(const char* cmd,
	const std::vector<string>& channels)
{
	int nchannels = 0, ret;
	size_t i = 0;
	do {
		const redis_result* res = run();
		if (res == NULL || res->get_type() != REDIS_RESULT_ARRAY)
			return -1;

		clear_request();

		const redis_result* o = res->get_child(0);
		if (o == NULL || o->get_type() != REDIS_RESULT_STRING)
			return -1;

		string tmp;
		o->argv_to_string(tmp);
		if (tmp.equal("message", false) || tmp.equal("pmessage", false))
			continue;

		if ((ret = check_channel(res, cmd, channels[i])) < 0)
			return -1;

		if (ret > nchannels)
			nchannels = ret;

		i++;

	} while (i < channels.size());

	return nchannels;
}

int redis_pubsub::subop(const char* cmd, const std::vector<string>& channels)
{
	size_t argc = 1 + channels.size();
	const char** argv = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t *) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	std::vector<string>::const_iterator cit = channels.begin();
	for (size_t i = 1; cit != channels.end(); ++cit, ++i) {
		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
	}

	if (channels.size() == 1)
		hash_slot(channels[0].c_str());

	build_request(argc, argv, lens);
	return subop_result(cmd, channels);
}

int redis_pubsub::check_channel(const redis_result* obj, const char* cmd,
	const char* channel)
{
	if (obj->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	const redis_result* rr = obj->get_child(0);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
		return -1;

	string buf;
	rr->argv_to_string(buf);
	if (strcasecmp(buf.c_str(), cmd) != 0) {
		acl::string tmp;
		obj->to_string(tmp);
		logger_warn("invalid cmd=%s, %s, result=%s",
			buf.c_str(), cmd, tmp.c_str());
		return -1;
	}

	rr = obj->get_child(1);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
		return -1;

	buf.clear();
	rr->argv_to_string(buf);
	if (strcasecmp(buf.c_str(), channel) != 0) {
		acl::string tmp;
		obj->to_string(tmp);
		logger_warn("invalid channel=%s, %s, result=%s",
			buf.c_str(), channel, tmp.c_str());
		return -1;
	}

	rr = obj->get_child(2);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_INTEGER)
		return -1;

	return rr->get_integer();
}

bool redis_pubsub::get_message(string& channel, string& msg,
	string* message_type /* = NULL */, string* pattern /* = NULL */,
	int timeout /* = -1 */)
{
	clear_request();
	int rw_timeout = -1;
	const redis_result* result = run(0, timeout >= 0 ? &timeout : &rw_timeout);
	if (result == NULL)
		return false;
	if (result->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size = result->get_size();
	if (size < 3)
		return false;

	const redis_result* obj = result->get_child(0);
	if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
		return false;

	string tmp;
	obj->argv_to_string(tmp);
	if (message_type)
		*message_type = tmp;
	if (tmp.equal("message", true)) {
		if (pattern)
			pattern->clear();

		obj = result->get_child(1);
		if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
			return false;
		else
			obj->argv_to_string(channel);

		obj = result->get_child(2);
		if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
			return false;
		else
			obj->argv_to_string(msg);

		return true;
	}

	if (!tmp.equal("pmessage", false)) {
		logger_error("unknown message type: %s", tmp.c_str());
		return false;
	}

	if (size < 4) {
		logger_error("invalid size: %d, message type: %s",
			(int) size, tmp.c_str());
		return false;
	}

	if (pattern) {
		obj = result->get_child(1);
		if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
			return false;
		else
			obj->argv_to_string(*pattern);
	}

	obj = result->get_child(2);
	if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
		return false;
	else
		obj->argv_to_string(channel);

	obj = result->get_child(3);
	if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
		return false;
	else
		obj->argv_to_string(msg);

	return true;
}

int redis_pubsub::pubsub_channels(std::vector<string>* channels,
	const char* first_pattern, ...)
{
	std::vector<const char*> patterns;
	if (first_pattern) {
		patterns.push_back(first_pattern);
		va_list ap;
		va_start(ap, first_pattern);
		const char* pattern;
		while ((pattern = va_arg(ap, const char*)) != NULL)
			patterns.push_back(pattern);
		va_end(ap);
	}

	return pubsub_channels(patterns, channels);
}

int redis_pubsub::pubsub_channels(const std::vector<const char*>& patterns,
	std::vector<string>* channels)
{
	build("PUBSUB", "CHANNELS", patterns);
	return get_strings(channels);
}

int redis_pubsub::pubsub_channels(const std::vector<string>& patterns,
	std::vector<string>* channels)
{
	build("PUBSUB", "CHANNELS", patterns);
	return get_strings(channels);
}

int redis_pubsub::pubsub_numsub(std::map<string, int>& out,
	const char* first_channel, ...)
{
	std::vector<const char*> channels;
	if (first_channel != NULL) {
		channels.push_back(first_channel);
		const char* channel;
		va_list ap;
		va_start(ap, first_channel);
		while ((channel = va_arg(ap, const char*)) != NULL)
			channels.push_back(channel);
	}

	return pubsub_numsub(channels, out);
}

int redis_pubsub::pubsub_numsub(const std::vector<const char*>& channels,
	std::map<string, int>& out)
{
	build("PUBSUB", "NUMSUB", channels);
	return pubsub_numsub(out);
}

int redis_pubsub::pubsub_numsub(const std::vector<string>& channels,
	std::map<string, int>& out)
{
	build("PUBSUB", "NUMSUB", channels);
	return pubsub_numsub(out);
}

int redis_pubsub::pubsub_numsub(std::map<string, int>& out)
{
	const redis_result* result = run();
	if (result == NULL)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;

	if (size % 2 != 0)
		return -1;

	string buf(128);
	const redis_result* rr;
	out.clear();

	for (size_t i = 0; i < size;) {
		rr = children[i];
		rr->argv_to_string(buf);
		i++;

		rr = children[i];
		out[buf] = rr->get_integer();
		buf.clear();
	}

	return (int) size / 2;
}

int redis_pubsub::pubsub_numpat()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "PUBSUB";
	lens[0] = sizeof("PUBSUB") - 1;

	argv[1] = "NUMPAT";
	lens[1] = sizeof("NUMPAT") - 1;

	build_request(2, argv, lens);
	return get_number();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
