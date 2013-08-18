#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/memcache/memcache.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

#define	SPECIAL_CHAR(x)	((x) == ' ' || (x) == '\t' || (x) == '\r' || (x) == '\n')

namespace acl
{

memcache::memcache(const char* addr /* = "127.0.0.1:11211" */,
	int conn_timeout /* = 180 */, int rw_timeout /* = 300 */)
: keypre_(NULL)
, coder_(false, false)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, encode_key_(false)
, opened_(false)
, retry_(true)
, conn_(NULL)
{
	acl_assert(addr && *addr);
	addr_ = acl_mystrdup(addr);
	char* ptr = strchr(addr_, ':');
	if (ptr == NULL)
		logger_fatal("addr(%s) invalid", addr);
	*ptr++ = 0;
	if (*ptr == 0)
		logger_fatal("addr(%s) invalid", addr);
	ip_ = addr_;
	port_ = atoi(ptr);
	if (port_ <= 0)
		logger_fatal("addr(%s) invalid", addr);
}

memcache::~memcache()
{
	close();
	if (keypre_)
		delete keypre_;
	acl_myfree(addr_);
}

memcache& memcache::set_prefix(const char* keypre)
{
	if (keypre == NULL || *keypre == 0)
	{
		if (keypre_)
		{
			delete keypre_;
			keypre_ = NULL;
		}
		return *this;
	}

	bool beCoding = false;

	if (keypre_ == NULL)
		keypre_ = NEW acl::string(strlen(keypre));
	else
		keypre_->clear();

	while (*keypre)
	{
		if (SPECIAL_CHAR(*keypre) || !ACL_ISPRINT(*keypre))
		{
			coder_.encode_update(keypre, 1, keypre_);
			beCoding = true;
		}
		else if (beCoding)
		{
			coder_.encode_finish(keypre_);
			coder_.reset();
			beCoding = false;
			*keypre_ << (char) *keypre;
		}
		else
			*keypre_ << (char) *keypre;
		keypre++;
	}

	if (beCoding)
		coder_.encode_finish(keypre_);
	return *this;
}

memcache& memcache::auto_retry(bool onoff)
{
	retry_ = onoff;
	return *this;
}

memcache& memcache::encode_key(bool onoff)
{
	encode_key_ = onoff;
	return *this;
}

void memcache::close()
{
	if (opened_ == false)
		return;

	if (conn_)
	{
		delete conn_;
		conn_ = NULL;
	}
	opened_ = false;
}

bool memcache::open()
{
	if (opened_)
		return (true);

	conn_ = NEW socket_stream();
	char  addr[64];

	snprintf(addr, sizeof(addr), "%s:%d", ip_, port_);
	if (conn_->open(addr, conn_timeout_, rw_timeout_) == false)
	{
		logger_error("connect %s error(%s)",
			addr, acl::last_serror());
		delete conn_;
		conn_ = NULL;
		opened_ = false;
		ebuf_.format("connect server(%s) error(%s)",
			addr, acl_last_serror());
		return (false);
	}
	opened_ = true;
	return (true);
}

bool memcache::set(const acl::string& key, const void* dat, size_t dlen,
	time_t timeout, unsigned short flags)
{
	bool has_tried = false;
	struct iovec v[4];
	line_.format("set %s %u %d %d\r\n", key.c_str(),
		flags, (int) timeout, (int) dlen);
AGAIN:
	if (open() == false)
		return (false);

	v[0].iov_base = (void*) line_.c_str();
	v[0].iov_len = line_.length();
	v[1].iov_base = (void*) dat;
	v[1].iov_len = dlen;
	v[2].iov_base = (void*) "\r\n";
	v[2].iov_len = 2;

	if (conn_->writev(v, 3) < 0)
	{
		close();
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write set(%s) error", key.c_str());
		return (false);
	}

	if (conn_->gets(line_) == false)
	{
		close();
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply for set(%s) error", key.c_str());
		return (false);
	}

	if (line_ != "STORED")
	{
		close();
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply(%s) for set(%s) error",
			line_.c_str(), key.c_str());
		return (false);
	}
	return (true);
}

bool memcache::set(const char* key, size_t klen, const void* dat,
	size_t dlen, time_t timeout /* = 0 */, unsigned short flags /* = 0 */)
{
	const acl::string& keybuf = get_key(key, klen);
	return (set(keybuf, dat, dlen, timeout, flags));
}

bool memcache::set(const char* key, const void* dat, size_t dlen,
	time_t timeout /* = 0 */, unsigned short flags /* = 0 */)
{
	return (set(key, strlen(key), dat, dlen, timeout, flags));
}

bool memcache::set(const char* key, size_t klen, time_t timeout /* = 0 */)
{
	const acl::string& keybuf = get_key(key, klen);
	acl::string buf;
	unsigned short flags;

	if (get(keybuf, buf, &flags) == false)
		return (false);
	return (set(keybuf, buf.c_str(), buf.length(), timeout, flags));
}

bool memcache::set(const char* key, time_t timeout /* = 0 */)
{
	return (set(key, strlen(key), timeout));
}

bool memcache::get(const acl::string& key, acl::string& buf,
	unsigned short* flags)
{
	bool has_tried = false;
	buf.clear();

	line_.format("get %s\r\n", key.c_str());

AGAIN:
	if (open() == false)
		return (false);
	if (conn_->write(line_) < 0)
	{
		close();
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write get(%s) error", key.c_str());
		return (false);
	}

	// 读取服务器响应行
	if (conn_->gets(line_) == false)
	{
		close();
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply for get(%s) error", key.c_str());
		return (false);
	}
	else if (line_ == "END")
	{
		ebuf_.format("not found");
		return (false);
	}
	else if (error_happen(line_.c_str()))
	{
		close();
		return (false);
	}

	// VALUE {key} {flags} {bytes}\r\n
	ACL_ARGV* tokens = acl_argv_split(line_.c_str(), " \t");
	if (tokens->argc < 4 || strcasecmp(tokens->argv[0], "VALUE") != 0)
	{
		close();
		ebuf_.format("server error for get(%s)", key.c_str());
		acl_argv_free(tokens);
		return (false);
	}
	if (flags)
		*flags = (unsigned short) atoi(tokens->argv[2]);

	int len = atoi(tokens->argv[3]);
	if (len < 0)
	{
		close();
		ebuf_.format("value's len < 0");
		acl_argv_free(tokens);
		return (false);
	}
	else if (len == 0)
	{
		acl_argv_free(tokens);
		return (true);
	}
	acl_argv_free(tokens);

	// 得需要保证足够的空间能容纳读取的数据，该种方式
	// 可能会造成数据量非常大时的缓冲区溢出！

	char  tmp[4096];
	int   n;
	while (true)
	{
		n = sizeof(tmp);
		if (n > len)
			n = len;
		if ((n = conn_->read(tmp, n, false)) < 0)
		{
			close();
			ebuf_.format("read data for get cmd error");
			return (false);
		}
		buf.append(tmp, n);
		len -= n;
		if (len <= 0)
			break;
	}

	// 读取数据尾部的 "\r\n"
	if (conn_->gets(line_) == false)
	{
		close();
		ebuf_.format("read data's delimiter error");
		return (false);
	}

	// 读取 "END\r\n"
	if (conn_->gets(line_) == false || line_ != "END")
	{
		close();
		ebuf_.format("END flag not found");
		return (false);
	}
	return (true);
}

bool memcache::get(const char* key, size_t klen, acl::string& buf,
	unsigned short* flags /* = NULL */)
{
	const acl::string& keybuf = get_key(key, klen);
	return (get(keybuf, buf, flags));
}

bool memcache::get(const char* key, acl::string& buf,
	unsigned short* flags /* = NULL */)
{
	return (get(key, strlen(key), buf, flags));
}

bool memcache::del(const char* key, size_t klen)
{
	bool has_tried = false;
	const acl::string& keybuf = get_key(key, klen);

AGAIN:
	if (open() == false)
		return (false);

	line_.format("delete %s\r\n", keybuf.c_str());
	if (conn_->write(line_) < 0)
	{
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write (%s) error", line_.c_str());
		return (false);
	}
	// DELETED|NOT_FOUND\r\n
	if (conn_->gets(line_) == false)
	{
		if (retry_ && !has_tried)
		{
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply for(%s) error", line_.c_str());
		return (false);
	}
	if (line_ != "DELETED" && line_ != "NOT_FOUND")
	{
		ebuf_.format("reply for (%s) error", line_.c_str());
		return (false);
	}
	return (true);
}

bool memcache::del(const char* key)
{
	return (del(key, strlen(key)));
}

const char* memcache::last_serror() const
{
	static const char* dummy = "ok";

	if (ebuf_.empty())
		return (dummy);
	return (ebuf_.c_str());
}

int memcache::last_error() const
{
	return (enum_);
}

const acl::string& memcache::get_key(const char* key, size_t klen)
{
	kbuf_.clear();
	if (keypre_)
		kbuf_.format("%s:", keypre_->c_str());

	coder_.reset();

	if (encode_key_)
	{
		coder_.encode_update(key, klen, &kbuf_);
		coder_.encode_finish(&kbuf_);
		return (kbuf_);
	}

	bool beCoding = false;

	while (klen > 0)
	{
		if (SPECIAL_CHAR(*key) || !ACL_ISPRINT(*key))
		{
			coder_.encode_update(key, 1, &kbuf_);
			beCoding = true;
		}
		else if (beCoding)
		{
			coder_.encode_finish(&kbuf_);
			coder_.reset();
			beCoding = false;
			kbuf_ << (char) *key;
		}
		else
			kbuf_ << (char) *key;
		key++;
		klen--;
	}

	if (beCoding)
		coder_.encode_finish(&kbuf_);

	return (kbuf_);
}

bool memcache::error_happen(const char* line)
{
	if (strcasecmp(line, "ERROR") == 0)
		return (true);
	if (strncasecmp(line, "CLIENT_ERROR", sizeof("CLIENT_ERROR") - 1) == 0)
	{
		ebuf_.format("%s", line);
		const char* ptr = line + sizeof("CLIENT_ERROR") - 1;
		if (*ptr == ' ' || *ptr == '\t')
			ptr++;
		enum_ = atoi(ptr);
		return (true);
	}
	if (strncasecmp(line, "SERVER_ERROR", sizeof("SERVER_ERROR") - 1) == 0)
	{
		ebuf_.format("%s", line);
		const char* ptr = line + sizeof("SERVER_ERROR") - 1;
		if (*ptr == ' ' || *ptr == '\t')
			ptr++;
		enum_ = atoi(ptr);
		return (true);
	}
	return (false);
}

void memcache::property_list()
{
}

} // namespace acl
