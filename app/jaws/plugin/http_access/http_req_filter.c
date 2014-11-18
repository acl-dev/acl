#include "lib_acl.h"
#include "lib_protocol.h"
#include "http_access.h"
#include "html_template.h"
#include "http_redirect.h"
#include "http_plugin.h"

int http_request_filter(ACL_VSTREAM *client acl_unused, HTTP_HDR_REQ *hdr_req, void **ctx_ptr)
{
	const char *myname = "http_request_filter";
	const char *domain = http_hdr_req_host(hdr_req);
	HTTP_DOMAIN_MAP *hdm;

	if (domain == NULL) {
		acl_msg_error("%s(%d): no host in request(%s)",
			myname, __LINE__, http_hdr_req_url(hdr_req));
		return (-403);
	}

	if (http_access_permit(domain)) {
		hdm = http_redirect_lookup(domain);
		if (hdm == NULL)
			return (0);
		*ctx_ptr = hdm;
		return (1);
	} else {
		acl_msg_info("%s(%d): domain(%s) denied!", myname, __LINE__, domain);
		return (-403);
	}
}

void http_request_forward(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void *ctx)
{
	const char *myname = "http_request_forward";
	HTTP_DOMAIN_MAP *hdm = (HTTP_DOMAIN_MAP*) ctx;
	ACL_VSTRING *buf;
	HTTP_HDR_RES *hdr_res;
	const char *host_ptr;
	int   n;

	if (hdm == NULL) {
		acl_msg_error("%s(%d): ctx null", NULL);
		acl_vstream_writen(client, HTTP_INTERNAL_ERROR, strlen(HTTP_INTERNAL_ERROR));
		acl_vstream_close(client);
		http_hdr_req_free(hdr_req);
		return;
	}

	host_ptr = http_hdr_req_host(hdr_req);
	if (acl_strrncmp(hdm->domain_from, host_ptr, hdm->size_from) != 0) {
		acl_msg_error("%s(%d): domain_from(%s) != host(%s)",
			myname, __LINE__, hdm->domain_from, host_ptr);
		acl_vstream_writen(client, HTTP_INTERNAL_ERROR, strlen(HTTP_INTERNAL_ERROR));
		acl_vstream_close(client);
		http_hdr_req_free(hdr_req);
		return;
	}

	buf = acl_vstring_alloc(256);

	/* 生成新的 url 地址 */
	n = (int) strlen(host_ptr) - hdm->size_from;
	acl_vstring_strcpy(buf, "http://");
	if (n > 0)
		acl_vstring_strncat(buf, host_ptr, n);
	acl_vstring_strcat(buf, hdm->domain_to);
	acl_vstring_strcat(buf, acl_vstring_str(hdr_req->url_part));

	/* 产生重定向头 */
	hdr_res = http_hdr_res_static(302);
	http_hdr_put_str(&hdr_res->hdr, "Location", acl_vstring_str(buf));
	http_hdr_put_str(&hdr_res->hdr, "Connection", "close");
	
	/* 生成响应数据包 */
	http_hdr_build(&hdr_res->hdr, buf);

	acl_vstream_writen(client, acl_vstring_str(buf), ACL_VSTRING_LEN(buf));
	acl_vstream_close(client);
	http_hdr_req_free(hdr_req);

	acl_vstring_free(buf);
	http_hdr_res_free(hdr_res);
}
