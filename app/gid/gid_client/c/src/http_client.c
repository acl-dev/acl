#include "lib_acl.h"
#include "lib_protocol.h"

#include "lib_gid.h"
#include "global.h"
#include "http_client.h"

int http_client_post_request(ACL_VSTREAM *client, const char *url, int keepalive,
	const char *gid_fmt, char* body, int len, int *errnum)
{
	HTTP_HDR_REQ* hdr_req = http_hdr_req_create(url, "POST", "HTTP/1.1");
	ACL_VSTRING* buf = acl_vstring_alloc(256);
	struct iovec vector[2];  /* 数据数组 */
	int   ret;

	/* 在请求头中设置请求体的数据长度 */
	http_hdr_put_int(&hdr_req->hdr, "Content-Length", len);
	if (keepalive)
		http_hdr_put_str(&hdr_req->hdr, "Connection", "keep-alive");
	if (gid_fmt && *gid_fmt)
		http_hdr_put_str(&hdr_req->hdr, "x-gid-format", gid_fmt);
	http_hdr_build_request(hdr_req, buf);  /* 构建 HTTP 请求头数据 */

	/* 设置 HTTP 头 */
	vector[0].iov_base = acl_vstring_str(buf);
	vector[0].iov_len = ACL_VSTRING_LEN(buf);

	/* 设置 HTTP 体 */
	vector[1].iov_base = (char*) body;
	vector[1].iov_len = len;

	ret = acl_vstream_writevn(client, vector, 2);  /* 发送请求头及请求体 */
	http_hdr_req_free(hdr_req);  /* 释放 HTTP 请求头对象 */
	acl_vstring_free(buf);  /* 释放缓冲区 */

	if (ret == ACL_VSTREAM_EOF) {
		if (errnum)
			*errnum = GID_ERR_IO;
		return -1;
	}

	/* 发送HTTP请求成功 */
	return (0);
}

int http_client_get_respond(ACL_VSTREAM* client, ACL_JSON *json,
	ACL_XML *xml, int *errnum, ACL_VSTRING *dump)
{
	HTTP_HDR_RES* hdr_res;
	HTTP_RES* res;
	char  buf[1024];
	int   ret;

	acl_assert(json != NULL || xml != NULL);

	hdr_res = http_hdr_res_new();  /* 创建HTTP 响应头对象 */
	/* 读取 HTTP 服务器响应头 */
	ret = http_hdr_res_get_sync(hdr_res, client, var_gid_rw_timeout);
	if (ret < 0) {
		http_hdr_res_free(hdr_res); /* 释放 HTTP 响应头对象 */
		return -1;
	}
	/* 解析 HTTP 响应头 */
	if (http_hdr_res_parse(hdr_res) < 0) {  /* 解析出错 */
		http_hdr_res_free(hdr_res);
		return -1;
	}

	/*
	http_hdr_print(&hdr_res->hdr, "---respond---");
	*/

	/* 需要先根据 HTTP 响应头判断是否有数据体 */
	if (hdr_res->hdr.content_length == 0  ||
		(hdr_res->hdr.content_length == -1 && !hdr_res->hdr.chunked  
		&& hdr_res->reply_status > 300  && hdr_res->reply_status < 400)) 
	{
		http_hdr_res_free(hdr_res);
		return 0;  
	}
	res = http_res_new(hdr_res);   /* 创建 HTTP 响应体对象 */
	while (1) {
		/* 读数据体数据 */
		ret = http_res_body_get_sync(res, client, buf, sizeof(buf) - 1);
		if (ret <= 0)
			break;
		buf[ret] = 0;
		if (json)
			acl_json_update(json, buf);
		else
			acl_xml_update(xml, buf);
		if (dump)
			acl_vstring_strcat(dump, buf);
	}

	/* 因为 res 中含有 hdr_res 所以会一同连 hdr_res 释放 */
	http_res_free(res);
	return (0);
}
