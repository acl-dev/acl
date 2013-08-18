#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/http/http_utils.hpp"

namespace acl
{

bool http_utils::get_addr(const char* url, char* out, size_t size)
{
	if (strncasecmp(url, "http://", sizeof("http://") - 1) != 0)
	{
		logger_error("invalid url: %s", url);
		return false;
	}

	const char* ptr = url + sizeof("http://") - 1;
	if (*ptr == 0)
	{
		logger_error("invalid url: %s", url);
		return false;
	}

	char buf[256];
	ACL_SAFE_STRNCPY(buf, ptr, sizeof(buf));

	char* slash = strchr(buf, '/');
	if (slash)
		*slash = 0;
	unsigned short port;
	char* col = strchr(buf, ':');
	if (col == NULL)
		port = 80;
	else
	{
		*col++ = 0;
		port = (unsigned short) atoi(col);
		if (port < 80)
			port = 80;
	}

	snprintf(out, size, "%s:%d", buf, port);
	return true;
}

} // namespace acl
