#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/xml.hpp"
#include "acl_cpp/stdlib/json.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/sslbase_conf.hpp"
#include "acl_cpp/stream/sslbase_io.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/charset_conv.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/http/http_client.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpCookie.hpp"
#include "acl_cpp/http/http_ctype.hpp"
#include "acl_cpp/http/http_pipe.hpp"
#include "acl_cpp/http/http_request.hpp"
#endif

namespace acl
{

#define RESET_RANGE() do \
{ \
	range_from_ = -1; \
	range_to_ = -1; \
	range_max_ = -1; \
} while(0)

http_request::http_request(socket_stream* client, int conn_timeout /* = 60 */,
	bool unzip /* = true */, bool stream_fixed /* = false */)
{
	// 设置解压参数
	client_ = NEW http_client(client, true, unzip, stream_fixed);

	unzip_            = unzip;
	ssl_conf_         = NULL;
	local_charset_[0] = 0;
	conv_             = NULL;

	set_timeout(client->get_vstream()->rw_timeout, conn_timeout);

	const char* ptr = client->get_peer(true);
	acl_assert(ptr);
	ACL_SAFE_STRNCPY(addr_, ptr, sizeof(addr_));
	header_.set_url("/");
	header_.set_keep_alive(true);
	header_.set_host(addr_);
	cookie_inited_ = false;
	cookies_       = NULL;
	need_retry_    = true;
	RESET_RANGE();
}

http_request::http_request(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 60 */, bool unzip /* = true */)
{
	acl_assert(addr && *addr);
	ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
	set_timeout(conn_timeout, rw_timeout);

	unzip_            = unzip;
	ssl_conf_         = NULL;
	local_charset_[0] = 0;
	conv_             = NULL;

	header_.set_url("/");
	header_.set_keep_alive(true);
	header_.set_host(addr_);
	cookie_inited_ = false;
	cookies_    = NULL;
	client_     = NULL;
	need_retry_ = true;
	RESET_RANGE();
}

http_request::~http_request(void)
{
	close();

	if (cookies_) {
		reset();
		delete cookies_;
	} else {
		delete conv_;
	}
}

void http_request::close(void)
{
	delete client_;
	client_ = NULL;
}

void http_request::reset(void)
{
	if (cookies_) {
		std::vector<HttpCookie*>::iterator it = cookies_->begin();
		for (; it != cookies_->end(); ++it) {
			(*it)->destroy();
		}
		cookies_->clear();
		cookie_inited_ = false;
	}

	delete conv_;
	conv_ = NULL;
	need_retry_ = true;
	RESET_RANGE();
	if (client_) {
		client_->reset();
	}
	header_.reset();
}

http_request& http_request::set_unzip(bool on)
{
	unzip_ = on;
	return *this;
}

http_request& http_request::set_ssl(sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
	return *this;
}

bool http_request::open()
{
	bool reuse;
	return try_open(&reuse);
}

bool http_request::try_open(bool* reuse_conn)
{
	if (client_) {
		client_->reset();
		*reuse_conn = true;
		return true;
	}

	*reuse_conn = false;

	client_ = NEW http_client();

	if (!client_->open(addr_, conn_timeout_, rw_timeout_, unzip_)) {
		logger_error("connect server(%s) error(%s)",
			addr_, last_serror());
		close();
		return false;
	}

	if (ssl_conf_ == NULL) {
		return true;
	}

	sslbase_io* ssl = ssl_conf_->create(false);
	const char* host = header_.get_host();
	if (host && *host) {
		ssl->set_sni_host(host);
	}
	if (client_->get_stream().setup_hook(ssl) == ssl) {
		logger_error("open client ssl error to: %s", addr_);
		ssl->destroy();
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

bool http_request::write_head()
{
#if 0
	// 必须保证该连接已经打开
	if (client_ == NULL) {
		logger_error("connection not opened!");
		return false;
	}
#endif

	bool  reuse_conn;
	http_method_t method = header_.get_method();

	while (true) {
		// 尝试打开远程连接
		if (try_open(&reuse_conn) == false) {
			logger_error("connect server error");
			need_retry_ = false;
			return false;

		}

		// 如果是新创建的连接，则不需重试
		if (!reuse_conn) {
			need_retry_ = false;
		}

		// 如果请求方法非 GET/PURGE/HEAD/DELETE/CONNECT，
		// 则需要首先探测一下连接是否正常
		if (method != HTTP_METHOD_GET
			&& method != HTTP_METHOD_PURGE
			&& method != HTTP_METHOD_HEAD
			&& method != HTTP_METHOD_DELETE
			&& method != HTTP_METHOD_OPTION
			&& method != HTTP_METHOD_CONNECT
			&& !client_->get_stream().alive()) {

			/* 因为系统 write API 成功并不能保证连接正常，所以只能
			 * 是调用系统 read API 来探测连接是否正常，该函数内部
			 * 会将套接口先转非阻塞套接口进行读操作，所以不会阻塞，
			 * 同时即使有数据读到也会先放到 ACL_VSTREAM 读缓冲中，
			 * 所以也不会丢失数据
			 */
			close();

			// 对于新创建的连接，则直接报错
			if (!reuse_conn) {
				logger_error("new connection error");
				return false;
			}

			need_retry_ = false;
			continue;
		}

		client_->reset();  // 重置状态

		// 发送 HTTP 请求头
		if (client_->write_head(header_)) {
			return true;
		}

		close();

		if (!need_retry_) {
			return false;
		}

		// 先取消重试标志位，然后再重试一次
		need_retry_ = false;
	}
}

bool http_request::write_body(const void* data, size_t len)
{
	while (true) {
		if (client_->write_body(data, len) == false) {
			if (!need_retry_) {
				return false;
			}

			// 取消重试标志位
			need_retry_ = false;

			// 再重试一次
			if (write_head() == false) {
				return false;
			}

			// 再次写数据体
			continue;
		}

		// 说明至少已经两次写操作了，所以应该取消重试标志位
		need_retry_ = false;

		// 如果数据非空，则说明还有数据可写
		if (data != NULL && len > 0) {
			return true;
		}

		// data == NULL || len == 0 时，表示请求数据
		// 已经发送完毕，开始从服务端读取 HTTP 响应数据
		// 读 HTTP 响应头
		if (client_->read_head() == true) {
			break;
		}

		return false;
	}

	// 说明所有数据已经发送完毕，并且成功读取了 HTTP 响应头，
	// 下面可以读取 HTTP 响应数据体了

	// 设置字符集转换器
	set_charset_conv();

	// 检查返回头中是否有 Content-Range 字段
	check_range();

	return true;
}

bool http_request::send_request(const void* data, size_t len)
{
	// 必须保证该连接已经打开
	if (client_ == NULL) {
		return false;
	}

	client_->reset();  // 重置状态

	// 写 HTTP 请求头
	if (client_->write_head(header_) == false) {
		close();
		return false;
	}

	// 写 HTTP 请求体
	if (client_->write_body(data, len) == false) {
		close();
		return false;
	}

	return true;
}

bool http_request::request(const void* data, size_t len)
{
	bool  have_retried = false;
	bool  reuse_conn;
	http_method_t method = header_.get_method();

	// 构建 HTTP 请求头
	if (data && len > 0) {
		header_.set_content_length(len);

		if (method != HTTP_METHOD_POST && method != HTTP_METHOD_PUT
			&& method != HTTP_METHOD_PATCH) {

			// 在有数据体的条件下，重新设置 HTTP 请求方法
			header_.set_method(HTTP_METHOD_POST);
		}
	} else if (method == HTTP_METHOD_POST || method == HTTP_METHOD_PUT) {
		header_.set_content_length(0);
	}

	while (true) {
		// 尝试打开远程连接
		if (try_open(&reuse_conn) == false) {
			logger_error("connect server error");
			return false;
		}

		// 发送 HTTP 请求至服务器
		if (send_request(data, len) == false) {
			if (have_retried || !reuse_conn) {
				logger_error("send request error");
				return false;
			}

			// 对于长连接，如果是第一次IO失败，则可以再重试一次
			have_retried = true;
			continue;
		}

		client_->reset();  // 重置状态

		// 读 HTTP 响应头
		if (client_->read_head() == true) {
			break;
		}

		if (have_retried || !reuse_conn) {
			logger_error("read response header error");
			close();
			return false;
		}

		// 先关闭之前的连接流
		close();

		// 对于长连接，如果是第一次IO失败，则可以再重试一次
		have_retried = true;
	}

	// 设置字符集转换器
	set_charset_conv();

	// 检查返回头中是否有 Content-Range 字段
	check_range();

	return true;
}

int http_request::http_status() const
{
	return client_ ? client_->response_status() : -1;
}

acl_int64 http_request::body_length() const
{
	return client_ ? client_->body_length() : -1;
}

bool http_request::keep_alive() const
{
	return client_ ? client_->keep_alive() : false;
}

const char* http_request::header_value(const char* name) const
{
	return client_ ? client_->header_value(name) : NULL;
}

bool http_request::body_finish() const
{
	return client_ ? client_->body_finish() : false;
}

void http_request::check_range()
{
	http_off_t range_from, range_to;
	acl_int64 length;

	// 先取出用户在请求时设置的 range 字段，如果没设置则直接返回
	header_.get_range(&range_from, &range_to);
	if (range_from < 0) {
		return;
	}

	const HTTP_HDR_RES* hdr_res = client_->get_respond_head(NULL);

	// 从 HTTP 服务器响应中获得 range 回应字段，如果没有则说明
	// 服务器不支持 range 请求
	if (http_hdr_res_range((HTTP_HDR_RES*) hdr_res, &range_from_,
		&range_to_, &range_max_) < 0) {

		RESET_RANGE();
	}

	// 如果服务器返回的 range 内容与请求的不一致，则说明有错
	else if (range_from_ != range_from) {
		logger_error("range_from(%lld) != %lld",
			range_from_, range_from);
		RESET_RANGE();
	}
	//else if (range_to >= range_from && range_to_ != range_to)
	else if (range_to >= range_from && range_to_ > range_to) {
		logger_error("range_to(%lld) > %lld", range_to_, range_to);
		RESET_RANGE();
	}

	// 虽然用户发送了 range 请求，但发送的请求内容是整个数据体，
	// 则需要检查数据体长度的一致性
	else if (range_from == 0 && range_to < 0
		&& (length = client_->body_length()) > 0
		&& range_max_ != length) {

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

http_request& http_request::set_local_charset(const char* local_charset)
{
	ACL_SAFE_STRNCPY(local_charset_, local_charset,
		sizeof(local_charset_));
	return *this;
}

void http_request::set_charset_conv()
{
	if (client_ == NULL || local_charset_[0] == 0) {
		return;
	}

	// 需要获得响应头字符集信息
	const char* ptr = client_->header_value("Content-Type");
	if (ptr == NULL || *ptr == 0) {
		return;
	}

#if !defined(ACL_MIME_DISABLE)

	http_ctype ctype;
	ctype.parse(ptr);

	const char* from_charset = ctype.get_charset();

	if (from_charset == NULL || *from_charset == 0
		|| strcasecmp(from_charset, local_charset_) == 0) {

		return;
	}

	// 初始化字符集转换器

	if (conv_ == NULL) {
		conv_ = charset_conv::create(from_charset, local_charset_);
	}
	// 复用之前创建的字符集转换器
	else if (!conv_->update_begin(from_charset, local_charset_)) {
		logger_error("invalid charset conv, from %s, to %s",
			from_charset, local_charset_);
		delete conv_;
		conv_ = NULL;
	}

#endif // !defined(ACL_MIME_DISABLE)
}

http_pipe* http_request::get_pipe(const char* to_charset)
{
	if (to_charset != NULL) {
		// 重新设置字符集转换器
		set_local_charset(to_charset);

		// 获取字符集转换器
		set_charset_conv();
	}

	if (conv_ == NULL) {
		return NULL;
	}

	http_pipe* hp = NEW http_pipe();

	// 将字符集转换器交由 http_pipe 管理
	hp->set_charset(conv_);
	conv_ = NULL;

	return hp;
}

bool http_request::get_body(xml& out, const char* to_charset /* = NULL */)
{
	if (client_ == NULL) {
		return false;
	}

	http_pipe* hp = get_pipe(to_charset);
	if (hp) {
		hp->append(&out);
	}

	string buf(4096);
	int    ret;

	// 读 HTTP 响应体，并按 xml 格式进行分析
	while (true) {
		// 调用可以自动解压缩的读函数
		ret = client_->read_body(buf);
		if (ret < 0) {
			close();
			break;
		} else if (ret == 0) {
			break;
		}
		if (hp) {
			hp->update(buf.c_str(), ret);
		} else {
			out.update(buf.c_str());
		}
	}

	if (hp) {
		hp->update_end();
		delete hp;
	}
	return true;
}

bool http_request::get_body(json& out, const char* to_charset /* = NULL */)
{
	if (client_ == NULL) {
		return false;
	}

	http_pipe* hp = get_pipe(to_charset);
	if (hp) {
		hp->append(&out);
	}

	string  buf(4096);
	int   ret;
	// 读 HTTP 响应体，并按 json 格式进行分析员
	while (true) {
		ret = client_->read_body(buf);
		if (ret < 0) {
			close();
			break;
		} else if (ret == 0) {
			break;
		}
		if (hp) {
			hp->update(buf.c_str(), ret);
		} else {
			out.update(buf.c_str());
		}
	}

	if (hp) {
		hp->update_end();
		delete hp;
	}
	return true;
}

bool http_request::get_body(string& out, const char* to_charset /* = NULL */)
{
	if (client_ == NULL) {
		return false;
	}

	http_pipe* hp = get_pipe(to_charset);
	pipe_string* ps;
	if (hp) {
		ps = NEW pipe_string(out);
		hp->append(ps);
	} else {
		ps = NULL;
	}

	string  buf(4096);
	int   ret;
	// 读 HTTP 响应体
	while (true) {
		ret = client_->read_body(buf);
		if (ret < 0) {
			close();
			break;
		} else if (ret == 0) {
			break;
		}
		if (hp) {
			hp->update(buf.c_str(), ret);
		} else {
			out.append(buf);
		}
	}

	if (hp) {
		hp->update_end();
		delete hp;
	}

	delete ps;
	return true;
}

int http_request::read_body(string& out, bool clean /* = false */,
	int* real_size /* = NULL */)
{
	if (clean) {
		out.clear();
	}
	if (client_ == NULL) {
		return -1;
	}

	int   ret;

	if (conv_ == NULL) {
		ret = client_->read_body(out, clean, real_size);
		if (ret < 0) {
			close();
		}
		return ret;
	}

	size_t saved_size = out.length();
	string  buf(4096);
	ret = client_->read_body(buf, true, real_size);
	if (ret < 0) {
		conv_->update_finish(&out);
		close();
		return ret;
	}

	if (ret == 0) {
		conv_->update_finish(&out);
	} else if (ret > 0) {
		conv_->update(buf.c_str(), ret, &out);
	}

	size_t curr_size = out.length();

	// 在进行字符集转换时，内容尺寸可能变化，所以根据前后实际
	// 内容尺寸之差来计算本次读到的数据长度
	return (int) (curr_size - saved_size);
}

bool http_request::body_gets(string& out, bool nonl /* = true */,
	size_t* size /* = NULL */)
{
	if (size) {
		*size = 0;
	}
	if (client_ == NULL) {
		return false;
	}

	if (conv_ == NULL) {
		if (client_->body_gets(out, nonl, size) == true) {
			return true;
		} else {
			if (client_->disconnected()) {
				close();
			}
			return false;
		}
	}

	size_t n, size_saved = out.length();
	string line(1024);
	if (client_->body_gets(line, nonl, &n) == false) {
		if (!line.empty()) {
			conv_->update(line.c_str(), line.length(), &out);
		}
		conv_->update_finish(&out);
		if (size) {
			*size = out.length() - size_saved;
		}
		if (client_->disconnected()) {
			close();
		}
		return false;
	}

	if (!line.empty()) {
		conv_->update(line.c_str(), line.length(), &out);
	}
	conv_->update_finish(&out);
	if (size) {
		*size = out.length() - size_saved;
	}

	return true;
}

int http_request::read_body(char* buf, size_t size)
{
	if (client_ == NULL) {
		return -1;
	}
	return client_->read_body(buf, size);
}

const std::vector<HttpCookie*>* http_request::get_cookies() const
{
	if (cookies_ && cookie_inited_) {
		return cookies_;
	}
	const_cast<http_request*>(this)->create_cookies();
	if (cookie_inited_ == false) {
		return NULL;
	}
	return cookies_;
}

const HttpCookie* http_request::get_cookie(const char* name,
	bool case_insensitive /* = true */) const
{
	if (!cookie_inited_) {
		const_cast<http_request*>(this)->create_cookies();
	}
	if (cookies_ == NULL) {
		return NULL;
	}

	std::vector<HttpCookie*>::const_iterator cit = cookies_->begin();
	for (; cit != cookies_->end(); ++cit) {
		if (case_insensitive) {
			if (strcasecmp((*cit)->getName(), name) == 0) {
				return *cit;
			}
		} else if (strcmp((*cit)->getName(), name) == 0) {
			return *cit;
		}
	}
	return NULL;
}

void http_request::create_cookies()
{
	acl_assert(cookie_inited_ == false);
	if (client_ == NULL) {
		return;
	}

	const HTTP_HDR_RES* res = client_->get_respond_head(NULL);
	if (res == NULL) {
		return;
	}

	if (cookies_ == NULL) {
		cookies_ = NEW std::vector<HttpCookie*>;
	}

	int n = acl_array_size(res->hdr.entry_lnk);
	for (int i = 0; i < n; i++) {
		const HTTP_HDR_ENTRY* hdr = (const HTTP_HDR_ENTRY*)
			acl_array_index(res->hdr.entry_lnk, i);
		if (strcasecmp(hdr->name, "Set-Cookie") != 0)
			continue;
		if (hdr->value == NULL || *(hdr->value) == 0)
			continue;
		HttpCookie* cookie = NEW HttpCookie();
		if (cookie->setCookie(hdr->value) == false) {
			cookie->destroy();
			continue;
		}
		cookies_->push_back(cookie);
	}

	cookie_inited_ = true;
}

} // namespace acl
