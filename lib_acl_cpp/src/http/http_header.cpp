#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/http/HttpCookie.hpp"
#include "acl_cpp/http/http_header.hpp"

namespace acl
{

http_header::http_header(void)
{
	init();
}

http_header::http_header(const char* url)
{
	init();
	if (url && *url)
		set_url(url);
}

http_header::http_header(int status)
{
	init();
	set_status(status);
}

http_header::~http_header(void)
{
	clear();
}

void http_header::init()
{
	is_request_ = true;
	url_ = NULL;
	method_ = HTTP_METHOD_GET;
	keep_alive_ = false;
	nredirect_ = 0; // 默认条件下不主动进行重定向
	accept_compress_ = true;
	status_ = 200;
	cgi_mode_ = false;
	range_from_ = -1;
	range_to_ = -1;
}

void http_header::clear()
{
	if (url_)
		acl_myfree(url_);

	std::list<HttpCookie*>::iterator cookies_it = cookies_.begin();
	for (; cookies_it != cookies_.end(); ++cookies_it)
		(*cookies_it)->destroy();
	cookies_.clear();

	std::list<HTTP_HDR_ENTRY*>::iterator entries_it = entries_.begin();
	for (; entries_it != entries_.end(); ++entries_it)
	{
		acl_myfree((*entries_it)->name);
		acl_myfree((*entries_it)->value);
		acl_myfree(*entries_it);
	}
	entries_.clear();

	std::list<HTTP_PARAM*>::iterator params_it = params_.begin();
	for (; params_it != params_.end(); ++params_it)
	{
		acl_myfree((*params_it)->name);
		acl_myfree((*params_it)->value);
		acl_myfree(*params_it);
	}
	params_.clear();
}

void http_header::reset()
{
	clear();
	init();
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
			acl_myfree((*it)->value);
			(*it)->value = acl_mystrdup(value);
			return *this;
		}
	}

	HTTP_HDR_ENTRY* entry = (HTTP_HDR_ENTRY*)
		acl_mycalloc(1, sizeof(HTTP_HDR_ENTRY));
	entry->name = acl_mystrdup(name);
	entry->value = acl_mystrdup(value);
	entries_.push_back(entry);
	return *this;
}

http_header& http_header::set_content_length(acl_int64 n)
{
	char buf[64];
#ifdef WIN32
	_snprintf(buf, sizeof(buf), "%I64d", n);
#else
	snprintf(buf, sizeof(buf), "%lld", n);
#endif
	add_entry("Content-Length", buf);
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

	HttpCookie* cookie = NEW HttpCookie(name, value);
	if (domain && *domain)
		cookie->setDomain(domain);
	if (path && *path)
		cookie->setPath(path);
	if (expires > 0)
		cookie->setExpires(expires);
	cookies_.push_back(cookie);
	return *this;
}

http_header& http_header::add_cookie(HttpCookie* cookie)
{
	if (cookie)
		cookies_.push_back(cookie);
	return *this;
}

#define RFC1123_STRFTIME "%a, %d %b %Y %H:%M:%S GMT"

void http_header::date_format(char* out, size_t size, time_t t)
{
	struct tm *gmt = gmtime(&t);
	strftime(out, size - 1, RFC1123_STRFTIME, gmt);
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

	if (is_request_ == false && cgi_mode_)
		return;
	if (keep_alive_)
		buf << "Connection: " << "Keep-Alive\r\n";
	else
		buf << "Connection: " << "Close\r\n";
}

//////////////////////////////////////////////////////////////////////////
// 与 HTTP 请求头相关的函数

http_header& http_header::set_url(const char* url)
{
	acl_assert(url && *url);
	if (url_)
		acl_myfree(url_);
	url_ = acl_mystrdup(url);
	is_request_ = true;
	return *this;
}

http_header& http_header::set_host(const char* value)
{
	add_entry("Host", value);
	return *this;
}

http_header& http_header::set_method(http_method_t method)
{
	method_ = method;
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
	if (name == NULL || *name == 0 || value == NULL || *value == 0)
		return *this;

	std::list<HTTP_PARAM*>::iterator it = params_.begin();
	for (; it != params_.end(); ++it)
	{
		if (strcasecmp((*it)->name, name) == 0)
		{
			acl_myfree((*it)->value);
			(*it)->value = acl_mystrdup(value);
			return *this;
		}
	}

	HTTP_PARAM* param = (HTTP_PARAM*) acl_mycalloc(1, sizeof(HTTP_PARAM));
	param->name = acl_mystrdup(name);
	param->value = acl_mystrdup(value);
	params_.push_back(param);
	return *this;
}

bool http_header::build_request(string& buf) const
{
	if (url_ == NULL || *url_ == 0)
	{
		logger_error("url empty");
		return (false);
	}

	buf.clear();
	switch(method_)
	{
	case HTTP_METHOD_GET:
		buf = "GET ";
		break;
	case HTTP_METHOD_POST:
		buf = "POST ";
		break;
	case HTTP_METHOD_PUT:
		buf = "PUT ";
		break;
	case HTTP_METHOD_CONNECT:
		buf = "CONNECT";
		break;
	case HTTP_METHOD_PURGE:
		buf = "PURGE";
		break;
	default:
		logger_error("unknown method(%d)", (int) method_);
		return (false);
	}

	buf << url_;
	if (!params_.empty())
	{
		buf << '?';
		acl::string tmp;
		int  i = 0;
		std::list<HTTP_PARAM*>::const_iterator it = params_.begin();
		for (; it != params_.end(); ++it)
		{
			if (!(*it)->name || !(*it)->value)
				continue;

			if (i > 0)
				buf << '&';

			// 需要对参数进行 URL 编码

			tmp.url_encode((*it)->name);
			buf << tmp.c_str() << '=';
			tmp.url_encode((*it)->value);
			buf << tmp.c_str();
			i++;
		}
	}
	buf << " HTTP/1.1\r\n";
	if (accept_compress_)
		// 因为目前的 zlib_stream 仅支持于此
		buf << "Accept-Encoding: gzip\r\n";


	if (!cookies_.empty())
	{
		buf << "Cookie: ";
		std::list<HttpCookie*>::const_iterator it = cookies_.begin();
		for (; it != cookies_.end(); ++it)
		{
			if (it != cookies_.begin())
				buf << "; ";
			buf << (*it)->getName() << "=" << (*it)->getValue();
		}
		buf << "\r\n";
	}

	// 添加分段请求字段
	if (range_from_ >= 0)
	{
		buf << "Range: bytes=" << range_from_ << '-';
		if (range_to_ >= range_from_)
			buf << range_to_;
		buf << "\r\n";
	}

	build_common(buf);
	buf << "\r\n";

	return (true);
}

bool http_header::redirect(const char* url)
{
	if (url == NULL || *url == 0)
	{
		logger_error("url null");
		return (false);
	}

	size_t n = 0;

	// url: http[s]://xxx.xxx.xxx/xxx or /xxx
	if (strncasecmp(url, "http://", sizeof("http://") - 1) == 0)
		n = sizeof("http://") - 1;
	else if (strncasecmp(url, "https://", sizeof("https://") - 1) == 0)
		n = sizeof("https://") - 1;
	if (url_)
	{
		acl_myfree(url_);
		url_ = NULL;
	}

	if (n > 0)
	{
		url += n;
		char* ptr = acl_mystrdup(url);
		char* p = strchr(ptr, '/');
		if (p)
			*p = 0;
		if (*ptr == 0)
		{
			logger_error("invalid url(%s)", url);
			acl_myfree(ptr);
			return (false);
		}
		set_host(ptr);
		if (*(p + 1))
		{
			*p = '/';
			url_ = acl_mystrdup(p);
		}
		else
			url_ = acl_mystrdup("/");
		acl_myfree(ptr);
	}
	else
		url_ = acl_mystrdup(url);

	return (true);
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

}  // namespace acl end
