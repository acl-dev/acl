#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/lib_http.h"

HTTP_REQ *http_req_new(HTTP_HDR_REQ *hdr_req)
{
	char  myname[] = "http_req_new";
	HTTP_REQ *request;

	request = (HTTP_REQ*) acl_mycalloc(1, sizeof(HTTP_REQ));
	if (request == NULL) {
		char ebuf[256];
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	request->hdr_req = hdr_req;
	return (request);
}

void http_req_free(HTTP_REQ *request)
{
	if (request) {
		if (request->hdr_req)
			http_hdr_req_free(request->hdr_req);
		if (request->ctx && request->free_ctx)
			request->free_ctx(request->ctx);
		acl_myfree(request);
	}
}

