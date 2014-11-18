#include "lib_acl.h"
#include "http_service.h"

static int var_http_filter;

int http_filter_type(void)
{
	return (var_http_filter);
}

void http_filter_set(const char *filter_info)
{
	const char *myname = "http_filter_set";

	if (strcasecmp(filter_info, "HTTP_FILTER_PROXY") == 0)
		var_http_filter = HTTP_FILTER_PROXY;
	else if (strcasecmp(filter_info, "HTTP_FILTER_HTTPD") == 0)
		var_http_filter = HTTP_FILTER_HTTPD;
	else {
		acl_msg_warn("%s(%d): unknown http filter(%s)",
			myname, __LINE__, filter_info);
		var_http_filter = HTTP_FILTER_HTTPD;
	}
}
