#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/http/http_utils.hpp"
#endif

namespace acl
{

bool http_utils::get_addr(const char* url, char* addr, size_t size)
{
	char  buf[256];
	unsigned short port;

	if (!get_addr(url, buf, sizeof(buf), &port)) {
		return false;
	}
	safe_snprintf(addr, size, "%s:%d", buf, port);
	return true;
}

bool http_utils::get_addr(const char* url, char* domain, size_t size,
	unsigned short* pport)
{
#define	HTTP_PREFIX	"http://"
#define	HTTPS_PREFIX	"https://"

	const char* ptr;
	unsigned short default_port;

	if (!strncasecmp(url, HTTP_PREFIX, sizeof(HTTP_PREFIX) - 1)) {
		ptr = url + sizeof(HTTP_PREFIX) - 1;
		default_port = 80;
	} else if (!strncasecmp(url, HTTPS_PREFIX, sizeof(HTTPS_PREFIX) - 1)) {
		ptr = url + sizeof(HTTPS_PREFIX) - 1;
		default_port = 443;
	} else {
		logger_error("invalid url: %s", url);
		return false;
	}

	if (*ptr == 0) {
		logger_error("invalid url: %s", url);
		return false;
	}

	char buf[256];
	ACL_SAFE_STRNCPY(buf, ptr, sizeof(buf));

	char* slash = strchr(buf, '/');
	if (slash) {
		*slash = 0;
	}

	unsigned short port;

	char* col = strchr(buf, ':');
	if (col == NULL) {
		port = default_port;
	} else {
		*col++ = 0;
		port = (unsigned short) atoi(col);
		if (port == 0 || port == 65535) {
			port = default_port;
		}
	}

	if (pport) {
		*pport = port;
	}
	ACL_SAFE_STRNCPY(domain, buf, size);
	return true;
}

} // namespace acl
