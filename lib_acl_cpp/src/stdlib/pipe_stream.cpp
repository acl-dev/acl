#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/pipe_stream.hpp"
#include "acl_cpp/stdlib/string.hpp"
#endif

namespace acl {

pipe_string::pipe_string(void)
{
	m_pBuf = NEW string(128);
	m_pSavedBufPtr = m_pBuf;
	m_pos = 0;
}

pipe_string::pipe_string(string& s)
{
	m_pBuf = &s;
	m_pSavedBufPtr = NULL;
	m_pos = 0;
}

pipe_string::~pipe_string(void)
{
	delete m_pSavedBufPtr;
}

int pipe_string::push_pop(const char* in, size_t len,
	string* out, size_t max /* = 0 */)
{
	if (in && len > 0) {
		m_pBuf->append(in, len);
	}

	if (out == NULL) {
		return 0;
	}

	len = m_pBuf->length();
	acl_assert(len >= m_pos);
	len -= m_pos;

	if (len == 0) {
		return 0;
	}
	
	size_t n;
	if (max > 0) {
		n = max > len ? len : max;
	} else {
		n = len;
	}

	out->append(m_pBuf->c_str() + m_pos, n);
	m_pos += n;
	return (int) n;
}

int pipe_string::pop_end(string* out, size_t max /* = 0 */)
{
	if (out == NULL) {
		return 0;
	}
	size_t n = m_pBuf->length();
	acl_assert(n >= m_pos);
	n -= m_pos;
	if (n == 0) {
		return 0;
	}
	if (max > 0 && n > max) {
		n = max;
	}
	out->append(m_pBuf->c_str() + m_pos, n);
	m_pos += n;
	return (int) n;
}

////////////////////////////////////////////////////////////////////////

pipe_manager::pipe_manager(void)
{
	m_pBuf1 = NEW string(128);
	m_pBuf2 = NEW string(128);
	m_pPipeStream = NULL;
}

pipe_manager::~pipe_manager(void)
{
	delete m_pBuf1;
	delete m_pBuf2;
	if (m_pPipeStream) {
		delete m_pPipeStream;
	}
}

bool pipe_manager::push_back(pipe_stream* stream)
{
	std::list<pipe_stream*>::const_iterator cit = m_streams.begin();

	for (; cit != m_streams.end(); ++cit) {
		if (stream == *cit) {
			return false;
		}
	}

	m_streams.push_back(stream);
	return true;
}

bool pipe_manager::push_front(pipe_stream* stream)
{
	std::list<pipe_stream*>::const_iterator cit = m_streams.begin();

	for (; cit != m_streams.end(); ++cit) {
		if (stream == *cit) {
			return false;
		}
	}

	m_streams.push_front(stream);
	return true;
}

bool pipe_manager::update(const char* src, size_t len,
	pipe_stream* out /* = NULL */)
{
	std::list<pipe_stream*>::iterator it = m_streams.begin();

	string* pBuf = m_pBuf1;
	string* pLast = pBuf;
	pBuf->clear();

	for (; it != m_streams.end(); ++it) {
		if (len > 0 && (*it)->push_pop(src, len, pBuf) == -1) {
			return false;
		}
		src = pBuf->c_str();
		len = pBuf->length();
		pLast = pBuf;
		if (pBuf == m_pBuf1) {
			pBuf = m_pBuf2;
		} else {
			pBuf = m_pBuf1;
		}
		pBuf->clear();
	}

	if (!pLast->empty() && out != NULL) {
		return out->push_pop(pLast->c_str(),
			pLast->length(), NULL) == -1 ? false : true;
	}
	return true;
}

bool pipe_manager::update_end(pipe_stream* out /* = NULL */)
{
	if (m_streams.empty()) {
		return true;
	}

	std::list<pipe_stream*>::iterator it = m_streams.begin();

	string* pBuf = m_pBuf2;
	pBuf->clear();

	if ((*it)->pop_end(pBuf) == -1) {
		return false;
	}

	string* pLast = pBuf;
	const char* ptr = pLast->c_str();
	size_t len = pLast->length();

	pBuf = m_pBuf1;
	pBuf->clear();

	++it;
	for (; it != m_streams.end(); ++it) {
		if (len > 0 && (*it)->push_pop(ptr, len, pBuf) == -1) {
			return false;
		}
		if ((*it)->pop_end(pBuf) == -1) {
			return false;
		}

		pLast = pBuf;
		ptr = pLast->c_str();
		len = pLast->length();

		if (pBuf == m_pBuf1) {
			pBuf = m_pBuf2;
		} else {
			pBuf = m_pBuf1;
		}
		pBuf->clear();
	}

	if (!pLast->empty() && out != NULL) {
		return out->push_pop(pLast->c_str(),
			pLast->length(), NULL) == -1 ? false : true;
	}
	return true;
}

pipe_manager& pipe_manager::operator<<(const acl::string& s)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update(s.c_str(), s.length(), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(const acl::string* s)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}

	update(s->c_str(), s->length(), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(const char* s)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update(s, strlen(s), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(acl_int64 n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(acl_uint64 n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(long n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(unsigned long n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(int n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(unsigned int n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(short n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(unsigned short n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(char n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

pipe_manager& pipe_manager::operator<<(unsigned char n)
{
	if (m_pPipeStream == NULL) {
		m_pPipeStream = NEW pipe_string();
	}
	update((const char*) &n, sizeof(n), m_pPipeStream);
	return *this;
}

char* pipe_manager::c_str(void) const
{
	if (m_pPipeStream) {
		return m_pPipeStream->c_str();
	} else {
		static const char* dummy = "";
		return (char*) dummy;
	}
}

size_t pipe_manager::length(void) const
{
	if (m_pPipeStream) {
		return m_pPipeStream->length();
	} else {
		return 0;
	}
}

void pipe_manager::clear(void)
{
	if (m_pPipeStream) {
		m_pPipeStream->clear();
	}
}

} // namespace acl
