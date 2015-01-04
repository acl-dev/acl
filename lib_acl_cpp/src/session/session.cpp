#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/escape.hpp"
#include "acl_cpp/session/session.hpp"

namespace acl
{

VBUF* session::vbuf_new(const void* str, size_t len, todo_t todo)
{
	// 这样可以减少分配内存的次数
	VBUF* buf = (VBUF*) acl_mymalloc(sizeof(VBUF) + len + 1);
	buf->size = len + 1;
	buf->todo = todo;

	memcpy(buf->buf, str, len);
	// 必须保证尾部以 \0 结束以允许返回字符串属性值
	buf->buf[len] = 0;
	buf->len = len;
	return buf;
}

VBUF* session::vbuf_set(VBUF* buf, const void* str, size_t len, todo_t todo)
{
	if (buf == NULL)
	{
		buf = (VBUF*) acl_mymalloc(sizeof(VBUF) + len + 1);
		buf->size = len + 1;
	}
	else if (buf->size <= len)
	{
		buf = (VBUF*) acl_myrealloc(buf, sizeof(VBUF) + len + 1);
		buf->size = len + 1;
	}

	buf->todo = todo;
	memcpy(buf->buf, str, len);
	buf->buf[len] = 0;
	buf->len = len;
	return buf;
}

void session::vbuf_free(VBUF* buf)
{
	acl_myfree(buf);
}

session::session(time_t ttl /* = 0 */, const char* sid /* = NULL */)
: ttl_(ttl)
, dirty_(false)
{
	struct timeval tv;

	(void) gettimeofday(&tv, NULL);
	if (sid == NULL || *sid == 0)
	{
		char buf[128];
		safe_snprintf(buf, sizeof(buf), "acl.%d.%d.%d",
			(int) tv.tv_sec, (int) tv.tv_usec, rand());
		sid_ = vbuf_new(buf, strlen(buf), TODO_NUL);
		sid_saved_ = false;
	}
	else
	{
		sid_ = vbuf_new(sid, strlen(sid), TODO_NUL);
		sid_saved_ = true;
	}
}

session::~session()
{
	reset();
	vbuf_free(sid_);
}

const char* session::get_sid() const
{
	return sid_->buf;
}

void session::set_sid(const char* sid)
{
	sid_ = vbuf_set(sid_, sid, strlen(sid), TODO_NUL);

	// 有可能已经存储在后端 cache 服务端了
	if (!sid_saved_)
		sid_saved_ = true;

	// 必须清除上次的中间结果
	reset();
}

void session::reset()
{
	attrs_clear(attrs_);
	attrs_clear(attrs_cache_);
}

void session::attrs_clear(std::map<string, VBUF*>& attrs)
{
	if (attrs.empty())
		return;

	std::map<string, VBUF*>::iterator it = attrs.begin();
	for (; it != attrs.end(); ++it)
		vbuf_free(it->second);
	attrs.clear();
}

bool session::flush()
{
	if (!dirty_)
		return true;
	dirty_ = false;

	string buf(256);

	// 调用纯虚接口，获得原来的 sid 数据
	if (get_data(sid_->buf, buf) == true)
	{
		if (!sid_saved_)
			sid_saved_ = true;
		deserialize(buf, attrs_);  // 反序列化
	}

	std::map<string, VBUF*>::iterator it_cache = attrs_cache_.begin();
	for (; it_cache != attrs_cache_.end(); ++it_cache)
	{
		// 如果该属性已存在，则需要先释放原来的属性值后再添加新值

		std::map<string, VBUF*>::iterator it_attr =
			attrs_.find(it_cache->first);
		if (it_attr == attrs_.end())
		{
			if (it_cache->second->todo == TODO_SET)
				attrs_[it_cache->first] = it_cache->second;
			else
				vbuf_free(it_cache->second);
		}
		else if (it_cache->second->todo == TODO_SET)
		{
			// 清除旧的数据
			vbuf_free(it_attr->second);
			// 设置新的数据
			attrs_[it_cache->first] = it_cache->second;
		}
		else if (it_cache->second->todo == TODO_DEL)
		{
			vbuf_free(it_attr->second);
			attrs_.erase(it_attr);
			vbuf_free(it_cache->second);
		}
		else
		{
			logger_warn("unknown todo(%d)",
				(int) it_cache->second->todo);
			vbuf_free(it_cache->second);
		}
	}

	// 清除缓存的数据：因为内部的数据已经被添加至 attrs_ 中，
	// 所以只需要将 attrs_cache_ 空间清除即可
	attrs_cache_.clear();

	serialize(attrs_, buf);  // 序列化数据
	attrs_clear(attrs_);  // 清除属性集合数据

	// 调用纯虚接口，向 memcached 或类似缓存中添加数据
	if (set_data(sid_->buf, buf.c_str(), buf.length(), ttl_) == false)
	{
		logger_error("set cache error, sid(%s)", sid_->buf);
		return false;
	}

	if (!sid_saved_)
		sid_saved_ = true;
	return true;
}

bool session::set(const char* name, const char* value,
	bool delay /* = false */)
{
	return set(name, value, strlen(value), delay);
}

bool session::set(const char* name, const void* value, size_t len,
	bool delay /* = false */)
{
	if (delay)
	{
		std::map<string, VBUF*>::iterator it = attrs_cache_.find(name);
		if (it == attrs_cache_.end())
			attrs_cache_[name] = vbuf_new(value, len, TODO_SET);
		else
			attrs_cache_[name] = vbuf_set(it->second, value,
							len, TODO_SET);
		dirty_ = true;
		return true;
	}

	// 直接操作后端 cache 服务器，设置(添加/修改) 属性字段

	string buf(256);

	// 调用纯虚接口，获得原来的 sid 数据
	if (get_data(sid_->buf, buf) == false)
	{
		// 如果没有则创建新的 sid 数据
		serialize(name, value, len, buf);
	}

	// 如果存在对应 sid 的数据，则将新数据添加在原来数据中
	else
	{
		if (!sid_saved_)
			sid_saved_ = true;

		// 反序列化
		deserialize(buf, attrs_);

		// 如果该属性已存在，则需要先释放原来的属性值后再添加新值

		std::map<string, VBUF*>::iterator it = attrs_.find(name);
		if (it == attrs_.end())
			attrs_[name] = vbuf_new(value, len, TODO_SET);
		else
			attrs_[name] = vbuf_set(it->second, value,
						len, TODO_SET);
		serialize(attrs_, buf);  // 序列化数据
		attrs_clear(attrs_);
	}

	// 调用纯虚接口，向 memcached 或类似缓存中添加数据
	if (set_data(sid_->buf, buf.c_str(), buf.length(), ttl_) == false)
	{
		logger_error("set cache error, sid(%s)", sid_->buf);
		return false;
	}
	if (!sid_saved_)
		sid_saved_ = true;
	return true;
}

const char* session::get(const char* name, bool local_cached /* = false */)
{
	const VBUF* bf = get_vbuf(name, local_cached);
	if (bf == NULL)
		return "";
	return bf->buf;
}

const VBUF* session::get_vbuf(const char* name, bool local_cached /* = false */)
{
	string buf(256);
	if (local_cached == false || attrs_.empty())
	{
		if (get_data(sid_->buf, buf) == false)
			return NULL;
		deserialize(buf, attrs_);
	}
	std::map<string, VBUF*>::const_iterator cit = attrs_.find(name);
	if (cit == attrs_.end())
		return NULL;
	return cit->second;
}

bool session::set_ttl(time_t ttl, bool delay /* = true */)
{
	if (ttl == ttl_)
		return true;

	// 如果是延迟修改，则仅设置相关成员变量，最后统一 flush
	else if (delay)
	{
		ttl_ = ttl;
		dirty_ = true;
		return true;
	}

	// 如果该 sid 还没有在后端 cache 上存储过，则仅在对象中本地设置一下
	else if (!sid_saved_)
	{
		ttl_ = ttl;
		return true;
	}

	// 修改后端 cache 上针对该 sid 的 ttl
	else if (set_timeout(sid_->buf, ttl) == true)
	{
		ttl_ = ttl;
		return true;
	}
	else
		return false;
}

time_t session::get_ttl() const
{
	return ttl_;
}

bool session::del(const char* name, bool delay /* = false */)
{
	if (delay)
	{
		std::map<string, VBUF*>::iterator it = attrs_cache_.find(name);
		if (it == attrs_cache_.end())
			attrs_cache_[name] = vbuf_new("", 0, TODO_DEL);
		else
			it->second->todo = TODO_DEL;
		dirty_ = true;
		return true;
	}

	// 直接操作后端 cache 服务器，删除属性字段

	string buf(256);
	if (get_data(sid_->buf, buf) == false)
		return true;

	deserialize(buf, attrs_);
	std::map<string, VBUF*>::iterator it = attrs_.find(name);
	if (it == attrs_.end())
		return false;

	// 先删除并释放对应的对象
	vbuf_free(it->second);
	attrs_.erase(it);

	// 如果 sid 中已经没有了数据，则应该将 sid 对象从 memcached 中删除
	if (attrs_.empty())
	{
		// 调用虚函数，删除该 sid 对应的缓存内容
		if (del_data(sid_->buf) == false)
		{
			logger_error("del sid(%s) error", sid_->buf);
			return false;
		}
		return true;
	}

	// 向 memcached 中重新添加剩余的数据

	serialize(attrs_, buf);
	attrs_clear(attrs_);

	if (set_data(sid_->buf, buf.c_str(), buf.length(), ttl_) == false)
	{
		logger_error("set cache error, sid(%s)", sid_->buf);
		return false;
	}
	return true;
}

bool session::remove()
{
	// 调用虚函数，删除缓存对象
	if (del_data(sid_->buf) == false)
	{
		logger_error("invalid sid(%s) error", sid_->buf);
		return false;
	}
	return true;
}

// 采用 handlersocket 的编码方式

void session::serialize(const std::map<string, VBUF*>& attrs, string& out)
{
	out.clear(); // 先清除缓冲区

	std::map<string, VBUF*>::const_iterator it = attrs.begin();
	if (it == attrs.end())
		return;

	// 添加第一个属性
	const char ch = 1;
	escape(it->first.c_str(), it->first.length(), out);
	escape(&ch, 1, out);
	escape(it->second->buf, it->second->len, out);
	++it;

	// 添加后续的属性
	for (; it != attrs.end(); ++it)
	{
		// 除第一个属性外后续的都需要添加分隔符
		out << '\t';
		escape(it->first.c_str(), it->first.length(), out);
		escape(&ch, 1, out);
		escape(it->second->buf, it->second->len, out);
	}
}

void session::serialize(const char* name, const void* value,
	size_t len, string& out)
{
	escape(name, strlen(name), out);
	const char ch = 1;
	escape(&ch, 1, out);
	escape((const char*) value, len, out);
}

// 采用 handlersocket 的解码方式

void session::deserialize(string& buf, std::map<string, VBUF*>& attrs)
{
	attrs_clear(attrs);  // 先重置 session 前一次查询状态

	ACL_ARGV* tokens = acl_argv_split(buf.c_str(), "\t");
	ACL_ITER  iter;
	acl_foreach(iter, tokens)
	{
		char* ptr = (char*) iter.data;

		// 重复使用原来的内存区，因为 tokens 中已经存储了中间结果数据
		buf.clear();
		if (unescape(ptr, strlen(ptr), buf) == false)
		{
			logger_error("unescape error");
			continue;
		}
		ptr = buf.c_str();
		// 因为 acl::string 肯定能保证缓冲区数据的尾部有 \0，所以在用
		// strchr 时不必须担心越界问题，但 std::string 并不保证这样
		char* p1 = strchr(ptr, 1);
		if (p1 == NULL || *(p1 + 1) == 0)
			continue;
		*p1++ = 0;
		std::map<string, VBUF*>::iterator it = attrs.find(ptr);

		// xxx: 以防有重复的属性
		if (it != attrs.end())
			vbuf_free(it->second);
		// 将从后端取得数据属性都设为 TODO_SET
		attrs[ptr] = vbuf_new(p1, buf.length() - (p1 - buf.c_str()),
				TODO_SET);
	}

	acl_argv_free(tokens);
}

} // namespace acl
