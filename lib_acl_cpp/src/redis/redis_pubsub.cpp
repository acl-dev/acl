#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_pubsub.hpp"

namespace acl
{

redis_pubsub::redis_pubsub(redis_client* conn /* = NULL */)
: redis_command(conn)
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

	const string& req = conn_->build_request(3, argv, lens);
	return conn_->get_number(req);
}

int redis_pubsub::subscribe(const char* first_channel, ...)
{
	std::vector<string> channels;
	channels.push_back(first_channel);
	va_list ap;
	va_start(ap, first_channel);
	const char* channel;
	while ((channel = va_arg(ap, const char*)) != NULL)
		channels.push_back(channel);
	va_end(ap);

	return subscribe(channels);
}

int redis_pubsub::subscribe(const std::vector<string>& channels)
{
	return subop("SUBSCRIBE", channels);
}

int redis_pubsub::unsubscribe(const char* first_channel, ...)
{
	std::vector<string> channels;
	channels.push_back(first_channel);
	va_list ap;
	va_start(ap, first_channel);
	const char* channel;
	while ((channel = va_arg(ap, const char*)) != NULL)
		channels.push_back(channel);
	va_end(ap);

	return unsubscribe(channels);
}

int redis_pubsub::unsubscribe(const std::vector<string>& channels)
{
	return subop("UNSUBSCRIBE", channels);
}

int redis_pubsub::subop(const char* cmd, const std::vector<string>& channels)
{
	size_t argc = 1 + channels.size();
	dbuf_pool* pool = conn_->get_pool();
	const char** argv = (const char**)
		pool->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t *) pool->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	std::vector<string>::const_iterator cit = channels.begin();
	for (size_t i = 1; cit != channels.end(); ++cit, ++i)
	{
		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
	}

	const string& req = conn_->build_request(argc, argv, lens);
	const redis_result* result = conn_->run(req);
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;
	size_t size = result->get_size();
	if (size != channels.size())
		return -1;

	int nchannels = 0, ret;
	for (size_t i = 1; i < size; i++)
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
	if (strcasecmp(cmd, channel.c_str()) != 0)
		return -1;

	rr = obj->get_child(2);
	if (rr == NULL || rr->get_type() != REDIS_RESULT_INTEGER)
		return -1;
	return obj->get_integer();
}

bool redis_pubsub::get_message(string& channel, string& msg)
{
	const redis_result* result = conn_->run("");
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

} // namespace acl
