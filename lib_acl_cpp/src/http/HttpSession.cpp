#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/session/session.hpp"
#include "acl_cpp/http/HttpSession.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

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
	const session_string* bf = const_cast<HttpSession*>
		(this)->session_.get_buf(name);
	if (bf == NULL) {
		return NULL;
	}
	if (size) {
		*size = bf->length();
	}
	return bf->c_str();
}

bool HttpSession::getAttributes(std::map<string, session_string>& attrs) const
{
	return const_cast<HttpSession*> (this)->session_.get_attrs(attrs);
}

bool HttpSession::getAttributes(const std::vector<string>& names,
	std::vector<session_string>& values) const
{
	return const_cast<HttpSession*> (this)->session_.get_attrs(names, values);
}

bool HttpSession::setAttribute(const char* name, const char* value)
{
	return setAttribute(name, value, strlen(value));
}

bool HttpSession::setAttribute(const char* name, const void* value, size_t len)
{
	return session_.set(name, value, len);
}

bool HttpSession::setAttributes(const std::map<string, session_string>& attrs)
{
	return session_.set_attrs(attrs);
}

bool HttpSession::removeAttribute(const char* name)
{
	return session_.del(name);
}

bool HttpSession::setMaxAge(time_t ttl)
{
	return session_.set_ttl(ttl, false);
}

bool HttpSession::invalidate()
{
	return session_.remove();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
