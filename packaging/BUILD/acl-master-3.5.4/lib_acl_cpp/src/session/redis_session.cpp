#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_client_cluster.hpp"
#include "acl_cpp/redis/redis.hpp"
#include "acl_cpp/session/redis_session.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

redis_session::redis_session(redis_client_cluster& cluster,
	time_t ttl /* = 0 */, const char* sid /* = NULL */)
: session(ttl, sid)
, cluster_(cluster)
{
	command_ = NEW redis;
	command_->set_cluster(&cluster_);
}

redis_session::~redis_session(void)
{
	delete command_;
	std::map<string, session_string*>::iterator it;
	for (it = buffers_.begin(); it != buffers_.end(); ++it) {
		delete it->second;
	}
}

bool redis_session::set(const char* name, const char* value)
{
	return set(name, value, strlen(value));
}

bool redis_session::set(const char* name, const void* value, size_t len)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	if (command_->hset(sid, name, (const char*) value, len) < 0) {
		return false;
	}
	time_t ttl = get_ttl();
	if (ttl > 0) {
		return set_timeout(ttl);
	}
	return true;
}

const session_string* redis_session::get_buf(const char* name)
{
	command_->clear();
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return NULL;
	}

	// 先尝试从缓存池中获得一个缓冲区，如果没有合适的，则创建新的缓冲区对象
	// 并将之加入至缓冲池中，以备下次重复查询相同属性时使用

	session_string* ss;
	std::map<string, session_string*>::iterator it = buffers_.find(name);
	if (it == buffers_.end()) {
		ss = NEW session_string;
		buffers_[name] = ss;
	} else {
		ss = it->second;
		ss->clear();
	}

	if (!command_->hget(sid, name, *ss)) {
		return NULL;
	}
	return ss;
}

bool redis_session::del(const char* name)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	return command_->hdel(sid, name) >= 0 ? true : false;
}

bool redis_session::set_attrs(const std::map<string, session_string>& attrs)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	if (!command_->hmset(sid, (const std::map<string, string>&) attrs)) {
		return false;
	}

	time_t ttl = get_ttl();
	if (ttl > 0) {
		return set_timeout(ttl);
	} else {
		return true;
	}
}

bool redis_session::get_attrs(std::map<string, session_string>& attrs)
{
	attrs_clear(attrs);
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	return command_->hgetall(sid, (std::map<string, string>&) attrs);
}

bool redis_session::get_attrs(const std::vector<string>& names,
	std::vector<session_string>& values)
{
	values.clear();
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	std::vector<string> vals;
	if (!command_->hmget(sid, names, &vals)) {
		return false;
	}

	for (std::vector<string>::const_iterator cit = vals.begin();
		cit != vals.end(); ++cit) {

		values.push_back(*cit);
	}

	return true;
}

bool redis_session::remove(void)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	return command_->del(sid) >= 0 ? true : false;
}

bool redis_session::set_timeout(time_t ttl)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0) {
		return false;
	}

	command_->clear();
	return command_->expire(sid, (int) ttl) > 0 ? true : false;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
