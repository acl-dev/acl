#ifndef	__HTTP_SERVICE_INCLUDE_H__
#define	__HTTP_SERVICE_INCLUDE_H__

#include "lib_acl.h"
#include "lib_protocol.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* in http_service.c */

/**
 * HTTP 协议方式处理方法
 * @param client {ACL_VSTREAM*} 客户端流
 * @return {int} 0：表示正常，1：表示正常且保持长连接，-1：表示出错
 */
int http_service(ACL_VSTREAM *client);

/**
 * 服务端返回 HTTP 响应给客户端
 * @param client {ACL_VSTREAM*} 客户端流
 * @param status {int} HTTP 响应状态码，1xx, 2xx, 3xx, 4xx, 5xx
 * @param keep_alive {int} 是否与客户端保持长连接
 * @param body {const char*} 数据体内容
 * @param len {int} 数据体长度
 */
int http_server_send_respond(ACL_VSTREAM* client, int status,
	int keep_alive, char* body, int len);

/* in http_json.c */

/**
 * 请求数据的格式为 JSON 格式的处理
 * @param client {ACL_VSTREAM*}
 * @param hdr_req {HTTP_HDR_REQ*} HTTP 请求协议头对象
 * @param json {ACL_JSON*} json 解析器对象
 * @return {int} 0：表示正常，1：表示正常且保持长连接，-1：表示出错
 */
int http_json_service(ACL_VSTREAM *client,
	HTTP_HDR_REQ *hdr_req, ACL_JSON *json);

/* in http_xml.c */

/**
 * 请求数据的格式为 XML 格式的处理
 * @param client {ACL_VSTREAM*}
 * @param hdr_req {HTTP_HDR_REQ*} HTTP 请求协议头对象
 * @param xml {ACL_XML*} xml 解析器对象
 * @return {int} 0：表示正常，1：表示正常且保持长连接，-1：表示出错
 */
int http_xml_service(ACL_VSTREAM *client,
        HTTP_HDR_REQ *hdr_req, ACL_XML *xml);

#ifdef	__cplusplus
}
#endif

#endif
