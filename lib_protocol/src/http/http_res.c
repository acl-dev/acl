#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/lib_http.h"

HTTP_RES *http_res_new(HTTP_HDR_RES *hdr_res)
{
	char  myname[] = "http_res_new";
	HTTP_RES *respond;

	respond = (HTTP_RES*) acl_mycalloc(1, sizeof(HTTP_RES));
	if (respond == NULL) {
		char ebuf[256];
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	}
	respond->hdr_res = hdr_res;
	return (respond);
}

void http_res_free(HTTP_RES *respond)
{
	if (respond) {
		if (respond->hdr_res)
			http_hdr_res_free(respond->hdr_res);
		if (respond->ctx && respond->free_ctx)
			respond->free_ctx(respond->ctx);
		acl_myfree(respond);
	}
}

