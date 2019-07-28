#include "lib_acl.h"
#include "lib_protocol.h"
#include "http_plugin.h"

int http_request_filter(ACL_VSTREAM *client acl_unused,
	HTTP_HDR_REQ *hdr_req, void **ctx_ptr)
{
	const char *url_path = http_hdr_req_url_path(hdr_req);
	CGI *cgi;

	/*
	acl_msg_info("%s(%d): be called now", __FUNCTION__, __LINE__);
	if (__logfp)
		http_hdr_fprint(__logfp, &hdr_req->hdr, "http_request_filter");
	*/

	if (url_path == NULL)
		return (0);

	cgi = http_cgi_path(url_path);
	if (cgi == NULL)
		return (0);

	*ctx_ptr = cgi;
	return (1);
}

void http_request_forward(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void *ctx)
{
	CGI *cgi = (CGI*) ctx;

	cgi->client = client;
	cgi->hdr_req = hdr_req;
	http_plugin_pool_append(http_cgi_thread, cgi);
}
