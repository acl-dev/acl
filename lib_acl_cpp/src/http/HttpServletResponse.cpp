#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/ostream.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/http_client.hpp"
#include "acl_cpp/http/HttpServletRequest.hpp"
#include "acl_cpp/http/HttpServletResponse.hpp"
#endif

namespace acl
{

HttpServletResponse::HttpServletResponse(socket_stream& stream)
: stream_(stream)
, request_(NULL)
{
	dbuf_internal_ = new dbuf_guard;
	dbuf_ = dbuf_internal_;

	client_ = new (dbuf_->dbuf_alloc(sizeof(http_client)))
		http_client(&stream_, false, true);
	header_ = dbuf_->create<http_header, dbuf_guard*>(dbuf_);

	header_->set_request_mode(false);
	charset_[0] = 0;
	safe_snprintf(content_type_, sizeof(content_type_), "text/html");
	head_sent_ = false;
}

HttpServletResponse::~HttpServletResponse(void)
{
	client_->~http_client();
	delete dbuf_internal_;
}

HttpServletResponse& HttpServletResponse::setRedirect(
	const char* location, int status /* = 302 */)
{
	header_->add_entry("Location", location);
	header_->set_status(status);
	return *this;
}

HttpServletResponse& HttpServletResponse::setCharacterEncoding(
	const char* charset)
{
	safe_snprintf(charset_, sizeof(charset_), "%s", charset);
	return *this;
}

HttpServletResponse& HttpServletResponse::setKeepAlive(bool on)
{
	header_->set_keep_alive(on);
	return *this;
}

HttpServletResponse& HttpServletResponse::setContentLength(acl_int64 n)
{
	header_->set_content_length(n);
	return *this;
}

HttpServletResponse& HttpServletResponse::setChunkedTransferEncoding(bool on)
{
	header_->set_chunked(on);
	return *this;
}

HttpServletResponse& HttpServletResponse::setContentType(const char* value)
{
	safe_snprintf(content_type_, sizeof(content_type_), "%s", value);
	return *this;
}

HttpServletResponse& HttpServletResponse::setContentEncoding(bool gzip)
{
	header_->set_transfer_gzip(gzip);
	return *this;
}

HttpServletResponse& HttpServletResponse::setDateHeader(
	const char* name, time_t value)
{
	char buf[256];
	header_->date_format(buf, sizeof(buf), value);
	header_->add_entry(name, buf);
	return *this;
}

HttpServletResponse& HttpServletResponse::setHeader(
	const char* name, int value)
{
	char buf[32];
	safe_snprintf(buf, sizeof(buf), "%d", value);
	header_->add_entry(name, buf);
	return *this;
}

HttpServletResponse& HttpServletResponse::setHeader(
	const char* name, const char* value)
{
	header_->add_entry(name, value);
	return *this;
}

HttpServletResponse& HttpServletResponse::setStatus(int status)
{
	header_->set_status(status);
	return *this;
}

HttpServletResponse& HttpServletResponse::setRange(
	http_off_t from, http_off_t to, http_off_t total)
{
	header_->set_range(from, to);
	header_->set_range_total(total);
	return *this;
}

HttpServletResponse& HttpServletResponse::setCgiMode(bool on)
{
	header_->set_cgi_mode(on);
	return *this;
}

HttpServletResponse& HttpServletResponse::addCookie(HttpCookie* cookie)
{
	header_->add_cookie(cookie);
	return *this;
}

HttpServletResponse& HttpServletResponse::addCookie(
	const char* name, const char* value, const char* domain /* = NULL */,
	const char* path /* = NULL */, time_t expires /* = 0 */)
{
	acl_assert(name && *name && value);
	header_->add_cookie(name, value, domain, path, expires);
	return *this;
}

http_header& HttpServletResponse::getHttpHeader(void) const
{
	return *header_;
}

bool HttpServletResponse::sendHeader(void)
{
	if (head_sent_)
		return true;
	head_sent_ = true;

	acl_assert(header_->is_request() == false);

	char  buf[256];
	if (charset_[0] != 0)
		safe_snprintf(buf, sizeof(buf), "%s; charset=%s",
			content_type_, charset_);
	else
		safe_snprintf(buf, sizeof(buf), "%s", content_type_);

	header_->set_content_type(buf);

	// 虽然服务端在响应头中设置了 gzip 压缩方式，但如果请求端不接收
	// gzip 压缩数据，则需要从响应头中禁止
	if (header_->is_transfer_gzip() && request_)
	{
		bool accept_gzip = false;
		std::vector<string> tokens;
		request_->getAcceptEncoding(tokens);
		std::vector<string>::const_iterator it;

		for (it = tokens.begin(); it != tokens.end(); ++it)
		{
			if ((*it).compare("gzip", false) == 0)
			{
				accept_gzip = true;
				break;
			}
		}

		if (!accept_gzip)
			header_->set_transfer_gzip(false);
	}

	return client_->write_head(*header_);
}

bool HttpServletResponse::write(const void* data, size_t len)
{
	if (!head_sent_ && sendHeader() == false)
		return false;
	return client_->write_body(data, len);
}

bool HttpServletResponse::write(const string& buf)
{
	return write(buf.c_str(), buf.length());
}

int HttpServletResponse::vformat(const char* fmt, va_list ap)
{
	string buf;
	buf.vformat(fmt, ap);
	if (write(buf) == false)
		return -1;
	return (int) buf.length();
}

int HttpServletResponse::format(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = vformat(fmt, ap);
	va_end(ap);
	return ret;
}

void HttpServletResponse::encodeUrl(string& out, const char* url)
{
	out.clear();
	out.url_encode(url);
}

ostream& HttpServletResponse::getOutputStream(void) const
{
	return stream_;
}

void HttpServletResponse::setHttpServletRequest(HttpServletRequest* request)
{
	request_ = request;
}

} // namespace acl
