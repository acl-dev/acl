#include "acl_stdafx.hpp"
#include "acl_cpp/session/session.hpp"
#include "acl_cpp/http/HttpSession.hpp"

namespace acl
{

HttpSession::HttpSession(session& session)
: session_(session)
{
}

HttpSession::~HttpSession(void)
{
}

const char* HttpSession::getSid(void) const
{
	return session_.get_sid();
}

const char* HttpSession::getAttribute(const char* name) const
{
	return const_cast<HttpSession*> (this)->session_.get(name);
}

const void* HttpSession::getAttribute(const char* name, size_t* size) const
{
	const VBUF* bf = const_cast<HttpSession*>
		(this)->session_.get_vbuf(name);
	if (bf == NULL)
		return NULL;
	if (size)
		*size = bf->len;
	return bf->buf;
}

bool HttpSession::setAttribute(const char* name, const char* value)
{
	return setAttribute(name, value, strlen(value));
}

bool HttpSession::setAttribute(const char* name, const void* value, size_t len)
{
	return session_.set(name, value, len);
}

bool HttpSession::removeAttribute(const char* name)
{
	return session_.del(name);
}

bool HttpSession::setMaxAge(time_t ttl)
{
	return session_.set_ttl(ttl);
}

bool HttpSession::invalidate()
{
	return session_.remove();
}

} // namespace acl
