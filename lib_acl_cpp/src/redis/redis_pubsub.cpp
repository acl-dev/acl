#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_pubsub.hpp"
#endif

namespace acl
{

redis_pubsub::redis_pubsub()
: redis_command(NULL)
{
}

redis_pubsub::redis_pubsub(redis_client* conn)
: redis_command(conn)
{
}

redis_pubsub::redis_pubsub(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_pubsub::~redis_pubsub()
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

int redis_pubsub::subop(const char* cmd, const std::vector<const char*>& channels)
{
	size_t argc = 1 + channels.size();
	const char** argv = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t *) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	std::vector<const char*>::const_iterator cit = channels.begin();
	for (size_t i = 1; cit != channels.end(); ++cit, ++i)
	{
		argv[i] = *cit;
		lens[i] = strlen(argv[i]);
	}

	build_request(argc, argv, lens);
	const redis_result* result = run(channels.size());
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size = channels.size();
	int nchannels = 0, ret;

	for (size_t i = 0; i < size; i++)
	{
		const redis_result* obj = result->get_child(i);
		if (obj == NULL)
			return -1;
		if (( ret = check_channel(obj, argv[0], channels[i])) < 0)
			return -1;
		if (ret > nchannels)
			nchannels = ret;
	}
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
	for (size_t i = 1; cit != channels.end(); ++cit, ++i)
	{
		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
	}

	build_request(argc, argv, lens);
	const redis_result* result = run(channels.size());
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size = channels.size();
	int nchannels = 0, ret;

	for (size_t i = 0; i < size; i++)
	{
		const redis_result* obj = result->get_child(i);
		if (obj == NULL)
			return -1;
		if (( ret = check_channel(obj, argv[0], channels[i])) < 0)
			return -1;
		if (ret > nchannels)
			nchannels = ret;
	}
	return nchannels;
}

int redis_pubsub::check_channel(const redis_result* obj, const char* cmd,
	const string& channel)
{
	if (obj->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	const redis_result* rr = obj->get_child(0);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
		return -1;

	string buf;
	rr->argv_to_string(buf);
	if (strcasecmp(buf.c_str(), cmd) != 0)
		return -1;

	rr = obj->get_child(1);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING)
		return -1;

	buf.clear();
	rr->argv_to_string(buf);
	if (strcasecmp(buf.c_str(), channel.c_str()) != 0)
		return -1;

	rr = obj->get_child(2);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_INTEGER)
		return -1;

	return rr->get_integer();
}

bool redis_pubsub::get_message(string& channel, string& msg)
{
	clear_request();
	const redis_result* result = run();
	if (result == NULL)
		return false;
	if (result->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size = result->get_size();
	if (size != 3)
		return false;

	const redis_result* obj = result->get_child(0);
	if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
		return false;

	string tmp;
	obj->argv_to_string(tmp);
	if (strcasecmp(tmp.c_str(), "message") != 0)
		return false;

	obj = result->get_child(1);
	if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
		return false;
	obj->argv_to_string(channel);

	obj = result->get_child(2);
	if (obj == NULL || obj->get_type() != REDIS_RESULT_STRING)
		return false;
	obj->argv_to_string(msg);
	return true;
}

int redis_pubsub::pubsub_channels(std::vector<string>* channels,
	const char* first_pattern, ...)
{
	std::vector<const char*> patterns;
	if (first_pattern)
	{
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
	if (first_channel != NULL)
	{
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

	for (size_t i = 0; i < size;)
	{
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
