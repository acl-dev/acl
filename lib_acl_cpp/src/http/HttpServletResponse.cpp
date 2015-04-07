#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/ostream.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/HttpServlet.hpp"
#include "acl_cpp/http/HttpServletResponse.hpp"

namespace acl
{

HttpServletResponse::HttpServletResponse(socket_stream& stream)
: stream_(stream)
{
	header_ = NEW http_header();
	header_->set_request_mode(false);
	charset_[0] = 0;
	safe_snprintf(content_type_, sizeof(content_type_), "text/html");
	head_sent_ = false;
}

HttpServletResponse::~HttpServletResponse(void)
{
	delete header_;
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
	string buf;
	if (charset_[0] != 0)
		buf.format("%s; charset=%s", content_type_, charset_);
	else
		buf.format("%s", content_type_);
	header_->set_content_type(buf.c_str());

	buf.clear();
	header_->build_response(buf);
	return getOutputStream().write(buf) == -1 ? false : true;
}

bool HttpServletResponse::write(const void* data, size_t len)
{
	if (!head_sent_ && sendHeader() == false)
		return false;

	if (header_->chunked_transfer() == false)
	{
		if (data == NULL || len == 0)
			return true;
		return stream_.write(data, len) == -1 ? false : true;
	}

	if (data == NULL || len == 0)
		return stream_.format("0\r\n\r\n") == -1 ? false : true;

#if 1
	struct iovec iov[3];

	char hdr[32];
	safe_snprintf(hdr, sizeof(hdr), "%x\r\n", (int) len);

	iov[0].iov_base = (void*) hdr;
	iov[0].iov_len = strlen(hdr);

	iov[1].iov_base = (void*) data;
	iov[1].iov_len = (int) len;

	iov[2].iov_base = (void*) "\r\n";
	iov[2].iov_len = 2;

	return stream_.writev(iov, 3) == -1 ? false : true;
#else
	if (stream_.format("%x\r\n", (int) len) == -1)
		return false;
	if (stream_.write(data, len) == -1)
		return false;
	if (stream_.write("\r\n", 2) == -1)
		return false;
	return true;
#endif
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

} // namespace acl
