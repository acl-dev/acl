#include "acl_stdafx.hpp"
#include "acl_cpp/stream/stream.hpp"

namespace acl {

stream::stream(void)
	: m_pStream(NULL)
	, m_bEof(true)
	, m_bOpened(false)
	, ctx_(NULL)
{
	open_stream();
}

stream::~stream(void)
{
	if (m_pStream)
		acl_vstream_free(m_pStream);
}

bool stream::eof(void) const
{
	return (m_bEof);
}

bool stream::opened(void) const
{
	return (m_bOpened);
}

ACL_VSTREAM* stream::get_vstream() const
{
	return (m_pStream);
}

void stream::set_rw_timeout(int n)
{
	if (m_pStream)
		m_pStream->rw_timeout = n;
}

int stream::get_rw_timeout() const
{
	if (m_pStream == NULL)
		return -1;
	return m_pStream->rw_timeout;
}

ACL_VSTREAM* stream::unbind()
{
	m_bEof = true;
	m_bOpened = false;
	ACL_VSTREAM* vstream = m_pStream;
	m_pStream = NULL;
	return vstream;
}

void stream::open_stream(void)
{
	if (m_pStream != NULL)
		return;
	m_pStream = acl_vstream_fdopen(ACL_SOCKET_INVALID, O_RDWR,
		8192, 0, ACL_VSTREAM_TYPE_SOCK);
}

void stream::reopen_stream(void)
{
	if (m_pStream)
		acl_vstream_free(m_pStream);
	open_stream();
}

void stream::set_ctx(void* ctx)
{
	ctx_ = ctx;
}

void* stream::get_ctx() const
{
	return (ctx_);
}

} // namespace acl
