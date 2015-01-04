#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/session/session.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpCookie.hpp"
#include "acl_cpp/http/http_client.hpp"
#include "acl_cpp/http/http_mime.hpp"
#include "acl_cpp/http/HttpSession.hpp"
#include "acl_cpp/http/HttpServletResponse.hpp"
#include "acl_cpp/http/HttpServletRequest.hpp"

#define SKIP_SPACE(x) { while (*x == ' ' || *x == '\t') x++; }

namespace acl
{

HttpServletRequest::HttpServletRequest(HttpServletResponse& res, session& store,
	socket_stream& stream, const char* local_charset /* = NULL */,
	bool body_parse /* = true */, int body_limit /* = 102400 */)
: req_error_(HTTP_REQ_OK)
, res_(res)
, store_(store)
, http_session_(NULL)
, stream_(stream)
, body_parse_(body_parse)
, body_limit_(body_limit)
, cookies_inited_(false)
, client_(NULL)
, method_(HTTP_METHOD_UNKNOWN)
, request_type_(HTTP_REQUEST_NORMAL)
, mime_(NULL)
, readHeaderCalled_(false)
{
	ACL_SAFE_STRNCPY(cookie_name_, "ACL_SESSION_ID", sizeof(cookie_name_));
	ACL_VSTREAM* in = stream.get_vstream();
	if (in == ACL_VSTREAM_IN)
		cgi_mode_ = true;
	else
		cgi_mode_ = false;
	if (local_charset && *local_charset)
		safe_snprintf(localCharset_, sizeof(localCharset_),
			"%s", local_charset);
	else
		localCharset_[0] = 0;
	rw_timeout_ = 60;
}

HttpServletRequest::~HttpServletRequest(void)
{
	delete client_;
	std::vector<HttpCookie*>::iterator it = cookies_.begin();
	for (; it != cookies_.end(); ++it)
		(*it)->destroy();
	std::vector<HTTP_PARAM*>::iterator it1 = params_.begin();
	for (; it1 != params_.end(); ++it1)
	{
		acl_myfree((*it1)->name);
		acl_myfree((*it1)->value);
		acl_myfree(*it1);
	}
	delete mime_;
	delete http_session_;
}

http_method_t HttpServletRequest::getMethod(void) const
{
	// HttpServlet 类对象的 doRun 运行时 readHeader 必须先被调用，
	// 而 HttpSevlet 类在初始化请求时肯定会调用 getMethod 方法，
	// 所以在此函数中触发 readHeader 方法比较好，这样一方面可以
	// 将 readHeader 方法隐藏起来，免得用户误调用；另一方面，又
	// 能保证 readHeader 肯定会被调用；同时又不必把 HttpServlet
	// 类声明本类的友元类

	if (readHeaderCalled_ == false)
		const_cast<HttpServletRequest*>(this)->readHeader();
	return method_;
}

static void add_cookie(std::vector<HttpCookie*>& cookies, char* data)
{
	SKIP_SPACE(data);
	if (*data == 0 || *data == '=')
		return;
	char* ptr = strchr(data, '=');
	if (ptr == NULL)
		return;
	*ptr++ = 0;
	SKIP_SPACE(ptr)
	if (*ptr == 0)
		return;
	char* end = ptr + strlen(ptr) - 1;
	while (end > ptr && (*end == ' ' || *end == '\t'))
		*end-- = 0;
	HttpCookie* cookie = NEW HttpCookie(data, ptr);
	cookies.push_back(cookie);
}

void HttpServletRequest::setCookie(const char* name, const char* value)
{
	if (name == NULL || *name == 0 || value == NULL)
		return;
	HttpCookie* cookie = NEW HttpCookie(name, value);
	cookies_.push_back(cookie);
}

const std::vector<HttpCookie*>& HttpServletRequest::getCookies(void) const
{
	if (cookies_inited_)
		return cookies_;

	// 设置标记表明已经分析过cookie了，以便于下次重复调用时节省时间
	const_cast<HttpServletRequest*>(this)->cookies_inited_ = true;

	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("HTTP_COOKIE");
		if (ptr == NULL || *ptr == 0)
			return cookies_;
		ACL_ARGV* argv = acl_argv_split(ptr, ";");
		ACL_ITER iter;
		acl_foreach(iter, argv)
		{
			add_cookie(const_cast<HttpServletRequest*>
				(this)->cookies_, (char*) iter.data);
		}
		acl_argv_free(argv);
		return cookies_;
	}

	if (client_ == NULL)
		return cookies_;
	const HTTP_HDR_REQ* req = client_->get_request_head(NULL);
	if (req == NULL)
		return cookies_;
	if (req->cookies_table == NULL)
		return cookies_;

	const char* name, *value;
	HttpCookie* cookie;
	ACL_HTABLE_ITER iter;

	// 遍历 HTTP  请求头中的 cookie 项
	acl_htable_foreach(iter, req->cookies_table)
	{
		name = acl_htable_iter_key(iter);
		value = (char*) acl_htable_iter_value(iter);
		if (name == NULL || *name == 0
			|| value == NULL || *value == 0)
		{
			continue;
		}
		// 创建 cookie 对象并将之加入数组中
		cookie = NEW HttpCookie(name, value);
		const_cast<HttpServletRequest*>
			(this)->cookies_.push_back(cookie);
	}

	return cookies_;
}

const char* HttpServletRequest::getCookieValue(const char* name) const
{
	(void) getCookies();

	std::vector<HttpCookie*>::const_iterator cit = cookies_.begin();
	for (; cit != cookies_.end(); ++cit)
	{
		const char* ptr = (*cit)->getName();
		if (ptr && strcmp(name, ptr) == 0)
			return (*cit)->getValue();
	}
	return NULL;
}

const char* HttpServletRequest::getHeader(const char* name) const
{
	if (cgi_mode_)
		return acl_getenv(name);

	if (client_ == NULL)
		return NULL;
	return client_->header_value(name);
}

const char* HttpServletRequest::getQueryString(void) const
{
	if (cgi_mode_)
		return acl_getenv("QUERY_STRING");
	if (client_ == NULL)
		return NULL;
	return client_->request_params();
}

const char* HttpServletRequest::getPathInfo(void) const
{
	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("SCRIPT_NAME");
		if (ptr != NULL)
			return ptr;
		ptr = acl_getenv("PATH_INFO");
		return ptr;
	}
	if (client_ == NULL)
		return NULL;
	return client_->request_path();
}

const char* HttpServletRequest::getRequestUri(void) const
{
	if (cgi_mode_)
		return acl_getenv("REQUEST_URI");
	if (client_ == NULL)
		return NULL;
	else
		return client_->request_url();
}

HttpSession& HttpServletRequest::getSession(bool create /* = true */,
	const char* sid_in /* = NULL */)
{
	if (http_session_ != NULL)
		return *http_session_;

	http_session_ = NEW HttpSession(store_);
	const char* sid;

	if ((sid = getCookieValue(cookie_name_)) != NULL)
		store_.set_sid(sid);
	else if (create)
	{
		// 获得唯一 ID 标识符
		sid = store_.get_sid();
		// 生成 cookie 对象，并分别向请求对象和响应对象添加 cookie
		HttpCookie* cookie = NEW HttpCookie(cookie_name_, sid);
		res_.addCookie(cookie);
		setCookie(cookie_name_, sid);
	}
	else if (sid_in != NULL && *sid_in != 0)
	{
		store_.set_sid(sid_in);
		// 生成 cookie 对象，并分别向请求对象和响应对象添加 cookie
		HttpCookie* cookie = NEW HttpCookie(cookie_name_, sid_in);
		res_.addCookie(cookie);
		setCookie(cookie_name_, sid_in);
	}

	return *http_session_;
}

acl_int64 HttpServletRequest::getContentLength(void) const
{
	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("CONTENT_LENGTH");
		if (ptr == NULL)
			return -1;
		return acl_atoui64(ptr);
	}
	if (client_ == NULL)
		return -1;
	return client_->body_length();
}

const char* HttpServletRequest::getContentType(bool part /* = true */) const
{
	if (part)
		return content_type_.get_ctype();
	if (cgi_mode_)
		return acl_getenv("CONTENT_TYPE");
	return client_->header_value("Content-Type");
}

const char* HttpServletRequest::getCharacterEncoding(void) const
{
	return content_type_.get_charset();
}

const char* HttpServletRequest::getLocalCharset(void) const
{
	return localCharset_[0] ? localCharset_ : NULL;
}

const char* HttpServletRequest::getLocalAddr(void) const
{
	if (cgi_mode_)
		return NULL;

	if (client_ == NULL)
		return NULL;
	const char* ptr = client_->get_stream().get_local();
	if (*ptr == 0)
		return NULL;
	safe_snprintf(const_cast<HttpServletRequest*>(this)->localAddr_,
		sizeof(localAddr_), "%s", ptr);
	char* p = (char*) strchr(localAddr_, ':');
	if (p)
		*p = 0;
	return localAddr_;
}

unsigned short HttpServletRequest::getLocalPort(void) const
{
	if (cgi_mode_)
		return 0;

	if (client_ == NULL)
		return 0;
	const char* ptr = client_->get_stream().get_local();
	if (*ptr == 0)
		return 0;
	char* p = (char*) strchr(ptr, ':');
	if (p == NULL || *(++p) == 0)
		return 0;

	return atoi(p);
}

const char* HttpServletRequest::getRemoteAddr(void) const
{
	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("REMOTE_ADDR");
		if (ptr && *ptr)
			return ptr;
		logger_warn("no REMOTE_ADDR from acl_getenv");
		return NULL;
	}
	if (client_ == NULL)
		return NULL;
	const char* ptr = client_->get_stream().get_peer();
	if (*ptr == 0)
	{
		logger_warn("get_peer return empty string");
		return NULL;
	}
	safe_snprintf(const_cast<HttpServletRequest*>(this)->remoteAddr_,
		sizeof(remoteAddr_), "%s", ptr);
	char* p = (char*) strchr(remoteAddr_, ':');
	if (p)
		*p = 0;
	return remoteAddr_;
}

unsigned short HttpServletRequest::getRemotePort(void) const
{
	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("REMOTE_PORT");
		if (ptr && *ptr)
			return atoi(ptr);
		logger_warn("no REMOTE_PORT from acl_getenv");
		return 0;
	}
	if (client_ == NULL)
		return 0;
	const char* ptr = client_->get_stream().get_peer(true);
	if (*ptr == 0)
	{
		logger_warn("get_peer return empty string");
		return 0;
	}
	char* port = (char*) strchr(ptr, ':');
	if (port == NULL || *(++port) == 0)
	{
		logger_warn("no port in addr: %s", ptr);
		return 0;
	}

	return atoi(port);
}

const char* HttpServletRequest::getParameter(const char* name) const
{
	std::vector<HTTP_PARAM*>::const_iterator cit = params_.begin();
	for (; cit != params_.end(); ++cit)
	{
		if (strcmp((*cit)->name, name) == 0)
			return (*cit)->value;
	}

	// 如果是 MIME 格式，则尝试从 mime_ 对象中查询参数
	if (mime_ == NULL)
		return NULL;
	const http_mime_node* node = mime_->get_node(name);
	if (node == NULL)
		return NULL;
	return node->get_value();
}

http_mime* HttpServletRequest::getHttpMime(void) const
{
	return mime_;
}

http_request_t HttpServletRequest::getRequestType(void) const
{
	return request_type_;
}

istream& HttpServletRequest::getInputStream(void) const
{
	return stream_;
}

void HttpServletRequest::parseParameters(const char* str)
{
	const char* requestCharset = getCharacterEncoding();
	charset_conv conv;
	string buf;
	ACL_ARGV* tokens = acl_argv_split(str, "&");
	ACL_ITER iter;
	acl_foreach(iter, tokens)
	{
		char* name = (char*) iter.data;
		char* value = strchr(name, '=');
		if (value == NULL || *(value + 1) == 0)
			continue;
		*value++ = 0;
		name = acl_url_decode(name);
		value = acl_url_decode(value);
		HTTP_PARAM* param = (HTTP_PARAM*) acl_mycalloc(1,
			sizeof(HTTP_PARAM));
		if (localCharset_[0] != 0 && requestCharset
			&& strcasecmp(requestCharset, localCharset_))
		{
			buf.clear();
			if (conv.convert(requestCharset, localCharset_,
				name, strlen(name), &buf) == true)
			{
				param->name = acl_mystrdup(buf.c_str());
				acl_myfree(name);
			}
			else
				param->name = name;

			buf.clear();
			if (conv.convert(requestCharset, localCharset_,
				value, strlen(value), &buf) == true)
			{
				param->value = acl_mystrdup(buf.c_str());
				acl_myfree(value);
			}
			else
				param->value = value;
		}
		else
		{
			param->name = name;
			param->value = value;
		}
		params_.push_back(param);
	}

	acl_argv_free(tokens);
}

// Content-Type: application/x-www-form-urlencoded; charset=utf-8
// Content-Type: multipart/form-data; boundary=---------------------------41184676334
// Content-Type: application/octet-stream

bool HttpServletRequest::readHeader(void)
{
	acl_assert(readHeaderCalled_ == false);
	readHeaderCalled_ = true;

	const char* method;

	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("CONTENT_TYPE");
		if (ptr && *ptr)
			content_type_.parse(ptr);

		// 必须是后获得 method，因为 acl_getenv 内部的内存区用的
		// 是线程局部变量，该内存区在同一线程中会被重复使用，这样
		// 后获得 method 可以保证 method 可以由下面的过程确定具体
		// 的请求方法
		method = acl_getenv("REQUEST_METHOD");
	}
	else
	{
		client_ = NEW http_client(&stream_, rw_timeout_);
		if (client_->read_head() == false)
		{
			req_error_ = HTTP_REQ_ERR_IO;
			return false;
		}
		method = client_->request_method();
		const char* ptr = client_->header_value("Content-Type");
		if (ptr && *ptr)
			content_type_.parse(ptr);
	}

	if (method == NULL || *method == 0)
	{
		req_error_ = HTTP_REQ_ERR_METHOD;
		logger_error("method null");
		return false;
	}
	if (strcmp(method, "GET") == 0)
		method_ = HTTP_METHOD_GET;
	else if (strcmp(method, "POST") == 0)
		method_ = HTTP_METHOD_POST;
	else if (strcmp(method, "PUT") == 0)
		method_ = HTTP_METHOD_PUT;
	else if (strcmp(method, "CONNECT") == 0)
		method_ = HTTP_METHOD_CONNECT;
	else if (strcmp(method, "PURGE") == 0)
		method_ = HTTP_METHOD_PURGE;
	else
	{
		logger_error("unkown method: %s", method);
		method_ = HTTP_METHOD_UNKNOWN;
		req_error_ = HTTP_REQ_ERR_METHOD;
		return false;
	}

	const char* ptr = getQueryString();
	if (ptr && *ptr)
		parseParameters(ptr);

	if (method_ == HTTP_METHOD_GET)
		request_type_ = HTTP_REQUEST_NORMAL;
	else if (method_ == HTTP_METHOD_POST)
	{
		acl_int64 len = getContentLength();
		if (len < -1)
			return false;
		if (len == 0)
		{
			request_type_ = HTTP_REQUEST_NORMAL;
			return true;
		}

		const char* ctype = getContentType();
		const char* stype = content_type_.get_stype();

		// 数据体为文件上传的 MIME 类型
		if (ctype == NULL)
			request_type_ = HTTP_REQUEST_OTHER;
		else if (strcasecmp(ctype, "multipart") == 0
			&& strcasecmp(stype, "form-data") == 0)
		{
			const char* bound = content_type_.get_bound();
			if (bound == NULL)
				request_type_ = HTTP_REQUEST_NORMAL;
			else
			{
				request_type_ = HTTP_REQUEST_MULTIPART_FORM;
				mime_ = NEW http_mime(bound, localCharset_);
			}
		}

		// 数据体为数据流类型
		else if (strcasecmp(ctype, "application") == 0
			&& strcasecmp(stype, "octet-stream") == 0)
		{
			request_type_ = HTTP_REQUEST_OCTET_STREAM;
		}

		// 数据体为普通的 name=value 类型
		else if (body_parse_)
		{
			if (len >= body_limit_)
				return false;

			request_type_ = HTTP_REQUEST_NORMAL;

			char* query = (char*) acl_mymalloc((size_t) len + 1);
			int ret = getInputStream().read(query, (size_t) len);
			if (ret > 0)
			{
				query[ret] = 0;
				parseParameters(query);
			}
			acl_myfree(query);
			return ret == -1 ? false : true;
		}
		else
			request_type_ = HTTP_REQUEST_OTHER;
	}

	return true;
}

const char* HttpServletRequest::getRequestReferer(void) const
{
	if (cgi_mode_)
		return acl_getenv("HTTP_REFERER");
	if (client_ == NULL)
		return NULL;
	return client_->header_value("Referer");
}

const http_ctype& HttpServletRequest::getHttpCtype(void) const
{
	return content_type_;
}

const char* HttpServletRequest::getRemoteHost(void) const
{
	if (cgi_mode_)
		return acl_getenv("HTTP_HOST");
	if (client_ == NULL)
		return NULL;
	return client_->request_host();
}

const char* HttpServletRequest::getUserAgent(void) const
{
	if (cgi_mode_)
		return acl_getenv("HTTP_USER_AGENT");
	if (client_ == NULL)
		return NULL;
	return client_->header_value("User-Agent");
}

bool HttpServletRequest::isKeepAlive(void) const
{
	if (cgi_mode_)
	{
		const char* ptr = acl_getenv("HTTP_CONNECTION");
		if (ptr == NULL || strcasecmp(ptr, "keep-alive") != 0)
			return false;
		else
			return true;
	}
	if (client_ == NULL)
		return false;
	return client_->keep_alive();
}

int HttpServletRequest::getKeepAlive(void) const
{
	if (cgi_mode_)
		return -1;

	if (client_ == NULL)
		return -1;
	const char* ptr = client_->header_value("Keep-Alive");
	if (ptr == NULL || *ptr == 0)
		return -1;
	return atoi(ptr);
}

void HttpServletRequest::setRwTimeout(int rw_timeout)
{
	rw_timeout_ = rw_timeout;
}

http_request_error_t HttpServletRequest::getLastError(void) const
{
	return req_error_;
}

http_client* HttpServletRequest::getClient(void) const
{
	if (client_ == NULL)
		logger_error("client_ NULL in CGI mode");
	return client_;
}

void HttpServletRequest::fprint_header(ostream& out, const char* prompt)
{
	if (client_)
	{
		client_->fprint_header(out, prompt);
	}
	else
	{
		const char* ptr = acl_getenv_list();
		if (ptr)
			out.format("%s", ptr);
	}
}

} // namespace acl
