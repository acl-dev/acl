#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/istream.hpp"

namespace acl {

int istream::read(void* buf, size_t size, bool loop /* = true */)
{
	int   ret;
	if (loop && size > 1)
		ret = acl_vstream_readn(m_pStream, buf, size);
	else
		ret = acl_vstream_read(m_pStream, buf, size);
	if (ret == ACL_VSTREAM_EOF)
	{
		m_bEof = true;
		return (-1);
	} else
		return (ret);
}

bool istream::readtags(void *buf, size_t* size, const char *tag, size_t taglen)
{
	int   ret = acl_vstream_readtags(m_pStream, buf, *size, tag, taglen);
	if (ret == ACL_VSTREAM_EOF)
		m_bEof = true;

	if ((m_pStream->flag & ACL_VSTREAM_FLAG_TAGYES))
		return (true);
	else
		return (false);
}

bool istream::gets(void* buf, size_t* size, bool nonl /* = true */)
{
	int   ret;
	if (nonl)
		ret = acl_vstream_gets_nonl(m_pStream, buf, *size);
	else
		ret = acl_vstream_gets(m_pStream, buf, *size);
	if (ret == ACL_VSTREAM_EOF) {
		m_bEof = true;
		*size = 0;
		return (false);
	} else {
		*size = ret;
		if ((m_pStream->flag | ACL_VSTREAM_FLAG_TAGYES))
			return (true);
		return (false);
	}
}

bool istream::read(acl_int64& n, bool loop /* = true */)
{
	return (read(&n, sizeof(n), loop) == (int) sizeof(n));
}

bool istream::read(int& n, bool loop /* = true */)
{
	return (read(&n, sizeof(n), loop) == (int) sizeof(n));
}

bool istream::read(short& n, bool loop /* = true */)
{
	return (read(&n, sizeof(n), loop) == (int) sizeof(n));
}

bool istream::read(char& ch)
{
	return (read(&ch, sizeof(ch), false) == (int) sizeof(ch));
}

bool istream::read(string& s, bool loop /* = true */)
{
	s.clear();
	int   ret;

	if ((ret = read(s.buf(), s.capacity(), loop)) == -1)
		return (false);
	s.set_offset(ret);
	return (true);
}

bool istream::read(string& s, size_t max, bool loop /* = true */)
{
	s.clear();
	s.space(max);
	int ret = read(s.buf(), max, loop);
	if (ret == -1)
		return false;
	s.set_offset(ret);
	return true;
}

bool istream::gets(string& s, bool nonl /* = true */)
{
	char buf[8192];

	s.clear();
	while (!eof()) {
		size_t size = sizeof(buf);
		if (gets(buf, &size, nonl) == true) {
			if (size > 0)
				s.append(buf, size);
			return (true);
		}
		if (size > 0)
			s.append(buf, size);
	}
	return (false);
}

bool istream::readtags(string& s, const string& tag)
{
	char buf[8192];

	s.clear();

	while (!eof()) {
		size_t size = sizeof(buf);
		if (readtags(buf, &size, tag.c_str(), tag.length()) == true) {
			if (size > 0)
				s.append(buf, size);
			return (true);
		}
		if (size > 0)
			s.append(buf, size);
	}
	return (false);
}

int istream::getch()
{
	int ret = acl_vstream_getc(m_pStream);
	if (ret == ACL_VSTREAM_EOF)
		m_bEof = true;
	return (ret);
}

int istream::ugetch(int ch)
{
	int   ret = acl_vstream_ungetc(m_pStream, ch);
	if (ret == ACL_VSTREAM_EOF)
		m_bEof = true;
	return (ret);
}

bool istream::gets_peek(string& buf, bool nonl /* = true */,
	bool clear /* = false */)
{
	if (clear)
		buf.clear();

	int ready, ret;
	ACL_VSTRING *vbf = (ACL_VSTRING*) buf.vstring();
	if (nonl)
		ret = acl_vstream_gets_nonl_peek(m_pStream, vbf, &ready);
	else
		ret = acl_vstream_gets_peek(m_pStream, vbf, &ready);
	if (ret == ACL_VSTREAM_EOF)
		m_bEof = true;
	return (ready ? true : false);
}

bool istream::read_peek(string& buf, bool clear /* = false */)
{
	if (clear)
		buf.clear();

	if (acl_vstream_read_peek(m_pStream, buf.vstring()) == ACL_VSTREAM_EOF)
	{
		m_bEof = true;
		return false;
	}
	else
		return true;
}

bool istream::readn_peek(string& buf, size_t cnt, bool clear /* = false */)
{
	if (clear)
		buf.clear();

	int ready;
	if (acl_vstream_readn_peek(m_pStream, buf.vstring(),
		(int) cnt, &ready) == ACL_VSTREAM_EOF)
	{
		m_bEof = true;
	}
	return (ready ? true : false);
}

istream& istream::operator>>(acl::string& s)
{
	s.clear();
	(void) read(s.buf(), s.length(), true);
	return (*this);
}

istream& istream::operator>>(acl_int64& n)
{
	(void) read(n);
	return (*this);
}

istream& istream::operator>>(int& n)
{
	(void) read(n);
	return (*this);
}

istream& istream::operator>>(short& n)
{
	(void) read(n);
	return (*this);
}

istream& istream::operator>>(char& ch)
{
	(void) read(ch);
	return (*this);
}

} // namespace acl
