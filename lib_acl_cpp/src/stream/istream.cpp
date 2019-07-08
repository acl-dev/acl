#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/istream.hpp"
#endif

namespace acl {

#if ACL_EWOULDBLOCK == ACL_EAGAIN
# define CHECK_ERROR(e) do { \
	if (e != ACL_EINTR && e != ACL_ETIMEDOUT && e != ACL_EWOULDBLOCK) \
		eof_ = true; \
} while (0)
#else
# define CHECK_ERROR(e) do { \
	if (e != ACL_EINTR && e != ACL_ETIMEDOUT \
		&& e != ACL_EWOULDBLOCK && e != ACL_EAGAIN) \
	{ \
		eof_ = true; \
	} \
} while (0)
#endif

int istream::read(void* buf, size_t size, bool loop /* = true */)
{
	int   ret;
	if (loop && size > 1) {
		ret = acl_vstream_readn(stream_, buf, size);
	} else {
		ret = acl_vstream_read(stream_, buf, size);
	}
	if (ret == ACL_VSTREAM_EOF) {
		CHECK_ERROR(errno);
		return -1;
	} else {
		return ret;
	}
}

bool istream::readtags(void *buf, size_t* size, const char *tag, size_t taglen)
{
	int   ret = acl_vstream_readtags(stream_, buf, *size, tag, taglen);
	if (ret == ACL_VSTREAM_EOF) {
		*size = 0;
		CHECK_ERROR(errno);
		return false;
	}

	*size = ret;

	if ((stream_->flag & ACL_VSTREAM_FLAG_TAGYES)) {
		return true;
	} else {
		return false;
	}
}

bool istream::read(acl_int64& n, bool loop /* = true */)
{
	return read(&n, sizeof(n), loop) == (int) sizeof(n);
}

bool istream::read(int& n, bool loop /* = true */)
{
	return read(&n, sizeof(n), loop) == (int) sizeof(n);
}

bool istream::read(short& n, bool loop /* = true */)
{
	return read(&n, sizeof(n), loop) == (int) sizeof(n);
}

bool istream::read(char& ch)
{
	return read(&ch, sizeof(ch), false) == (int) sizeof(ch);
}

bool istream::read(string& s, bool loop /* = true */)
{
	s.clear();
	int   ret;

	if ((ret = read(s.buf(), s.capacity(), loop)) == -1) {
		return false;
	}
	s.set_offset(ret);
	return true;
}

bool istream::read(string* s, bool loop /* = true */)
{
	acl_assert(s);
	return read(*s, loop);
}

bool istream::read(string& s, size_t max, bool loop /* = true */)
{
	s.clear();
	s.space(max);
	int ret = read(s.buf(), max, loop);
	if (ret == -1) {
		return false;
	}
	s.set_offset(ret);
	return true;
}

bool istream::read(string* s, size_t max, bool loop /* = true */)
{
	acl_assert(s);
	return read(*s, max, loop);
}

bool istream::gets(void* buf, size_t* size, bool nonl /* = true */)
{
	int   ret;
	if (nonl) {
		ret = acl_vstream_gets_nonl(stream_, buf, *size);
	} else {
		ret = acl_vstream_gets(stream_, buf, *size);
	}
	if (ret == ACL_VSTREAM_EOF) {
		CHECK_ERROR(errno);
		*size = 0;
		return false;
	} else {
		*size = ret;
		if ((stream_->flag & ACL_VSTREAM_FLAG_TAGYES)) {
			return true;
		}
		return false;
	}
}

bool istream::gets(string& s, bool nonl /* = true */, size_t max /* = 0 */)
{
	char   buf[8192];
	size_t size;

	s.clear();

	if (max == 0) {
		while (!eof()) {
			size = sizeof(buf);
			if (gets(buf, &size, nonl)) {
				if (size > 0) {
					s.append(buf, size);
				}

				return true;
			}

			if (size == 0) {
				break;
			}

//			printf(">>>size: %d\r\n", (int) size);
			s.append(buf, size);
		}

		return false;
	}

	size_t saved_max = max;

	while (!eof()) {
		size = sizeof(buf) > max ? max : sizeof(buf);
		if (gets(buf, &size, nonl)) {
			if (size > 0) {
				s.append(buf, size);
			}
			return true;
		}

		if (size == 0) {
			break;
		}

		s.append(buf, size);
		max -= size;

		if (max == 0) {
			logger_warn("reached the max limit: %d",
				(int) saved_max);
			return true;
		}
	}

	return false;
}

bool istream::gets(string* s, bool nonl /* = true */, size_t max /* = 0 */)
{
	acl_assert(s);
	return gets(*s, nonl, max);
}

bool istream::readtags(string& s, const string& tag)
{
	char buf[8192];

	s.clear();

	while (!eof()) {
		size_t size = sizeof(buf);
		if (readtags(buf, &size, tag.c_str(), tag.length())) {
			if (size > 0) {
				s.append(buf, size);
			}
			return true;
		}

		if (size == 0) {
			break;
		}

		s.append(buf, size);
	}

	return false;
}

bool istream::readtags(string* s, const string& tag)
{
	acl_assert(s);
	return readtags(*s, tag);
}

int istream::getch(void)
{
	int ret = acl_vstream_getc(stream_);
	if (ret == ACL_VSTREAM_EOF) {
		CHECK_ERROR(errno);
	}
	return ret;
}

int istream::ugetch(int ch)
{
	int ret = acl_vstream_ungetc(stream_, ch);
	if (ret == ACL_VSTREAM_EOF) {
		CHECK_ERROR(errno);
	}
	return ret;
}

bool istream::gets_peek(string& buf, bool nonl /* = true */,
	bool clear /* = false */, int max /* = 0 */)
{
	if (clear) {
		buf.clear();
	}

	if (max > 0) {
		buf.set_max(max);
	}

	int ready, ret;
	ACL_VSTRING *vbf = (ACL_VSTRING*) buf.vstring();
	if (nonl) {
		ret = acl_vstream_gets_nonl_peek(stream_, vbf, &ready);
	} else {
		ret = acl_vstream_gets_peek(stream_, vbf, &ready);
	}
	if (ret == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (stream_->errnum != ACL_EWOULDBLOCK) {
#else
		if (stream_->errnum != ACL_EWOULDBLOCK
			&& stream_->errnum != ACL_EAGAIN) {
#endif
			eof_ = true;
		}
	}

	return ready ? true : false;
}

bool istream::gets_peek(string* buf, bool nonl /* = true */,
	bool clear /* = false */, int max /* = 0 */)
{
	acl_assert(buf);
	return gets_peek(*buf, nonl, clear, max);
}

bool istream::read_peek(string& buf, bool clear /* = false */)
{
	if (clear) {
		buf.clear();
	}

	int n = acl_vstream_read_peek(stream_, buf.vstring());
	if (n == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (stream_->errnum != ACL_EWOULDBLOCK) {
#else
		if (stream_->errnum != ACL_EWOULDBLOCK
			&& stream_->errnum != ACL_EAGAIN) {
#endif
			eof_ = true;
		}
		return false;
	} else if (n == 0) {
		return false;
	} else {
		return true;
	}
}

bool istream::read_peek(string* buf, bool clear /* = false */)
{
	acl_assert(buf);
	return read_peek(*buf, clear);
}

int istream::read_peek(void* buf, size_t size)
{
	if (buf == NULL || size == 0) {
		logger_error("invalid params, buf=%p, size=%ld",
			buf, (long) size);
		return -1;
	}

	int n = acl_vstream_read_peek3(stream_, buf, size);
	if (n == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (stream_->errnum != ACL_EWOULDBLOCK) {
#else
		if (stream_->errnum != ACL_EWOULDBLOCK
			&& stream_->errnum != ACL_EAGAIN) {
#endif
			eof_ = true;
			return -1;
		}
		return 0;
	} else if (n == 0) {
		return 0;
	} else {
		return n;
	}
}

bool istream::readn_peek(string& buf, size_t cnt, bool clear /* = false */)
{
	if (clear) {
		buf.clear();
	}

	int ready;
	if (acl_vstream_readn_peek(stream_, buf.vstring(),
		(int) cnt, &ready) == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (stream_->errnum != ACL_EWOULDBLOCK) {
#else
		if (stream_->errnum != ACL_EWOULDBLOCK
			&& stream_->errnum != ACL_EAGAIN) {
#endif
			eof_ = true;
		}
	}
	return ready ? true : false;
}

bool istream::readn_peek(string* buf, size_t cnt, bool clear /* = false */)
{
	acl_assert(buf);
	return readn_peek(*buf, cnt, clear);
}

istream& istream::operator>>(acl::string& s)
{
	s.clear();
	(void) read(s.buf(), s.length(), true);
	return *this;
}

istream& istream::operator>>(acl_int64& n)
{
	(void) read(n);
	return *this;
}

istream& istream::operator>>(int& n)
{
	(void) read(n);
	return *this;
}

istream& istream::operator>>(short& n)
{
	(void) read(n);
	return *this;
}

istream& istream::operator>>(char& ch)
{
	(void) read(ch);
	return *this;
}

} // namespace acl
