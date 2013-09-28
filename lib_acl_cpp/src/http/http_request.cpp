#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/xml.hpp"
#include "acl_cpp/stdlib/json.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/http/http_client.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpCookie.hpp"
#include "acl_cpp/http/http_ctype.hpp"
#include "acl_cpp/http/http_pipe.hpp"
#include "acl_cpp/http/http_request.hpp"

namespace acl
{

#define RESET_RANGE() do \
{ \
	range_from_ = -1; \
	range_to_ = -1; \
	range_max_ = -1; \
} while(0)

http_request::http_request(socket_stream* client,
	int conn_timeout /* = 60 */, bool unzip /* = true */)
{
	// 设置解压参数
	client_ = NEW http_client(client, client->get_vstream()->rw_timeout,
		true, unzip);
	unzip_ = unzip;
	conv_ = NULL;

	const char* ptr = client->get_peer();
	acl_assert(ptr);
	ACL_SAFE_STRNCPY(addr_, ptr, sizeof(addr_));
	rw_timeout_ = client->get_vstream()->rw_timeout;
	conn_timeout_ = conn_timeout;
	header_.set_url("/");
	header_.set_keep_alive(true);
	header_.set_host(addr_);
	cookie_inited_ = false;
	cookies_ = NULL;
	RESET_RANGE();
}

http_request::http_request(const char* addr,
	int conn_timeout /* = 60 */, int rw_timeout /* = 60 */,
	bool unzip /* = true */)
{
	acl_assert(addr && *addr);
	ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	unzip_ = unzip;
	conv_ = NULL;

	header_.set_url("/");
	header_.set_keep_alive(true);
	header_.set_host(addr_);
	cookie_inited_ = false;
	cookies_ = NULL;
	client_ = NULL;
	RESET_RANGE();
}

http_request::~http_request(void)
{
	close();
	if (cookies_)
	{
		reset();
		delete cookies_;
	}
	else if (conv_)
		delete conv_;
}

void http_request::close(void)
{
	if (client_)
	{
		delete client_;
		client_ = NULL;
	}
}

void http_request::reset(void)
{
	if (cookies_)
	{
		std::vector<HttpCookie*>::iterator it = cookies_->begin();
		for (; it != cookies_->end(); ++it)
			(*it)->destroy();
		cookies_->clear();
		cookie_inited_ = false;
	}
	if (conv_)
	{
		delete conv_;
		conv_ = NULL;
	}
	RESET_RANGE();
}

http_request& http_request::set_unzip(bool on)
{
	unzip_ = on;
	return *this;
}

bool http_request::open()
{
	bool reuse;
	return try_open(&reuse);
}

bool http_request::try_open(bool* reuse_conn)
{
	if (client_)
	{
		client_->reset();
		*reuse_conn = true;
		return true;
	}

	*reuse_conn = false;

	client_ = new http_client();

	if (client_->open(addr_, conn_timeout_,
		rw_timeout_, unzip_) == false)
	{
		logger_error("connect server(%s) error(%s)",
			addr_, last_serror());
		close();
		return false;
	}

	return true;
}

http_header& http_request::request_header()
{
	return header_;
}

http_client* http_request::get_client(void) const
{
	return client_;
}

bool http_request::send_request(const char* data, size_t len)
{
	acl_assert(client_);  // 必须保证该连接已经打开
	client_->reset();  // 重置状态

	// 写 HTTP 请求头
	if (client_->write(header_) == false)
	{
		close();
		return false;
	}

	if (data == NULL || len == 0)
		return true;

	// 写 HTTP 请求体
	if (client_->get_ostream().write(data, len) == -1)
	{
		close();
		return false;
	}

	return true;
}

bool http_request::request(const char* data, size_t len)
{
	bool  have_retried = false;
	bool  reuse_conn;

	// 构建 HTTP 请求头
	if (data && len > 0)
	{
		http_method_t method = header_.get_method();
		// 在有数据体的条件下，HTTP 请求方法必须为以下两者之一：
		// HTTP_METHOD_POST 或 HTTP_METHOD_PUT
		if (method != HTTP_METHOD_POST && method != HTTP_METHOD_PUT)
			header_.set_method(HTTP_METHOD_POST);
		header_.set_content_length(len);
	}

	while (true)
	{
		// 尝试打开远程连接
		if (try_open(&reuse_conn) == false)
		{
			logger_error("connect server error");
			return false;

		}

		// 发送 HTTP 请求至服务器
		if (send_request(data, len) == false)
		{
			if (have_retried || !reuse_conn)
			{
				logger_error("send request error");
				return false;
			}

			// 对于长连接，如果是第一次IO失败，则可以再重试一次
			have_retried = true;
			continue;
		}

		client_->reset();  // 重置状态

		// 读 HTTP 响应头
		if (client_->read_head() == true)
			break;

		if (have_retried || !reuse_conn)
		{
			logger_error("read response header error");
			close();
			return false;
		}

		// 先关闭之前的连接流
		close();

		// 对于长连接，如果是第一次IO失败，则可以再重试一次
		have_retried = true;
	}

	// 检查返回头中是否有 Content-Range 字段
	check_range();
	return true;
}

void http_request::check_range()
{
	http_off_t range_from, range_to;
	acl_int64 length;

	// 先取出用户在请求时设置的 range 字段，如果没设置则直接返回
	header_.get_range(&range_from, &range_to);
	if (range_from < 0)
		return;

	const HTTP_HDR_RES* hdr_res = client_->get_respond_head(NULL);

	// 从 HTTP 服务器响应中获得 range 回应字段，如果没有则说明
	// 服务器不支持 range 请求
	if (http_hdr_res_range((HTTP_HDR_RES*) hdr_res, &range_from_,
		&range_to_, &range_max_) < 0)
	{
		RESET_RANGE();
	}

	// 如果服务器返回的 range 内容与请求的不一致，则说明有错
	else if (range_from_ != range_from)
	{
		logger_error("range_from(%lld) != %lld", range_from_, range_from);
		RESET_RANGE();
	}
	else if (range_to >= range_from && range_to_ != range_to)
	{
		logger_error("range_to(%lld) != %lld", range_to_, range_to);
		RESET_RANGE();
	}

	// 虽然用户发送了 range 请求，但发送的请求内容是整个数据体，
	// 则需要检查数据体长度的一致性
	else if (range_from == 0 && range_to < 0
		&& (length = client_->body_length()) > 0
		&& range_max_ != length)
	{
		logger_error("range_total_length: %lld != content_length: %lld",
			range_max_, length);
		RESET_RANGE();
	}
}

bool http_request::support_range() const
{
	return range_from_ >= 0 ? true : false;
}

acl_int64 http_request::get_range_from() const
{
	return range_from_;
}

acl_int64 http_request::get_range_to() const
{
	return range_to_;
}

acl_int64 http_request::get_range_max() const
{
	return range_max_;
}

http_pipe* http_request::get_pipe(const char* to_charset)
{
	if (to_charset == NULL || *to_charset == 0)
		return NULL;

	// 需要获得响应头字符集信息
	const char* ptr = client_->header_value("Content-Type");
	if (ptr == NULL || *ptr == 0)
		return NULL;

	http_ctype ctype;
	ctype.parse(ptr);

	const char* from_charset = ctype.get_charset();

	if (from_charset && strcasecmp(from_charset, to_charset) != 0)
	{
		http_pipe* hp = NEW http_pipe();
		hp->set_charset(from_charset, to_charset);
		return hp;
	}
	else
		return NULL;
}

bool http_request::get_body(xml& out, const char* to_charset /* = NULL */)
{
	if (client_ == NULL)
		return false;

	http_pipe* hp = get_pipe(to_charset);
	if (hp)
		hp->append(&out);

	string  buf(4096);
	int   ret;
	// 读 HTTP 响应体，并按 xml 格式进行分析
	while (true)
	{
		// 调用可以自动解压缩的读函数
		ret = client_->read_body(buf);
		if (ret < 0)
		{
			close();
			break;
		}
		else if (ret == 0)
			break;
		if (hp)
			hp->update(buf.c_str(), ret);
		else
			out.update(buf.c_str());
	}

	if (hp)
	{
		hp->update_end();
		delete hp;
	}
	return true;
}

bool http_request::get_body(json& out, const char* to_charset /* = NULL */)
{
	if (client_ == NULL)
		return false;

	http_pipe* hp = get_pipe(to_charset);
	if (hp)
		hp->append(&out);

	string  buf(4096);
	int   ret;
	// 读 HTTP 响应体，并按 json 格式进行分析员
	while (true)
	{
		ret = client_->read_body(buf);
		if (ret < 0)
		{
			close();
			break;
		}
		else if (ret == 0)
			break;
		if (hp)
			hp->update(buf.c_str(), ret);
		else
			out.update(buf.c_str());
	}

	if (hp)
	{
		hp->update_end();
		delete hp;
	}
	return true;
}

bool http_request::get_body(string& out, const char* to_charset /* = NULL */)
{
	if (client_ == NULL)
		return false;

	http_pipe* hp = get_pipe(to_charset);
	if (hp)
	{
		pipe_string ps(out);
		hp->append(&ps);
	}

	string  buf(4096);
	int   ret;
	// 读 HTTP 响应体
	while (true)
	{
		ret = client_->read_body(buf);
		if (ret < 0)
		{
			close();
			break;
		}
		else if (ret == 0)
			break;
		if (hp)
			hp->update(buf.c_str(), ret);
		else
			out.append(buf);
	}

	if (hp)
	{
		hp->update_end();
		delete hp;
	}
	return true;
}

void http_request::set_charset(const char* to_charset)
{
	if (client_ == NULL || to_charset == NULL || *to_charset == 0)
	{
		if (conv_)
		{
			delete conv_;
			conv_ = NULL;
		}
		return;
	}

	// 需要获得响应头字符集信息
	const char* ptr = client_->header_value("Content-Type");
	if (ptr == NULL || *ptr == 0)
	{
		if (conv_)
		{
			delete conv_;
			conv_ = NULL;
		}
		return;
	}

	http_ctype ctype;
	ctype.parse(ptr);

	const char* from_charset = ctype.get_charset();
	if (from_charset && strcasecmp(from_charset, to_charset) != 0)
	{
		if (conv_ == NULL)
			conv_ = charset_conv::create(from_charset, to_charset);
		else if (conv_->update_begin(from_charset, to_charset) == false)
		{
			logger_error("invalid charset conv, from %s, to %s",
				from_charset, to_charset);
			delete conv_;
			conv_ = NULL;
		}
	}
	else if (conv_)
	{
		delete conv_;
		conv_ = NULL;
	}
}

int http_request::read_body(string& out, bool clean /* = NULL */,
	int* real_size /* = NULL */)
{
	if (clean)
		out.clear();
	if (client_ == NULL)
		return -1;

	int   ret;
	if (conv_ == NULL)
	{
		ret = client_->read_body(out, clean, real_size);
		if (ret < 0)
			close();
		return ret;
	}

	string  buf(4096);
	ret = client_->read_body(buf, true, real_size);
	if (ret < 0)
	{
		conv_->update_finish(&out);
		close();
	}
	else if (ret == 0)
		conv_->update_finish(&out);
	else if (ret > 0)
		conv_->update(buf.c_str(), ret, &out);

	return ret;
}

int http_request::get_body(char* buf, size_t size)
{
	if (client_ == NULL)
		return -1;
	return client_->read_body(buf, size);
}

const std::vector<HttpCookie*>* http_request::get_cookies() const
{
	if (cookies_ && cookie_inited_)
		return cookies_;
	const_cast<http_request*>(this)->create_cookies();
	if (cookie_inited_ == false)
		return NULL;
	return cookies_;
}

const HttpCookie* http_request::get_cookie(const char* name,
	bool case_insensitive /* = true */) const
{
	if (!cookie_inited_)
		const_cast<http_request*>(this)->create_cookies();
	if (cookies_ == NULL)
		return NULL;
	std::vector<HttpCookie*>::const_iterator cit = cookies_->begin();
	for (; cit != cookies_->end(); ++cit)
	{
		if (case_insensitive)
		{
			if (strcasecmp((*cit)->getName(), name) == 0)
			{
				return *cit;
			}
		}
		else if (strcmp((*cit)->getName(), name) == 0)
		{
			return *cit;
		}
	}
	return NULL;
}

void http_request::create_cookies()
{
	acl_assert(cookie_inited_ == false);
	if (client_ == NULL)
		return;
	const HTTP_HDR_RES* res = client_->get_respond_head(NULL);
	if (res == NULL)
		return;

	if (cookies_ == NULL)
		cookies_ = NEW std::vector<HttpCookie*>;

	int n = acl_array_size(res->hdr.entry_lnk);
	for (int i = 0; i < n; i++)
	{
		const HTTP_HDR_ENTRY* hdr = (const HTTP_HDR_ENTRY*)
			acl_array_index(res->hdr.entry_lnk, i);
		if (strcasecmp(hdr->name, "Set-Cookie") != 0)
			continue;
		if (hdr->value == NULL || *(hdr->value) == 0)
			continue;
		HttpCookie* cookie = NEW HttpCookie();
		if (cookie->setCookie(hdr->value) == false)
		{
			cookie->destroy();
			continue;
		}
		cookies_->push_back(cookie);
	}
	cookie_inited_ = true;
}

} // namespace acl
