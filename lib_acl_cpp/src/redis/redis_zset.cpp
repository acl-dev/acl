#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_zset.hpp"
#endif

namespace acl
{

#define BUFLEN	32
#define INTLEN	11

redis_zset::redis_zset()
: redis_command(NULL)
{
}

redis_zset::redis_zset(redis_client* conn)
: redis_command(conn)
{
}

redis_zset::redis_zset(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_zset::~redis_zset()
{
}

int redis_zset::zadd(const char* key, const std::map<string, double>& members)
{
	size_t argc = 2 + members.size() * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t *lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char* buf;
	size_t i = 2;
	std::map<string, double>::const_iterator cit = members.begin();
	for (; cit != members.end(); ++cit)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", cit->second);

		argv[i] = buf;
		lens[i] = strlen(buf);
		i++;

		argv[i] = cit->first.c_str();
		lens[i] = strlen(argv[i]);
		i++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zadd(const char* key,
	const std::vector<std::pair<string, double> >&members)
{
	size_t argc = 2 + members.size() * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char* buf;
	size_t i = 2;
	std::vector<std::pair<string, double> >::const_iterator cit;
	for (cit = members.begin(); cit != members.end(); ++cit)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", (*cit).second);
		argv[i] = buf;
		lens[i] = strlen(buf);
		i++;

		argv[i] = (*cit).first.c_str();
		lens[i] = (*cit).first.length();
		i++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zadd(const char* key,
	const std::vector<std::pair<const char*, double> >&members)
{
	size_t argc = 2 + members.size() * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char* buf;
	size_t i = 2;
	std::vector<std::pair<const char*, double> >::const_iterator cit;

	for (cit = members.begin(); cit != members.end(); ++cit)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", (*cit).second);
		argv[i] = buf;
		lens[i] = strlen(buf);
		i++;

		argv[i] = (*cit).first;
		lens[i] = strlen(argv[i]);
		i++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zadd(const char* key, const std::vector<string>& members,
	const std::vector<double>& scores)
{
	size_t size = scores.size();
	if (size != members.size())
		return -1;

	size_t argc = 2 + scores.size() * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	size_t j = 2;
	char* buf;

	for (size_t i = 0; i < size; i++)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", scores[i]);
		argv[j] = buf;
		lens[j] = strlen(buf);
		j++;

		argv[j] = members[i].c_str();
		lens[j] = strlen(argv[j]);
		j++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zadd(const char* key, const std::vector<const char*>& members,
	const std::vector<double>& scores)
{
	size_t size = scores.size();
	if (size != members.size())
		return -1;

	size_t argc = 2 + scores.size() * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	size_t j = 2;
	char* buf;

	for (size_t i = 0; i < size; i++)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", scores[i]);
		argv[j] = buf;
		lens[j] = strlen(buf);
		j++;

		argv[j] = members[i];
		lens[j] = strlen(argv[j]);
		j++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zadd(const char* key, const char* members[], double scores[],
	size_t size)
{
	size_t argc = 2 + size * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	size_t j = 2;
	char* buf;

	for (size_t i = 0; i < size; i++)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", scores[i]);
		argv[j] = buf;
		lens[j] = strlen(buf);
		j++;

		argv[j] = members[i];
		lens[j] = strlen(argv[j]);
		j++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zadd(const char* key, const char* members[],
	size_t members_len[], double scores[], size_t size)
{
	size_t argc = 2 + size * 2;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ZADD";
	lens[0] = sizeof("ZADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	size_t j = 2;
	char* buf;
	int len;

	for (size_t i = 0; i < size; i++)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		len = safe_snprintf(buf, BUFLEN, "%.8f", scores[i]);
		argv[j] = buf;
		lens[j] = len;
		j++;

		argv[j] = members[i];
		lens[j] = members_len[i];
		j++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zcard(const char* key)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "ZCARD";
	lens[0] = sizeof("ZCARD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	hash_slot(key);
	build_request(2, argv, lens);
	return get_number();
}

int redis_zset::zcount(const char* key, double min, double max)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "ZCOUNT";
	lens[0] = sizeof("ZCOUNT") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char min_buf[BUFLEN], max_buf[BUFLEN];
	safe_snprintf(min_buf, sizeof(min_buf), "%.8f", min);
	safe_snprintf(max_buf, sizeof(max_buf), "%.8f", max);

	argv[2] = min_buf;
	lens[2] = strlen(min_buf);

	argv[3] = max_buf;
	lens[3] = strlen(max_buf);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_number();
}

bool redis_zset::zincrby(const char* key, double inc,
	const char* member, double* result /* = NULL */)
{
	return zincrby(key, inc, member, strlen(member), result);
}

bool redis_zset::zincrby(const char* key, double inc,
	const char* member, size_t len, double* result /* = NULL */)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "ZINCRBY";
	lens[0] = sizeof("ZINCRBY") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char score[BUFLEN];
	safe_snprintf(score, sizeof(score), "%.8f", inc);
	argv[2] = score;
	lens[2] = strlen(score);

	argv[3] = member;
	lens[3] = len;

	hash_slot(key);
	build_request(4, argv, lens);
	int ret = get_string(score, sizeof(score));
	if (ret < 0)
		return false;
	if (result)
		*result = atof(score);
	return true;
}

int redis_zset::zrange_get(const char* cmd, const char* key, int start,
	int stop, std::vector<string>* result)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = key;
	lens[1] = strlen(key);

	char start_s[INTLEN], stop_s[INTLEN];
	safe_snprintf(start_s, sizeof(start_s), "%d", start);
	safe_snprintf(stop_s, sizeof(stop_s), "%d", stop);

	argv[2] = start_s;
	lens[2] = strlen(start_s);

	argv[3] = stop_s;
	lens[3] = strlen(stop_s);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_strings(result);
}

int redis_zset::zrange(const char* key, int start,
	int stop, std::vector<string>* result)
{
	return zrange_get("ZRANGE", key, start, stop, result);
}

int redis_zset::zrange_get_with_scores(const char* cmd, const char* key,
	int start, int stop, std::vector<std::pair<string, double> >& out)
{
	out.clear();

	const char* argv[5];
	size_t lens[5];

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = key;
	lens[1] = strlen(key);

	char start_s[INTLEN], stop_s[INTLEN];
	safe_snprintf(start_s, sizeof(start_s), "%d", start);
	safe_snprintf(stop_s, sizeof(stop_s), "%d", stop);

	argv[2] = start_s;
	lens[2] = strlen(start_s);

	argv[3] = stop_s;
	lens[3] = strlen(stop_s);

	argv[4] = "WITHSCORES";
	lens[4] = sizeof("WITHSCORES") - 1;

	hash_slot(key);
	build_request(5, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;
	if (size % 2 != 0)
		return -1;

	size /= 2;
	out.reserve(size);
	double score;
	const redis_result* child;
	string buf(128);

	for (size_t i = 0; i < size; i++)
	{
		child = children[2 * i + 1];
		if (child == NULL)
			continue;

		child->argv_to_string(buf);
		score = atof(buf.c_str());
		buf.clear();

		child = children[2 * i];
		if (child == NULL)
			continue;

		child->argv_to_string(buf);
		out.push_back(std::make_pair(buf, score));
		buf.clear();
	}

	return (int) size;
}

int redis_zset::zrange_with_scores(const char* key, int start, int stop,
	std::vector<std::pair<string, double> >& out)
{
	return zrange_get_with_scores("ZRANGE", key, start, stop, out);
}

int redis_zset::zrangebyscore_get(const char* cmd, const char* key,
	const char* min, const char* max, std::vector<string>* out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	const char* argv[8];
	size_t lens[8];
	size_t argc = 4;

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = min;
	lens[2] = strlen(min);

	argv[3] = max;
	lens[3] = strlen(max);

	char offset_s[INTLEN], count_s[INTLEN];
	if (offset && count)
	{
		safe_snprintf(offset_s, sizeof(offset_s), "%d", *offset);
		safe_snprintf(count_s, sizeof(count_s), "%d", *count);

		argv[4] = "LIMIT";
		lens[4] = sizeof("LIMIT") - 1;

		argv[5] = offset_s;
		lens[5] = strlen(offset_s);

		argv[6] = count_s;
		lens[6] = strlen(count_s);

		argc += 3;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_strings(out);
}

int redis_zset::zrangebyscore(const char* key, const char* min,
	const char* max, std::vector<string>* out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	return zrangebyscore_get("ZRANGEBYSCORE", key, min, max,
		out, offset, count);
}

int redis_zset::zrangebyscore(const char* key, double min,
	double max, std::vector<string>* out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	char min_s[BUFLEN], max_s[BUFLEN];
	safe_snprintf(min_s, sizeof(min_s), "%.8f", min);
	safe_snprintf(max_s, sizeof(max_s), "%.8f", max);

	return zrangebyscore(key, min_s, max_s, out, offset, count);
}

int redis_zset::zrangebyscore_get_with_scores(const char* cmd,
	const char* key, const char* min, const char* max,
	std::vector<std::pair<string, double> >& out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	const char* argv[8];
	size_t lens[8];
	size_t argc = 5;

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = min;
	lens[2] = strlen(min);

	argv[3] = max;
	lens[3] = strlen(max);

	argv[4] = "WITHSCORES";
	lens[4] = sizeof("WITHSCORES") - 1;

	char offset_s[INTLEN], count_s[INTLEN];
	if (offset && count)
	{
		safe_snprintf(offset_s, sizeof(offset_s), "%d", *offset);
		safe_snprintf(count_s, sizeof(count_s), "%d", *count);

		argv[5] = "LIMIT";
		lens[5] = sizeof("LIMIT") - 1;

		argv[6] = offset_s;
		lens[6] = strlen(offset_s);

		argv[7] = count_s;
		lens[7] = strlen(count_s);

		argc += 3;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return -1;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return 0;
	if (size % 2 != 0)
		return -1;

	size /= 2;
	out.reserve(size);
	double score;
	const redis_result* child;
	string buf(128);
	out.clear();

	for (size_t i = 0; i < size; i++)
	{
		child = children[2 * i + 1];
		if (child == NULL)
			continue;

		child->argv_to_string(buf);
		score = atof(buf.c_str());
		buf.clear();

		child = children[2 * i];
		if (child == NULL)
			continue;
		child->argv_to_string(buf);

		out.push_back(std::make_pair(buf, score));
		buf.clear();
	}

	return (int) size;
}

int redis_zset::zrangebyscore_with_scores(const char* key, const char* min,
	const char* max, std::vector<std::pair<string, double> >& out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	return zrangebyscore_get_with_scores("ZRANGEBYSCORE", key, min,
		max, out, offset, count);
}

int redis_zset::zrangebyscore_with_scores(const char* key, double min,
	double max, std::vector<std::pair<string, double> >& out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	char min_s[BUFLEN], max_s[BUFLEN];
	safe_snprintf(min_s, sizeof(min_s), "%.8f", min);
	safe_snprintf(max_s, sizeof(max_s), "%.8f", max);

	return zrangebyscore_with_scores(key, min_s, max_s, out, offset, count);
}

int redis_zset::zrank(const char* key, const char* member, size_t len)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "ZRANK";
	lens[0] = sizeof("ZRANK") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = member;
	lens[2] = len;

	hash_slot(key);
	build_request(3, argv, lens);
	return get_number();
}

int redis_zset::zrank(const char* key, const char* member)
{
	return zrank(key, member, strlen(member));
}

int redis_zset::zrem(const char* key, const char* first_member, ...)
{
	std::vector<const char*> members;
	members.push_back(first_member);
	va_list ap;
	va_start(ap, first_member);
	const char* member;
	while ((member = va_arg(ap, const char*)) != NULL)
		members.push_back(member);

	return zrem(key, members);
}

int redis_zset::zrem(const char* key, const std::vector<string>& members)
{
	hash_slot(key);
	build("ZREM", key, members);
	return get_number();
}

int redis_zset::zrem(const char* key, const std::vector<const char*>& members)
{
	hash_slot(key);
	build("ZREM", key, members);
	return get_number();
}

int redis_zset::zrem(const char* key, const char* members[],
	const size_t lens[], size_t argc)
{
	hash_slot(key);
	build("ZREM", key, members, lens, argc);
	return get_number();
}

int redis_zset::zremrangebyrank(const char* key, int start, int stop)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "ZREMRANGEBYRANK";
	lens[0] = sizeof("ZREMRANGEBYRANK") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char start_s[INTLEN], stop_s[INTLEN];
	safe_snprintf(start_s, sizeof(start_s), "%d", start);
	safe_snprintf(stop_s, sizeof(stop_s), "%d", stop);

	argv[2] = start_s;
	lens[2] = strlen(start_s);

	argv[3] = stop_s;
	lens[3] = strlen(stop_s);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_number();
}

int redis_zset::zremrangebyscore(const char* key, double min, double max)
{
	char min_s[BUFLEN], max_s[BUFLEN];
	safe_snprintf(min_s, sizeof(min_s), "%.8f", min);
	safe_snprintf(max_s, sizeof(max_s), "%.8f", max);

	return zremrangebyscore(key, min_s, max_s);
}

int redis_zset::zremrangebyscore(const char* key, const char* min,
	const char* max)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "ZREMRANGEBYSCORE";
	lens[0] = sizeof("ZREMRANGEBYSCORE") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = min;
	lens[2] = strlen(min);

	argv[3] = max;
	lens[3] = strlen(max);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_number();
}

int redis_zset::zrevrange(const char* key, int start, int stop,
	std::vector<string>* result)
{
	return zrange_get("ZREVRANGE", key, start, stop, result);
}

int redis_zset::zrevrange_with_scores(const char* key, int start, int stop,
	std::vector<std::pair<string, double> >& out)
{
	return zrange_get_with_scores("ZREVRANGE", key, start, stop, out);
}

int redis_zset::zrevrangebyscore_with_scores(const char* key, const char* min,
	const char* max, std::vector<std::pair<string, double> >& out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	return zrangebyscore_get_with_scores("ZREVRANGEBYSCORE", key, min,
		max, out, offset, count);
}

int redis_zset::zrevrangebyscore_with_scores(const char* key, double min,
	double max, std::vector<std::pair<string, double> >& out,
	const int* offset /* = NULL */, const int* count /* = NULL */)
{
	char min_s[BUFLEN], max_s[BUFLEN];
	safe_snprintf(min_s, sizeof(min_s), "%.8f", min);
	safe_snprintf(max_s, sizeof(max_s), "%.8f", max);

	return zrangebyscore_with_scores(key, min_s, max_s, out, offset, count);
}

int redis_zset::zrevrank(const char* key, const char* member, size_t len)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "ZREVRANK";
	lens[0] = sizeof("ZREVRANK") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = member;
	lens[2] = len;

	hash_slot(key);
	build_request(3, argv, lens);
	return get_number();
}

int redis_zset::zrevrank(const char* key, const char* member)
{
	return zrevrank(key, member, strlen(member));
}

bool redis_zset::zscore(const char* key, const char* member, size_t len,
	double& result)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "ZSCORE";
	lens[0] = sizeof("ZSCORE") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = member;
	lens[2] = len;

	hash_slot(key);
	build_request(3, argv, lens);

	char buf[BUFLEN];
	int ret = get_string(buf, sizeof(buf));
	if (ret <= 0)
		return false;
	result = atof(buf);
	return true;
}

bool redis_zset::zscore(const char* key, const char* member, double& result)
{
	return zscore(key, member, strlen(member), result);
}

int redis_zset::zstore(const char* cmd, const char* dst,
	const std::map<string, double>& keys, const char* aggregate)
{
	size_t num = keys.size();
	if (num == 0)
		return -1;

	size_t argc = num * 2 + 6;

	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = dst;
	lens[1] = strlen(dst);

	char num_s[BUFLEN];
	safe_snprintf(num_s, sizeof(num_s), "%d", (int) num);
	argv[2] = num_s;
	lens[2] = strlen(num_s);

	size_t i = 3;
	std::map<string, double>::const_iterator cit = keys.begin();
	for (; cit != keys.end(); ++cit, i++)
	{
		argv[i] = cit->first.c_str();
		lens[i] = strlen(argv[i]);
		i++;
	}

	argv[i] = "WEIGHTS";
	lens[i] = sizeof("WEIGHTS") - 1;

	char* buf;
	for (cit = keys.begin(); cit != keys.end(); ++cit)
	{
		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", cit->second);

		argv[i] = buf;
		lens[i] = strlen(buf);
		i++;
	}

	argv[i] = "AGGREGATE";
	lens[i] = sizeof("AGGREGATE") - 1;
	i++;

	if (aggregate == NULL || *aggregate == 0)
		aggregate = "SUM";
	argv[i] = aggregate;
	lens[i] = strlen(aggregate);
	i++;

	acl_assert(i == argc);

	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zunionstore(const char* dst,
	const std::map<string, double>& keys, const char* aggregate /* = "SUM" */)
{
	return zstore("ZUNIONSTORE", dst, keys, aggregate);
}

int redis_zset::zinterstore(const char* dst,
	const std::map<string, double>& keys, const char* aggregate /* = "SUM" */)
{
	return zstore("ZINTERSTORE", dst, keys, aggregate);
}

int redis_zset::zstore(const char* cmd, const char* dst,
	const std::vector<string>& keys, const std::vector<double>* weights,
	const char* aggregate)
{
	size_t argc = 3 + keys.size();

	if (weights != NULL)
	{
		if (weights->size() != keys.size())
			return -1;
		argc += weights->size() + 1;
	}
	if (aggregate != NULL && *aggregate != 0)
		argc += 2;

	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	argv[1] = dst;
	lens[1] = strlen(dst);

	char num_s[INTLEN];
	safe_snprintf(num_s, sizeof(num_s), "%d", (int) keys.size());
	argv[2] = num_s;
	lens[2] = strlen(num_s);

	size_t i = 3;
	std::vector<string>::const_iterator cit = keys.begin();
	for (; cit != keys.end(); ++cit)
	{
		argv[i] = (*cit).c_str();
		lens[i] = strlen(argv[i]);
		i++;
	}

	if (weights != NULL)
	{
		argv[i] = "WEIGHTS";
		lens[i] = sizeof("WEIGHTS") - 1;
		i++;

		char* score;
		std::vector<double>::const_iterator cit2 = weights->begin();
		for (; cit2 != weights->end(); ++cit2)
		{
			score = (char*) dbuf_->dbuf_alloc(BUFLEN);
			safe_snprintf(score, BUFLEN, "%.8f", *cit2);
			argv[i] = score;
			lens[i] = strlen(score);
			i++;
		}
	}

	if (aggregate != NULL)
	{
		argv[i] = "AGGREGATE";
		lens[i] = sizeof("AGGREGATE") - 1;
		i++;

		argv[i] = aggregate;
		lens[i] = strlen(aggregate);
		i++;
	}

	acl_assert(i == argc);

	build_request(argc, argv, lens);
	return get_number();
}

int redis_zset::zunionstore(const char* dst, const std::vector<string>& keys,
	const std::vector<double>* weights /* = NULL */,
	const char* aggregate /* = "SUM" */)
{
	return zstore("ZUNIONSTORE", dst, keys, weights, aggregate);
}

int redis_zset::zinterstore(const char* dst, const std::vector<string>& keys,
	const std::vector<double>* weights /* = NULL */,
	const char* aggregate /* = "SUM" */)
{
	return zstore("ZINTERSTORE", dst, keys, weights, aggregate);
}

int redis_zset::zrangebylex(const char* key, const char* min, const char* max,
	std::vector<string>* out, const int* offset /* = NULL */,
	const int* count /* = NULL */)
{
	const char* argv[7];
	size_t lens[7];
	size_t argc = 4;

	argv[0] = "ZRANGEBYLEX";
	lens[0] = sizeof("ZRANGEBYLEX") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = min;
	lens[2] = strlen(min);

	argv[3] = max;
	lens[3] = strlen(max);

	if (offset != NULL && count != NULL)
	{
		argv[4] = min;
		lens[4] = strlen(min);
		argc++;

		argv[5] = max;
		lens[5] = strlen(max);
		argc++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_strings(out);
}

int redis_zset::zlexcount(const char* key, const char* min, const char* max)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "ZLEXCOUNT";
	lens[0] = sizeof("ZLEXCOUNT") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = min;
	lens[2] = strlen(min);

	argv[3] = max;
	lens[3] = strlen(max);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_number();
}

int redis_zset::zremrangebylex(const char* key, const char* min, const char* max)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "ZREMRANGEBYLEX";
	lens[0] = sizeof("ZREMRANGEBYLEX") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	argv[2] = min;
	lens[2] = strlen(min);
	
	argv[3] = max;
	lens[3] = strlen(max);

	hash_slot(key);
	build_request(4, argv, lens);
	return get_number();
}

int redis_zset::zscan(const char* key, int cursor,
	std::vector<std::pair<string, double> >& out,
	const char* pattern /* = NULL */, const size_t* count /* = NULL */)
{
	if (key == NULL || *key == 0 || cursor < 0)
		return -1;

	size_t size;
	const redis_result** children = scan_keys("ZSCAN", key, cursor,
		size, pattern, count);
	if (children == NULL)
		return cursor;

	if (size % 2 != 0)
		return -1;

	out.reserve(out.size() + size);

	const redis_result* rr;
	string name(128), value(128);

	for (size_t i = 0; i < size;)
	{
		rr = children[i];
		rr->argv_to_string(name);
		i++;

		rr = children[i];
		rr->argv_to_string(value);
		i++;

		out.push_back(std::make_pair(name, atof(value.c_str())));
	}

	return cursor;
}

} // namespace acl
