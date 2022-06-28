#include "lib_acl.h"
#include "lib_protocol.h"

#include "global.h"
#include "http_service.h"

/* 将数据返回给 HTTP 客户端 */

int http_server_send_respond(ACL_VSTREAM* client, int status,
	int keep_alive, char* body, int len)
{
	int   ret;
	struct iovec vector[2];  /* 数据数组 */
	ACL_VSTRING* buf = acl_vstring_alloc(256);
	HTTP_HDR_RES* hdr_res = http_hdr_res_static(status);

	/* 在请求头中设置请求体的数据长度 */
	http_hdr_put_int(&hdr_res->hdr, "Content-Length", len);
	/* 设置长连接选项 */
	http_hdr_put_str(&hdr_res->hdr, "Connection",
		keep_alive ? "keep-alive" : "close");

	/* 构建 HTTP 响应头数据 */
	http_hdr_build(&hdr_res->hdr, buf);

	/* 设置 HTTP 头 */
	vector[0].iov_base = acl_vstring_str(buf);
	vector[0].iov_len = ACL_VSTRING_LEN(buf);
	/* 设置 HTTP 体 */
	vector[1].iov_base = body;
	vector[1].iov_len = len;

	/* 发送响应头及响应体 */
	ret = acl_vstream_writevn(client, vector, 2);
	/* 释放 HTTP 响应头对象 */
	http_hdr_res_free(hdr_res);
	/* 释放缓冲区 */
	acl_vstring_free(buf);

	if (ret == ACL_VSTREAM_EOF)
		return -1;

	/* 发送HTTP响应成功 */
	return 0;
}

/* HTTP 协议处理过程入口 */

int http_service(ACL_VSTREAM *client)
{
	HTTP_HDR_REQ *hdr_req = http_hdr_req_new();
	HTTP_REQ *req;
	char  buf[4096];
	int   ret, json_fmt;
	const char *ptr;

	/* 读取HTTP请求头 */
	ret = http_hdr_req_get_sync(hdr_req, client, var_cfg_io_timeout);
	if (ret < 0) {
		http_hdr_req_free(hdr_req);
		return (-1);
	}

	/* 分析HTTP请求头 */
	if (http_hdr_req_parse(hdr_req) < 0) {
		http_hdr_req_free(hdr_req);
		acl_msg_error("%s(%d), %s: http request header invalid",
			__FILE__, __LINE__, __FUNCTION__);
		return (-1);
	}

	/* 必须保证数据体长度 > 0  */
	if (hdr_req->hdr.content_length <= 0) {
		http_hdr_req_free(hdr_req);
		acl_msg_error("%s(%d), %s: http request header invalid",
			__FILE__, __LINE__, __FUNCTION__);
		return (-1);
	}

	/* 从 HTTP 请求头中获取请求体中的数据格式：XML 或 JSON 格式 */
	ptr = http_hdr_entry_value(&hdr_req->hdr, "x-gid-format");
	if (ptr != NULL && strcasecmp(ptr, "xml") == 0)
		json_fmt = 0;
	else
		json_fmt = 1;

	req = http_req_new(hdr_req);  /* 创建HTTP请求对象 */

	if (json_fmt) {
		ACL_JSON *json = acl_json_alloc();  /* 创建JSON解析器对象 */

		/* 不断从客户端连接中读取数据，并放入JSON解析器中进行解析 */
		while (1) {
			ret = http_req_body_get_sync(req, client,
					buf, sizeof(buf) - 1);
			if (ret < 0) {
				/* 说明没有读到所要求的数据长度，表明出错 */
				http_req_free(req);
				acl_json_free(json);
				return (-1);
			} else if (ret == 0)  /* 表明已经读完了所有的数据 */
				break;
			buf[ret] = 0;
			acl_json_update(json, buf);
		}

		ret = http_json_service(client, hdr_req, json);
		acl_json_free(json);
	} else {
		ACL_XML *xml = acl_xml_alloc();  /* 创建XML解析器对象 */

		while (1) {
			ret = http_req_body_get_sync(req, client,
					buf, sizeof(buf) - 1);
			if (ret < 0) {
				http_req_free(req);
				acl_xml_free(xml);
				return (-1);
			} else if (ret == 0)
				break;
			buf[ret] = 0;
			acl_xml_update(xml, buf);
		}

		ret = http_xml_service(client, hdr_req, xml);
		acl_xml_free(xml);
	}

	http_req_free(req);
	return (ret);
}
