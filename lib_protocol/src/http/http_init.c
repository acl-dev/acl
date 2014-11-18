#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>

#include "http/lib_http_struct.h"
#include "http/lib_http.h"
#include "http.h"

http_off_t var_http_buf_size = HTTP_BSIZE;
int   var_http_tls_cache = 50;

void http_buf_size_set(http_off_t size)
{
	if (size > 1024)
		var_http_buf_size = size;
}

http_off_t http_buf_size_get(void)
{
	return (var_http_buf_size);
}

void http_init(const char *tmpl_path)
{
	const char *path = tmpl_path ? tmpl_path : "/opt/acl/httpd/pub";

	http_tmpl_load(path);
}

void http_hdr_cache(int max)
{
	var_http_tls_cache = max;
}
