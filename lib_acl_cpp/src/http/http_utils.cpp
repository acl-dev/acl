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

#define	HTTP_PREFIX	"http://"
#define	HTTPS_PREFIX	"https://"

bool http_utils::get_addr(const char* url, char* domain, size_t size,
	unsigned short* pport)
{
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

	char buf[512];
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

//////////////////////////////////////////////////////////////////////////////

http_url::http_url(void) {
	ACL_SAFE_STRNCPY(proto_, "http", sizeof(proto_));
	port_ = 80;
}

void http_url::reset(void) {
	ACL_SAFE_STRNCPY(proto_, "http", sizeof(proto_));
	port_ = 80;
	domain_.clear();
	url_path_.clear();
	url_params_.clear();
}

bool http_url::parse(const char *url) {
	const char* ptr;

	if (!strncasecmp(url, HTTP_PREFIX, sizeof(HTTP_PREFIX) - 1)) {
		ptr = url + sizeof(HTTP_PREFIX) - 1;
	} else if (!strncasecmp(url, HTTPS_PREFIX, sizeof(HTTPS_PREFIX) - 1)) {
		ptr = url + sizeof(HTTPS_PREFIX) - 1;
		port_ = 443;
		ACL_SAFE_STRNCPY(proto_, "https", sizeof(proto_));
	} else if (*url == '/'){
		ptr = url;
	} else {
		logger_error("invalid url: %s", url);
		return false;
	}

	if (*ptr == 0) {
		logger_error("invalid url: %s", url);
		return false;
	}

	if (ptr == url) {
		// 说明是仅含相对路径的 url
		return parse_url_part(url);
	} else {
		// 说明包含有完整路径的 url，下面先提取域名字段，再提取相对 url
		ptr = parse_domain(ptr);
		if (ptr == NULL) {
			url_path_ = "/";
			return true;
		}
		return parse_url_part(ptr);
	}
}

#define SKIP_WHILE(cond, ptr) { while(*ptr && (cond)) ptr++; }

bool http_url::parse_url_part(const char *url) {
	if (*url != '/') {
		logger_error("invalid url: %s", url);
		return false;
	}

	const char* ptr = url;
	SKIP_WHILE(*ptr == '/', ptr);
	if (*ptr == 0) {
		url_path_ = "/";
		return true;
	}

	const char* qm = strchr(ptr, '?');
	if (qm == NULL) {
		url_path_ = url;
		return true;
	}
	url_path_.copy(url, qm - url);
	++qm;
	if (*qm != 0) {
		url_params_ = qm;
	}
	return true;
}

const char* http_url::parse_domain(const char *url) {
	if (*url == '/') {
		logger_error("invalid url: %s", url);
		return NULL;
	}
	const char* ptr = strchr(url, '/');
	if (ptr == NULL) {
		domain_ = url;
		return NULL;
	}

	char buf[256];
	size_t size = ptr - url + 1;
	if (size > sizeof(buf)) { // xxx: sanity check
		size = sizeof(buf);
	}
	ACL_SAFE_STRNCPY(buf, url, size);

	// fixme: Is it error if buf contains IPV6 Addr ---zsx
	char* col = strchr(buf, ':');
	if (col != NULL) {
		*col++ = 0;
		port_ = (unsigned short) atoi(col);
		if (port_ == 0 || port_ == 65535) {
			port_ = 80;
		}
	}
	domain_ = buf;
	return ptr;
}

} // namespace acl
