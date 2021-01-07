#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/ostream.hpp"
#endif

namespace acl {

int ostream::write(const void* data, size_t size, bool loop /* = true */,
	bool buffed /* = false */)
{
	int   ret;
	if (loop) {
		if (buffed) {
			ret = acl_vstream_buffed_writen(stream_, data, size);
		} else {
			ret = acl_vstream_writen(stream_, data, size);
		}
	} else {
		ret = acl_vstream_write(stream_, data, (int) size);
	}

	if (ret == ACL_VSTREAM_EOF) {
		eof_ = true;
	}
	return ret;
}

bool ostream::fflush(void)
{
	if (acl_vstream_fflush(stream_) == ACL_VSTREAM_EOF) {
		return false;
	} else {
		return true;
	}
}

int ostream::writev(const struct iovec *v, int count, bool loop /* = true */)
{
	int   ret;
	if (loop) {
		ret = acl_vstream_writevn(stream_, v, count);
	} else {
		ret = acl_vstream_writev(stream_, v, count);
	}

	if (ret == ACL_VSTREAM_EOF) {
		eof_ = true;
	}
	return ret;
}

int ostream::vformat(const char* fmt, va_list ap)
{
	int   ret = acl_vstream_vfprintf(stream_, fmt, ap);
	if (ret == ACL_VSTREAM_EOF) {
		eof_ = true;
	}
	return ret;
}

int ostream::write(acl_int64 n)
{
	return write(&n, sizeof(n), true);
}

int ostream::write(int n)
{
	return write(&n, sizeof(n), true);
}

int ostream::write(short n)
{
	return write(&n, sizeof(n), true);
}

int ostream::write(char ch)
{
	return write(&ch, sizeof(ch), false);
}

int ostream::write(const acl::string& s, bool loop /* = true */)
{
	return write(s.c_str(), s.length(), loop);
}

int ostream::format(const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int ret = vformat(fmt, ap);
	va_end(ap);
	return ret;
}

int ostream::puts(const char* s)
{
	return format("%s\r\n", s);
}

ostream& ostream::operator<<(const acl::string& s)
{
	(void) write(s.c_str(), s.length(), true);
	return *this;
}

ostream& ostream::operator<<(const char* s)
{
	(void) write(s, strlen(s), true);
	return *this;
}

ostream& ostream::operator<<(acl_int64 n)
{
	(void) write(&n, sizeof(n), true);
	return *this;
}

ostream& ostream::operator<<(int n)
{
	(void) write(&n, sizeof(n), true);
	return *this;
}

ostream& ostream::operator<<(short n)
{
	(void) write(&n, sizeof(n), true);
	return *this;
}

ostream& ostream::operator<<(char ch)
{
	(void) write(&ch, sizeof(ch), false);
	return *this;
}

int ostream::push_pop(const char* in, size_t len,
	string* out /* = NULL */ acl_unused, size_t max /* = 0 */ acl_unused)
{
	if (in == NULL || len == 0) {
		return 0;
	}
	if ((size_t) write(in, len) != len) {
		return -1;
	}
	if (out == NULL) {
		return 0;
	}
	if (max > 0 && len > max) {
		len = max;
	}
	out->append(in, len);
	return (int) len;
}

} // namespace acl
