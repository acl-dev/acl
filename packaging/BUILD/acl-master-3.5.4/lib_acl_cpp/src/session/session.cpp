#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <utility>
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/escape.hpp"
#include "acl_cpp/session/session.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

session::session(time_t ttl /* = 0 */, const char* sid /* = NULL */)
: sid_(64)
, ttl_(ttl)
, dirty_(false)
{
	if (sid == NULL || *sid == 0) {
		struct timeval tv;

		(void) gettimeofday(&tv, NULL);
		sid_.format("acl.%d.%d.%d", (int) tv.tv_sec,
			(int) tv.tv_usec, rand());
		sid_.todo_ = TODO_NUL;
		sid_saved_ = false;
	} else {
		sid_.copy(sid);
		sid_.todo_ = TODO_NUL;
		sid_saved_ = true;
	}
}

session::~session(void)
{
	reset();
}

void session::set_sid(const char* sid)
{
	sid_.copy(sid);
	sid_.todo_ = TODO_NUL;

	// 有可能已经存储在后端 cache 服务端了
	if (!sid_saved_) {
		sid_saved_ = true;
	}

	// 必须清除上次的中间结果
	reset();
}

void session::reset(void)
{
	attrs_clear(attrs_);
	attrs_clear(attrs_cache_);
}

void session::attrs_clear(std::map<string, session_string>& attrs)
{
	attrs.clear();
}

bool session::flush(void)
{
	if (!dirty_) {
		return true;
	}
	dirty_ = false;

	// 调用纯虚接口，获得原来的 sid 数据
	if (get_attrs(attrs_)) {
		if (!sid_saved_) {
			sid_saved_ = true;
		}
	}

	std::map<string, session_string>::iterator it_cache =
		attrs_cache_.begin();
	for (; it_cache != attrs_cache_.end(); ++it_cache) {
		// 如果该属性已存在，则需要先释放原来的属性值后再添加新值

		std::map<string, session_string>::iterator it_attr =
			attrs_.find(it_cache->first);
		if (it_attr == attrs_.end()) {
			if (it_cache->second.todo_ == TODO_SET) {
				attrs_.insert(std::make_pair(it_cache->first,
					it_cache->second));
			}
		} else if (it_cache->second.todo_ == TODO_SET) {
			// 设置新的数据
			attrs_.insert(std::make_pair(it_cache->first,
				it_cache->second));
		} else if (it_cache->second.todo_ == TODO_DEL) {
			attrs_.erase(it_attr);
		} else {
			logger_warn("unknown todo(%d)",
				(int) it_cache->second.todo_);
		}
	}

	// 清除缓存的数据：因为内部的数据已经被添加至 attrs_ 中，
	// 所以只需要将 attrs_cache_ 空间清除即可
	attrs_cache_.clear();

	// 调用纯虚接口，向 memcached 或类似缓存中添加数据
	if (!this->set_attrs(attrs_)) {
		logger_error("set cache error, sid(%s)", sid_.c_str());
		attrs_clear(attrs_);  // 清除属性集合数据

		return false;
	}

	attrs_clear(attrs_);  // 清除属性集合数据

	if (!sid_saved_) {
		sid_saved_ = true;
	}
	return true;
}

bool session::set(const char* name, const char* value)
{
	return set(name, value, strlen(value));
}

bool session::set_delay(const char* name, const void* value, size_t len)
{
	session_string ss(len);
	ss.copy(value, len);
	ss.todo_ = TODO_SET;
	attrs_cache_.insert(std::make_pair(string(name), ss));

	dirty_ = true;
	return true;
}

bool session::set(const char* name, const void* value, size_t len)
{
	// 直接操作后端 cache 服务器，设置(添加/修改) 属性字段

	// 调用纯虚接口，获得原来的 sid 数据
	if (!this->get_attrs(attrs_)) {
		session_string ss(len);
		ss.copy(value, len);
		ss.todo_ = TODO_SET;
		attrs_cache_.insert(std::make_pair(string(name), ss));
	}
	// 如果存在对应 sid 的数据，则将新数据添加在原来数据中
	else {
		if (!sid_saved_) {
			sid_saved_ = true;
		}

		// 如果该属性已存在，则需要先释放原来的属性值后再添加新值
		session_string ss(len);
		ss.copy(value, len);
		ss.todo_ = TODO_SET;
		attrs_cache_.insert(std::make_pair(string(name), ss));
	}

	// 调用纯虚接口，向 memcached 或类似缓存中添加数据
	if (!this->set_attrs(attrs_)) {
		logger_error("set cache error, sid(%s)", sid_.c_str());
		attrs_clear(attrs_);  // 清除属性集合数据

		return false;
	}
	attrs_clear(attrs_);  // 清除属性集合数据

	if (!sid_saved_) {
		sid_saved_ = true;
	}
	return true;
}

const char* session::get(const char* name)
{
	const session_string* bf = get_buf(name);
	if (bf == NULL) {
		return "";
	}
	return bf->c_str();
}

const session_string* session::get_buf(const char* name)
{
	attrs_clear(attrs_);

	if (!get_attrs(attrs_)) {
		return NULL;
	}

	std::map<string, session_string>::const_iterator cit = attrs_.find(name);
	if (cit == attrs_.end()) {
		return NULL;
	}
	return &cit->second;
}

bool session::get_attrs(const std::vector<string>& names,
	std::vector<session_string>& values)
{
	attrs_clear(attrs_);

	if (!get_attrs(attrs_)) {
		return false;
	}

	for (std::vector<string>::const_iterator cit = names.begin();
		cit != names.end(); ++cit) {

		std::map<string, session_string>::const_iterator cit2
			= attrs_.find(*cit);
		if (cit2 != attrs_.end()) {
			values.push_back(cit2->second);
		} else {
			values.push_back("");
		}
	}

	return true;
}

bool session::set_ttl(time_t ttl, bool delay)
{
	if (ttl == ttl_) {
		return true;
	}

	// 如果是延迟修改，则仅设置相关成员变量，最后统一 flush
	else if (delay) {
		ttl_   = ttl;
		dirty_ = true;
		return true;
	}

#if 0
	// 如果该 sid 还没有在后端 cache 上存储过，则仅在对象中本地设置一下
	else if (!sid_saved_) {
		ttl_ = ttl;
		return true;
	}
#endif

	// 修改后端 cache 上针对该 sid 的 ttl
	else if (set_timeout(ttl)) {
		ttl_ = ttl;
		return true;
	} else {
		return false;
	}
}

bool session::del_delay(const char* name)
{
	std::map<string, session_string>::iterator it = attrs_cache_.find(name);
	if (it != attrs_cache_.end()) {
		it->second.todo_ = TODO_DEL;
	}
	dirty_ = true;
	return true;
}

bool session::del(const char* name)
{
	// 直接操作后端 cache 服务器，删除属性字段

	if (!get_attrs(attrs_)) {
		return true;
	}

	std::map<string, session_string>::iterator it = attrs_.find(name);
	if (it == attrs_.end()) {
		return false;
	}

	// 先删除并释放对应的对象
	attrs_.erase(it);

	// 如果 sid 中已经没有了数据，则应该将 sid 对象从 memcached 中删除
	if (attrs_.empty()) {
		// 调用虚函数，删除该 sid 对应的缓存内容
		if (!this->remove()) {
			logger_error("del sid(%s) error", sid_.c_str());
			return false;
		}
		return true;
	}

	// 重新添加剩余的数据

	if (!set_attrs(attrs_)) {
		logger_error("set cache error, sid(%s)", sid_.c_str());
		attrs_clear(attrs_);  // 清除属性集合数据

		return false;
	}

	attrs_clear(attrs_);  // 清除属性集合数据
	return true;
}

// 采用 handlersocket 的编码方式

void session::serialize(const std::map<string, session_string>& attrs,
	string& out)
{
	out.clear(); // 先清除缓冲区

	std::map<string, session_string>::const_iterator it = attrs.begin();
	if (it == attrs.end()) {
		return;
	}

	// 添加第一个属性
	const char ch = 1;
	escape(it->first.c_str(), it->first.length(), out);
	escape(&ch, 1, out);
	escape(it->second.c_str(), it->second.length(), out);
	++it;

	// 添加后续的属性
	for (; it != attrs.end(); ++it) {
		// 除第一个属性外后续的都需要添加分隔符
		out << '\t';
		escape(it->first.c_str(), it->first.length(), out);
		escape(&ch, 1, out);
		escape(it->second.c_str(), it->second.length(), out);
	}
}

// 采用 handlersocket 的解码方式

void session::deserialize(string& buf, std::map<string, session_string>& attrs)
{
	attrs_clear(attrs);  // 先重置 session 前一次查询状态

	ACL_ARGV* tokens = acl_argv_split(buf.c_str(), "\t");
	ACL_ITER  iter;
	acl_foreach(iter, tokens) {
		char* ptr = (char*) iter.data;

		// 重复使用原来的内存区，因为 tokens 中已经存储了中间结果数据
		buf.clear();
		if (!unescape(ptr, strlen(ptr), buf)) {
			logger_error("unescape error");
			continue;
		}
		ptr = buf.c_str();
		// 因为 acl::string 肯定能保证缓冲区数据的尾部有 \0，所以在用
		// strchr 时不必须担心越界问题，但 std::string 并不保证这样
		char* p1 = strchr(ptr, 1);
		if (p1 == NULL || *(p1 + 1) == 0) {
			continue;
		}
		*p1++ = 0;
		//std::map<string, session_string>::iterator it = attrs.find(ptr);

		size_t len = buf.length() - (p1 - buf.c_str());
		session_string ss(len);
		ss.copy(p1, len);
		ss.todo_ = TODO_SET;
		attrs.insert(std::make_pair(string(ptr), ss));
	}

	acl_argv_free(tokens);
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
