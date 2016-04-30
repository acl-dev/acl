#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/url_coder.hpp"
#include "acl_cpp/http/HttpCookie.hpp"
#include "acl_cpp/http/http_header.hpp"
#endif

namespace acl
{

#define CP(x, y) ACL_SAFE_STRNCPY(x, y, sizeof(x))

http_header::http_header(dbuf_guard* dbuf /* = NULL */)
{
	if (dbuf != NULL)
	{
		dbuf_ = dbuf;
		dbuf_internal_ = NULL;
	}
	else
	{
		dbuf_internal_ = new dbuf_guard;
		dbuf_ = dbuf_internal_;
	}
	init();
}

http_header::http_header(const char* url, dbuf_guard* dbuf /* = NULL */)
{
	if (dbuf != NULL)
	{
		dbuf_ = dbuf;
		dbuf_internal_ = NULL;
	}
	else
	{
		dbuf_internal_ = new dbuf_guard;
		dbuf_ = dbuf_internal_;
	}
	init();
	if (url && *url)
		set_url(url);
}

http_header::http_header(int status, dbuf_guard* dbuf /* = NULL */)
{
	if (dbuf != NULL)
	{
		dbuf_ = dbuf;
		dbuf_internal_ = NULL;
	}
	else
	{
		dbuf_internal_ = new dbuf_guard;
		dbuf_ = dbuf_internal_;
	}
	init();
	set_status(status);
}

http_header::~http_header(void)
{
	clear();
	delete dbuf_internal_;
}

void http_header::init()
{
	is_request_ = true;
	url_ = NULL;
	method_ = HTTP_METHOD_GET;
	CP(method_s_, "GET");
	host_[0]  = 0;
	keep_alive_ = false;
	nredirect_ = 0; // 默认条件下不主动进行重定向
	accept_compress_ = true;
	status_ = 200;
	cgi_mode_ = false;
	range_from_ = -1;
	range_to_ = -1;
	range_total_ = -1;
	content_length_ = -1;
	chunked_transfer_ = false;
	transfer_gzip_ = false;
}

void http_header::clear()
{
	cookies_.clear();
	entries_.clear();
	params_.clear();
}

void http_header::reset()
{
	clear();
	init();

	if (dbuf_internal_)
		dbuf_internal_->dbuf_reset();
}

//////////////////////////////////////////////////////////////////////////
// 通用的函数

http_header& http_header::set_request_mode(bool onoff)
{
	is_request_ = onoff;
	return *this;
}

http_header& http_header::add_entry(const char* name, const char* value)
{
	if (name == NULL || *name == 0 || value == NULL || *value == 0)
		return *this;

	std::list<HTTP_HDR_ENTRY*>::iterator it = entries_.begin();
	for (; it != entries_.end(); ++it)
	{
		if (strcasecmp((*it)->name, name) == 0)
		{
			(*it)->value = dbuf_->dbuf_strdup(value);
			return *this;
		}
	}

	HTTP_HDR_ENTRY* entry = (HTTP_HDR_ENTRY*)
		dbuf_->dbuf_calloc(sizeof(HTTP_HDR_ENTRY));
	entry->name = dbuf_->dbuf_strdup(name);
	entry->value = dbuf_->dbuf_strdup(value);
	entries_.push_back(entry);
	return *this;
}

http_header& http_header::set_content_length(acl_int64 n)
{
	content_length_ = n;
	return *this;
}

http_header& http_header::set_chunked(bool on)
{
	chunked_transfer_ = on;
	return *this;
}

http_header& http_header::set_content_type(const char* value)
{
	add_entry("Content-Type", value);
	return *this;
}

http_header& http_header::set_keep_alive(bool on)
{
	keep_alive_ = on;
	return *this;
}

http_header& http_header::add_cookie(const char* name, const char* value,
	const char* domain /* = NULL */, const char* path /* = NULL */,
	time_t expires /* = 0 */)
{
	if (name == NULL || *name == 0 || value == NULL)
		return *this;

	HttpCookie* cookie = dbuf_->create<HttpCookie, const char*,
		const char*, dbuf_guard*>(name, value, dbuf_);

	if (domain && *domain)
		cookie->setDomain(domain);
	if (path && *path)
		cookie->setPath(path);
	if (expires > 0)
		cookie->setExpires(expires);
	cookies_.push_back(cookie);
	return *this;
}

http_header& http_header::add_cookie(const HttpCookie* in)
{
	if (in == NULL)
		return *this;

	HttpCookie* cookie = dbuf_->create<HttpCookie, const HttpCookie*,
		dbuf_guard*> (in, dbuf_);
	cookies_.push_back(cookie);
	return *this;
}

void http_header::date_format(char* out, size_t size, time_t t)
{
	const char* ptr = http_mkrfc1123(out, size, t);
	if (*ptr == 0)
		logger_error("gmtime error %s", last_serror());
}

bool http_header::is_request() const
{
	return is_request_;
}

void http_header::build_common(string& buf) const
{
	if (!entries_.empty())
	{
		std::list<HTTP_HDR_ENTRY*>::const_iterator it = entries_.begin();
		for (; it != entries_.end(); ++it)
			buf << (*it)->name << ": " << (*it)->value << "\r\n";
	}

	if (chunked_transfer_)
		buf << "Transfer-Encoding: " << "chunked\r\n";
	else if (content_length_ >= 0)
	{
		char length[64];
#ifdef ACL_WINDOWS
# if _MSC_VER >= 1500
		_snprintf_s(length, sizeof(length), sizeof(length),
			"%I64d", content_length_);
# else
		_snprintf(length, sizeof(length), "%I64d", content_length_);
#endif
#else
		snprintf(length, sizeof(length), "%lld", content_length_);
#endif
		buf << "Content-Length: " << length << "\r\n";
	}

	if (is_request_ == false && cgi_mode_)
		return;
	if (keep_alive_)
		buf << "Connection: " << "Keep-Alive\r\n";
	else
		buf << "Connection: " << "Close\r\n";
}

//////////////////////////////////////////////////////////////////////////
// 与 HTTP 请求头相关的函数

http_header& http_header::set_url(const char* url, bool encoding /* = true */)
{
	acl_assert(url && *url);

	is_request_ = true;
	size_t len = strlen(url);

	// 多分配两个字节：'\0' 及可能添加的 '/'
	url_ = (char*) dbuf_->dbuf_alloc(len + 2);
	memcpy(url_, url, len);
	url_[len] = 0;

	char* ptr;
	if (strncasecmp(url_, "http://", sizeof("http://") - 1) == 0)
		ptr = url_ + sizeof("http://") - 1;
	else if (strncasecmp(url_, "https://", sizeof("https://") - 1) == 0)
		ptr = url_+ sizeof("https://") -1;
	else
		ptr = url_;

	char* params, *slash;

	// 开始提取 host 字段

	// 当 url 中只有相对路径时
	if (ptr == url_)
	{
		if (encoding)
			params = strchr(ptr, '?');
		else
			params = NULL;
	}

	// 当 url 为绝对路径时
	else if ((slash = strchr(ptr, '/')) != NULL && slash > ptr)
	{
		size_t n = slash - ptr + 1;
		if (n > sizeof(host_))
			n = sizeof(host_);

		// 添加主机地址
		ACL_SAFE_STRNCPY(host_, ptr, n);

		if (encoding)
			params = strchr(slash, '?');
		else
			params = NULL;
	}

	// 当 url 为绝对路径且主机地址后没有 '/'
	else
	{
		// 这是安全的，因为在前面给 url_ 分配内存时多了一个字节
		if (slash == NULL)
		{
			url_[len] = '/';
			url_[len + 1] = 0;
		}

		if (encoding)
			params = strchr(ptr, '?');
		else
			params = NULL;
	}

	if (params == NULL)
		return *this;

	*params++ = 0;
	if (*params == 0)
		return *this;

	url_coder coder;
	coder.decode(params);
	const std::vector<URL_NV*>& tokens = coder.get_params();
	std::vector<URL_NV*>::const_iterator cit = tokens.begin();
	for (; cit != tokens.end(); ++cit)
		add_param((*cit)->name, (*cit)->value);

	return *this;
}

http_header& http_header::set_host(const char* value)
{
	if (value && *value)
		CP(host_, value);
	return *this;
}

http_header& http_header::set_method(http_method_t method)
{
	method_ = method;

	switch(method_)
	{
	case HTTP_METHOD_GET:
		CP(method_s_, "GET");
		break;
	case HTTP_METHOD_POST:
		CP(method_s_, "POST");
		break;
	case HTTP_METHOD_PUT:
		CP(method_s_, "PUT");
		break;
	case HTTP_METHOD_CONNECT:
		CP(method_s_, "CONNECT");
		break;
	case HTTP_METHOD_PURGE:
		CP(method_s_, "PURGE");
		break;
	case  HTTP_METHOD_DELETE:
		CP(method_s_, "DELETE");
		break;
	case  HTTP_METHOD_HEAD:
		CP(method_s_, "HEAD");
		break;
	case  HTTP_METHOD_OPTION:
		CP(method_s_, "OPTIONS");
		break;
	case HTTP_METHOD_PROPFIND:
		CP(method_s_, "PROPFIND");
		break;
	default:
		CP(method_s_, "UNKNOWN");
		break;
	}
	return *this;
}

http_header& http_header::set_method(const char* method)
{
	if (strcasecmp(method, "GET") == 0)
		method_ = HTTP_METHOD_GET;
	else if (strcasecmp(method, "POST") == 0)
		method_ = HTTP_METHOD_POST;
	else if (strcasecmp(method, "PUT") == 0)
		method_ = HTTP_METHOD_PUT;
	else if (strcasecmp(method, "CONNECT") == 0)
		method_ = HTTP_METHOD_CONNECT;
	else if (strcasecmp(method, "PURGE") == 0)
		method_ = HTTP_METHOD_PURGE;
	else if (strcasecmp(method, "DELETE") == 0)
		method_ = HTTP_METHOD_DELETE;
	else if (strcasecmp(method, "HEAD") == 0)
		method_ = HTTP_METHOD_HEAD;
	else if (strcasecmp(method, "OPTIONS") == 0)
		method_ = HTTP_METHOD_OPTION;
	else if (strcasecmp(method, "PROPFIND") == 0)
		method_ = HTTP_METHOD_PROPFIND;
	else if (*method != 0)
		method_ = HTTP_METHOD_OTHER;
	else
		method_ = HTTP_METHOD_UNKNOWN;

	CP(method_s_, method);

	return *this;
}

http_method_t http_header::get_method(string* buf /* = NULL */) const
{
	if (buf)
		*buf = method_s_;
	return method_;
}

http_header& http_header::set_range_total(http_off_t total)
{
	range_total_ = total;
	return *this;
}

http_header& http_header::set_range(http_off_t from, http_off_t to)
{
	range_from_ = from;
	if (to >= from)
		range_to_ = to;
	else
		range_to_ = -1;
	return *this;
}

void http_header::get_range(http_off_t* from, http_off_t* to)
{
	if (from)
		*from = range_from_;
	if (to)
		*to = range_to_;
}

http_header& http_header::accept_gzip(bool on)
{
	accept_compress_ = on;
	return *this;
}

http_header& http_header::add_param(const char* name, const char* value)
{
	if (name == NULL || *name == 0)
		return *this;

	std::list<HTTP_PARAM*>::iterator it = params_.begin();
	for (; it != params_.end(); ++it)
	{
		if (strcasecmp((*it)->name, name) == 0)
		{
			if (value)
				(*it)->value = dbuf_->dbuf_strdup(value);
			else
				(*it)->value = NULL;
			return *this;
		}
	}

	HTTP_PARAM* param = (HTTP_PARAM*) dbuf_->dbuf_calloc(sizeof(HTTP_PARAM));
	param->name = dbuf_->dbuf_strdup(name);
	if (value)
		param->value = dbuf_->dbuf_strdup(value);
	else
		param->value = NULL;
	params_.push_back(param);
	return *this;
}

http_header& http_header::add_int(const char* name, short value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%d", value);
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, unsigned short value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%u", value);
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, int value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%d", value);
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, unsigned int value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%u", value);
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, long value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%ld", value);
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, unsigned long value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%lu", value);
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, acl_int64 value)
{
	char buf[32];
#ifdef ACL_WINDOWS
	safe_snprintf(buf, sizeof(buf), "%I64d", value);
#else
	safe_snprintf(buf, sizeof(buf), "%lld", value);
#endif
	return add_param(name, buf);
}

http_header& http_header::add_int(const char* name, acl_uint64 value)
{
	char buf[32];
#ifdef ACL_WINDOWS
	safe_snprintf(buf, sizeof(buf), "%I64u", value);
#else
	safe_snprintf(buf, sizeof(buf), "%llu", value);
#endif
	return add_param(name, buf);
}

http_header& http_header::add_format(const char* name, const char* fmt, ...)
{
	string buf(128);
	va_list ap;
	va_start(ap, fmt);
	buf.vformat(fmt, ap);
	va_end(ap);
	return add_param(name, buf.c_str());
}

bool http_header::build_request(string& buf) const
{
	if (url_ == NULL || *url_ == 0)
	{
		logger_error("url empty");
		return (false);
	}

	buf.format("%s %s", method_s_, url_);

	if (!params_.empty())
	{
		buf << '?';
		acl::string tmp;
		int i = 0;
		std::list<HTTP_PARAM*>::const_iterator it = params_.begin();
		for (; it != params_.end(); ++it)
		{
			if (i > 0)
				buf += '&';
			else
				i++;

			// 需要对参数进行 URL 编码

			tmp.url_encode((*it)->name);
			buf += tmp.c_str();

			// 允许参数值为空指针
			if ((*it)->value == NULL)
				continue;

			buf += '=';

			// 允许参数值为空串
			if (*((*it)->value) == 0)
				continue;

			tmp.url_encode((*it)->value);
			buf += tmp.c_str();
		}
	}
	buf += " HTTP/1.1\r\n";
	if (accept_compress_)
		// 因为目前的 zlib_stream 仅支持于此
		buf += "Accept-Encoding: gzip\r\n";

	if (host_[0] != 0)
		buf.format_append("Host: %s\r\n", host_);

	if (!cookies_.empty())
	{
		buf += "Cookie: ";
		std::list<HttpCookie*>::const_iterator it = cookies_.begin();
		for (; it != cookies_.end(); ++it)
		{
			if (it != cookies_.begin())
				buf += "; ";
			buf << (*it)->getName() << "=" << (*it)->getValue();
		}
		buf += "\r\n";
	}

	// 添加分段请求字段
	if (range_from_ >= 0)
	{
		buf << "Range: bytes=" << range_from_ << '-';
		if (range_to_ >= range_from_)
			buf << range_to_;
		buf += "\r\n";
	}

	build_common(buf);
	buf += "\r\n";

	return (true);
}

bool http_header::redirect(const char* url)
{
	if (url == NULL || *url == 0)
	{
		logger_error("url null");
		return false;
	}

	size_t n = 0;

	// url: http[s]://xxx.xxx.xxx/xxx or /xxx
	if (strncasecmp(url, "http://", sizeof("http://") - 1) == 0)
		n = sizeof("http://") - 1;
	else if (strncasecmp(url, "https://", sizeof("https://") - 1) == 0)
		n = sizeof("https://") - 1;
	if (url_)
		url_ = NULL;

	if (n > 0)
	{
		url += n;
		char* ptr = dbuf_->dbuf_strdup(url);
		char* p = strchr(ptr, '/');
		if (p)
			*p = 0;
		if (*ptr == 0)
		{
			logger_error("invalid url(%s)", url);
			return false;
		}
		set_host(ptr);
		if (*(p + 1))
		{
			*p = '/';
			url_ = p;
		}
		else
			url_ = dbuf_->dbuf_strdup("/");
	}
	else
		url_ = dbuf_->dbuf_strdup(url);

	return true;
}

http_header& http_header::set_redirect(unsigned int n /* = 5 */)
{
	nredirect_ = n;
	return *this;
}

unsigned int http_header::get_redirect() const
{
	return (nredirect_);
}

//////////////////////////////////////////////////////////////////////////
// 与 HTTP 响应头相关的函数

typedef struct HTTP_STATUS {
	int   status;
	const char *title;
} HTTP_STATUS;

static HTTP_STATUS __1xx_tab[] = {
	/* 1xx */
	{ 100, "Continue" },
	{ 101, "Switching Protocols" },
	{ 102, "Processing" },  /* RFC2518 section 10.1 */
};

static HTTP_STATUS __2xx_tab[] = {
	/* 2xx */
	{ 200, "OK" },
	{ 201, "Created" },
	{ 202, "Accepted" },
	{ 203, "Non Authoritative Information" },
	{ 204, "No Content" },
	{ 205, "Reset Content" },
	{ 206, "Partial Content" },
	{ 207, "Multi Status" },  /* RFC2518 section 10.2 */
};

static HTTP_STATUS __3xx_tab[] = {
	/* 3xx */
	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Moved Temporarily" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 307, "Temporary Redirect" },
};

static HTTP_STATUS __4xx_tab[] = {
	/* 4xx */
	{ 400, "Bad Request" },
	{ 401, "Authorization Required" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Time-out" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request-URI Too Large" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Requested Range Not Satisfiable" },
	{ 417, "Expectation Failed" },
	{ 418, NULL },
	{ 419, NULL },
	{ 420, NULL },
	{ 421, NULL },
	{ 422, "Unprocessable Entity" },
	{ 423, "Locked" },
	{ 424, "Failed Dependency" },
	{ 425, "No code" },
	{ 426, "Upgrade Required" },
};

static HTTP_STATUS __5xx_tab[] = {
	/* 5xx */
	{ 500, "Internal Server Error" },
	{ 501, "Method Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Temporarily Unavailable" },
	{ 504, "Gateway Time-out" },
	{ 505, "HTTP Version Not Supported" },
	{ 506, "Variant Also Negotiates" },
	{ 507, "Insufficient Storage" },
	{ 508, NULL },
	{ 509, NULL },
	{ 510, "Not Extended" },
};

typedef struct STATUS_MAP {
	int   level;
	size_t size;
	HTTP_STATUS *hs;
} STATUS_MAP;

static STATUS_MAP __maps[] = {
	{ 100, sizeof(__1xx_tab) / sizeof(HTTP_STATUS), __1xx_tab },
	{ 200, sizeof(__2xx_tab) / sizeof(HTTP_STATUS), __2xx_tab },
	{ 300, sizeof(__3xx_tab) / sizeof(HTTP_STATUS), __3xx_tab },
	{ 400, sizeof(__4xx_tab) / sizeof(HTTP_STATUS), __4xx_tab },
	{ 500, sizeof(__5xx_tab) / sizeof(HTTP_STATUS), __5xx_tab }
};

static const char *__unknown_status = "unknow status";

static const char *http_status(int status)
{
	size_t   i, pos;

	i = status / 100;
	if (i < 1 || i > 5)
		return (__unknown_status);

	i--;
	pos = status - __maps[i].level;

	if (pos >= __maps[i].size)
		return (__unknown_status);

	if (__maps[i].hs[pos].title == NULL)
		return (__unknown_status);

	return (__maps[i].hs[pos].title);
}

bool http_header::build_response(string& out) const
{
	out.clear();

	if (status_ > 0)
	{
		if (cgi_mode_)
			out.format("STATUS: %d\r\n", status_);
		else
		{
			out = "HTTP/1.1 ";
			out << status_ << " " << http_status(status_) << "\r\n";

			time_t now = time(NULL);
			char buf[64];
			date_format(buf, sizeof(buf), now);
			out << "Date: " << buf << "\r\n";
			out << "Server: acl\r\n";
		}
	}

	if (!cookies_.empty())
	{
		std::list<HttpCookie*>::const_iterator it = cookies_.begin();
		for (; it != cookies_.end(); ++it)
		{
			out.format_append("Set-Cookie: %s=%s",
				(*it)->getName(), (*it)->getValue());
			const std::list<HTTP_PARAM*>& params = (*it)->getParams();
			std::list<HTTP_PARAM*>::const_iterator cit = params.begin();
			for (; cit != params.end(); ++ cit)
			{
				out.format_append("; %s=%s",
					(*cit)->name, (*cit)->value);
			}
			out << "\r\n";
		}
	}

	// 添加分段响应字段
	if (range_from_ >= 0 && range_to_ >= range_from_ && range_total_ > 0)
		out << "Content-Range: bytes=" << range_from_ << '-'
			<< range_to_ << '/' << range_total_ << "\r\n";

	// 如果是 gzip 压缩数据，当非 chunked 传输时，必须取消 Content-Length 字段，
	// 同时禁止保持长连接，即： Connection: close
	if (transfer_gzip_)
	{
		out << "Content-Encoding: gzip\r\n";

		if (!chunked_transfer_ && keep_alive_)
			const_cast<http_header*>(this)->keep_alive_ = false;
		if (content_length_ > 0)
			const_cast<http_header*>(this)->content_length_ = -1;
	}

	build_common(out);
	out << "\r\n";
	return true;
}

http_header& http_header::set_status(int status)
{
	status_ = status;
	is_request_ = false;
	return *this;
}

http_header& http_header::set_cgi_mode(bool on)
{
	cgi_mode_ = on;
	if (cgi_mode_)
		is_request_ = false;
	return *this;
}

http_header& http_header::set_transfer_gzip(bool on)
{
	transfer_gzip_ = on;
	if (transfer_gzip_)
		is_request_ = false;
	return *this;
}

}  // namespace acl end
