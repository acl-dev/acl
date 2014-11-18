#include "lib_acl.h"
#include "lib_protocol.h"
#include "http_plugin.h"

int http_request_filter(ACL_VSTREAM *client acl_unused, HTTP_HDR_REQ *hdr_req acl_unused, void **ctx_ptr acl_unused)
{
	/*
	   acl_msg_info("%s(%d): be called now", __FUNCTION__, __LINE__);
	   if (__logfp)
	   http_hdr_fprint(__logfp, &hdr_req->hdr, "http_request_filter");
	 */

	/* ¹Ø±ÕÑ¹ËõÇëÇó */
	http_hdr_entry_off(&hdr_req->hdr, "Accept-Encoding");
	return (0);
}

void http_request_forward(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void *ctx)
{
	(void) client;
	(void) hdr_req;
	(void) ctx;
}
