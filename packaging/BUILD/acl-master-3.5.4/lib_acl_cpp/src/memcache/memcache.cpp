#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include "acl_cpp/memcache/memcache.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#endif

#define	SPECIAL_CHAR(x)	((x) == ' ' || (x) == '\t' || (x) == '\r' || (x) == '\n')

#ifndef ACL_CLIENT_ONLY

namespace acl
{

memcache::memcache(const char* addr /* = "127.0.0.1:11211" */,
	int conn_timeout /* = 30 */, int rw_timeout /* = 30 */)
: keypre_(NULL)
, coder_(false, false)
, encode_key_(false)
, opened_(false)
, retry_(true)
, content_length_(0)
, length_(0)
, conn_(NULL)
{
	acl_assert(addr && *addr);
	addr_ = acl_mystrdup(addr);
	set_timeout(conn_timeout, rw_timeout);
}

memcache::~memcache(void)
{
	close();
	delete keypre_;
	acl_myfree(addr_);
}

memcache& memcache::set_prefix(const char* keypre)
{
	if (keypre == NULL || *keypre == 0) {
		delete keypre_;
		keypre_ = NULL;
		return *this;
	}

	bool beCoding = false;

	if (keypre_ == NULL) {
		keypre_ = NEW string(strlen(keypre));
	} else {
		keypre_->clear();
	}

	while (*keypre) {
		if (SPECIAL_CHAR(*keypre) || !ACL_ISPRINT(*keypre)) {
			coder_.encode_update(keypre, 1, keypre_);
			beCoding = true;
		} else if (beCoding) {
			coder_.encode_finish(keypre_);
			coder_.reset();
			beCoding = false;
			*keypre_ << (char) *keypre;
		} else {
			*keypre_ << (char) *keypre;
		}
		keypre++;
	}

	if (beCoding) {
		coder_.encode_finish(keypre_);
	}
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

void memcache::close(void)
{
	if (!opened_) {
		return;
	}

	delete conn_;
	conn_   = NULL;
	opened_ = false;
}

bool memcache::open(void)
{
	if (opened_) {
		return true;
	}

	conn_ = NEW socket_stream();

	if (!conn_->open(addr_, conn_timeout_, rw_timeout_)) {
		logger_error("connect %s error(%s)", addr_, last_serror());
		delete conn_;
		conn_ = NULL;
		opened_ = false;
		ebuf_.format("connect server(%s) error(%s)",
			addr_, acl_last_serror());
		return false;
	}
	opened_ = true;
	return true;
}

bool memcache::set(const string& key, const void* dat, size_t dlen,
	time_t timeout, unsigned short flags)
{
	bool has_tried = false;
	struct iovec v[3];
	req_line_.format("set %s %u %d %d\r\n", key.c_str(),
		flags, (int) timeout, (int) dlen);
AGAIN:
	if (!open()) {
		return false;
	}

#ifdef MINGW
	v[0].iov_base = (char*) req_line_.c_str();
#else
	v[0].iov_base = (void*) req_line_.c_str();
#endif
	v[0].iov_len = req_line_.length();
#ifdef MINGW
	v[1].iov_base = (char*) dat;
#else
	v[1].iov_base = (void*) dat;
#endif
	v[1].iov_len = dlen;
#ifdef MINGW
	v[2].iov_base = (char*) "\r\n";
#else
	v[2].iov_base = (void*) "\r\n";
#endif
	v[2].iov_len = 2;

	if (conn_->writev(v, 3) < 0) {
		close();
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write set(%s) error", key.c_str());
		return false;
	}

	if (!conn_->gets(res_line_)) {
		close();
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply for set(%s) error", key.c_str());
		return false;
	}

	if (res_line_.compare("STORED", false) != 0) {
		close();
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply(%s) for set(%s) error",
			res_line_.c_str(), key.c_str());
		return false;
	}
	return true;
}

bool memcache::set(const char* key, size_t klen, const void* dat,
	size_t dlen, time_t timeout /* = 0 */, unsigned short flags /* = 0 */)
{
	const string& kbuf = build_key(key, klen);
	return set(kbuf, dat, dlen, timeout, flags);
}

bool memcache::set(const char* key, const void* dat, size_t dlen,
	time_t timeout /* = 0 */, unsigned short flags /* = 0 */)
{
	return set(key, strlen(key), dat, dlen, timeout, flags);
}

bool memcache::set(const char* key, size_t klen, time_t timeout /* = 0 */)
{
	string buf;
	unsigned short flags;
	if (!get(key, klen, buf, &flags)) {
		return false;
	}

	const string& kbuf = build_key(key, klen);
	return set(kbuf, buf.c_str(), buf.length(), timeout, flags);
}

bool memcache::set(const char* key, time_t timeout /* = 0 */)
{
	return set(key, strlen(key), timeout);
}

bool memcache::set_begin(const char* key, size_t dlen,
	time_t timeout /* = 0 */, unsigned short flags /* = 0 */)
{
	if (dlen == 0) {
		logger_error("dlen == 0, invalid");
		return false;
	}

	content_length_ = dlen;
	length_ = 0;

	const string& kbuf = build_key(key, strlen(key));
	req_line_.format("set %s %u %d %d\r\n", kbuf.c_str(),
		flags, (int) timeout, (int) dlen);

	bool has_tried = false;

AGAIN:
	if (!open()) {
		return false;
	}

	if (conn_->write(req_line_) == -1) {
		close();
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write set(%s) error", key);
		return false;
	}
	return true;
}

bool memcache::set_data(const void* data, size_t dlen)
{
	if (!opened_) {
		ebuf_.format("not opened yet!");
		return false;
	}

	if (data == NULL || dlen == 0) {
		ebuf_.format("invalid input, data %s, dlen %d",
			data ? "not null" : "null", dlen ? (int) dlen : 0);
		return false;
	}

	if (dlen + length_ > content_length_) {
		ebuf_.format("dlen(%d) + length_(%d) > content_length_(%d)",
			(int) dlen, (int) length_, (int) content_length_);
		return false;
	}

	if (dlen + length_ < content_length_) {
		if (conn_->write(data, dlen) == -1) {
			close();
			ebuf_.format("write data error");
			return false;
		}
		length_ += dlen;
		return true;
	}

	struct iovec v[2];

#ifdef MINGW
	v[0].iov_base = (char*) data;
#else
	v[0].iov_base = (void*) data;
#endif
	v[0].iov_len = dlen;
#ifdef MINGW
	v[1].iov_base = (char*) "\r\n";
#else
	v[1].iov_base = (void*) "\r\n";
#endif
	v[1].iov_len = 2;

	if (conn_->writev(v, 2) < 0) {
		close();
		ebuf_.format("write data2 error!");
		return false;
	}
	length_ += dlen;

	if (!conn_->gets(res_line_)) {
		close();
		ebuf_.format("reply forerror");
		return false;
	}

	if (res_line_.compare("STORED", false) != 0) {
		close();
		ebuf_.format("reply(%s) error", res_line_.c_str());
		return false;
	}
	return true;
}

int memcache::get_begin(const char* key, unsigned short* flags /* = NULL */)
{
	return get_begin(key, strlen(key), flags);
}

int memcache::get_begin(const void* key, size_t klen, unsigned short* flags)
{
	content_length_ = 0;
	length_ = 0;

	bool has_tried = false;

	const string& kbuf = build_key((const char*) key, klen);
	req_line_.format("get %s\r\n", kbuf.c_str());

AGAIN:
	if (!open()) {
		return -1;
	}
	if (conn_->write(req_line_) < 0) {
		close();
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write get(%s) error", kbuf.c_str());
		return -1;
	}

	// 读取服务器响应行
	if (!conn_->gets(res_line_)) {
		close();
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply for get(%s) error", kbuf.c_str());
		return -1;
	} else if (res_line_.compare("END", false) == 0) {
		ebuf_.format("not found");
		return 0;
	} else if (error_happen(res_line_.c_str())) {
		close();
		return -1;
	}

	// VALUE {key} {flags} {bytes}\r\n
	ACL_ARGV* tokens = acl_argv_split(res_line_.c_str(), " \t");
	if (tokens->argc < 4 || strcasecmp(tokens->argv[0], "VALUE") != 0) {
		close();
		ebuf_.format("server error for get(%s), value: %s",
			kbuf.c_str(), res_line_.c_str());
		acl_argv_free(tokens);
		return -1;
	}
	if (flags) {
		*flags = (unsigned short) atoi(tokens->argv[2]);
	}

	content_length_ = atoi(tokens->argv[3]);
	acl_argv_free(tokens);

	// 如果服务端返回数据体长度值为 0 则当不存在处理
	if (content_length_ == 0) {
		return 0;
	}
	return (int) content_length_;
}

int memcache::get_data(void* buf, size_t size)
{
	acl_assert(content_length_ >= length_);

	if (length_ == content_length_) {
		// 读取数据尾部的 "\r\n"
		if (!conn_->gets(res_line_)) {
			close();
			ebuf_.format("read data CRLF error");
			return -1;
		}
		// 读取 "END\r\n"
		if (!conn_->gets(res_line_)
			|| res_line_.compare("END", false) != 0) {

			close();
			ebuf_.format("END flag not found");
			return -1;
		}
		return 0;
	}

	size_t n = content_length_ - length_;
	if (n > size) {
		n = size;
	}
	if (conn_->read(buf, n) < 0) {
		close();
		ebuf_.format("read data error!");
		return -1;
	}
	length_ += n;
	return (int) n;
}

bool memcache::get(const char* key, size_t klen, string& out,
	unsigned short* flags)
{
	out.clear();

	int  len = get_begin(key, klen, flags);
	if (len <= 0) {
		return false;
	}

	// 得需要保证足够的空间能容纳读取的数据，该种方式
	// 可能会造成数据量非常大时的缓冲区溢出！

	char  buf[4096];
	int   n;
	while (true) {
		n = get_data(buf, sizeof(buf));
		if (n < 0) {
			return false;
		} else if (n == 0) {
			break;
		}
		out.append(buf, n);
	}

	return !out.empty();
}

bool memcache::get(const char* key, string& buf, unsigned short* flags /* = NULL */)
{
	return get(key, strlen(key), buf, flags);
}

bool memcache::del(const char* key, size_t klen)
{
	bool has_tried = false;
	const string& kbuf = build_key(key, klen);

AGAIN:
	if (!open()) {
		return false;
	}

	req_line_.format("delete %s\r\n", kbuf.c_str());
	if (conn_->write(req_line_) < 0) {
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("write (%s) error", req_line_.c_str());
		return false;
	}
	// DELETED|NOT_FOUND\r\n
	if (!conn_->gets(res_line_)) {
		if (retry_ && !has_tried) {
			has_tried = true;
			goto AGAIN;
		}
		ebuf_.format("reply for(%s) error", req_line_.c_str());
		return false;
	}
	if (res_line_.compare("DELETED", false) != 0
		&& res_line_.compare("NOT_FOUND", false) != 0) {

		ebuf_.format("reply(%s) for (%s) error",
			res_line_.c_str(), req_line_.c_str());
		return false;
	}
	return true;
}

bool memcache::del(const char* key)
{
	return del(key, strlen(key));
}

const char* memcache::last_serror(void) const
{
	static const char* dummy = "ok";

	if (ebuf_.empty()) {
		return dummy;
	}
	return ebuf_.c_str();
}

int memcache::last_error(void) const
{
	return enum_;
}

const string& memcache::build_key(const char* key, size_t klen)
{
	kbuf_.clear();
	if (keypre_) {
		kbuf_.format("%s:", keypre_->c_str());
	}

	coder_.reset();

	if (encode_key_) {
		coder_.encode_update(key, (int) klen, &kbuf_);
		coder_.encode_finish(&kbuf_);
		return kbuf_;
	}

	bool beCoding = false;

	while (klen > 0) {
		if (SPECIAL_CHAR(*key) || !ACL_ISPRINT(*key)) {
			coder_.encode_update(key, 1, &kbuf_);
			beCoding = true;
		} else if (beCoding) {
			coder_.encode_finish(&kbuf_);
			coder_.reset();
			beCoding = false;
			kbuf_ << (char) *key;
		} else {
			kbuf_ << (char) *key;
		}
		key++;
		klen--;
	}

	if (beCoding) {
		coder_.encode_finish(&kbuf_);
	}

	return kbuf_;
}

bool memcache::error_happen(const char* line)
{
	if (strcasecmp(line, "ERROR") == 0) {
		return true;
	}
	if (strncasecmp(line, "CLIENT_ERROR", sizeof("CLIENT_ERROR") - 1) == 0) {
		ebuf_.format("%s", line);
		const char* ptr = line + sizeof("CLIENT_ERROR") - 1;
		if (*ptr == ' ' || *ptr == '\t') {
			ptr++;
		}
		enum_ = atoi(ptr);
		return true;
	}
	if (strncasecmp(line, "SERVER_ERROR", sizeof("SERVER_ERROR") - 1) == 0) {
		ebuf_.format("%s", line);
		const char* ptr = line + sizeof("SERVER_ERROR") - 1;
		if (*ptr == ' ' || *ptr == '\t') {
			ptr++;
		}
		enum_ = atoi(ptr);
		return true;
	}
	return false;
}

void memcache::property_list(void)
{
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
