#ifndef	__LIB_HTTP_UTIL_INCLUDE_H__
#define	__LIB_HTTP_UTIL_INCLUDE_H__

#include "lib_http_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct HTTP_UTIL {
	HTTP_HDR_REQ *hdr_req;		/**< HTTP 请求头 */
	HTTP_HDR_RES *hdr_res;		/**< HTTP 响应头 */
	HTTP_RES *http_res;		/**< HTTP 响应体 */
	char  server_addr[256];		/**< 远程 HTTP 服务器地址 */
	ACL_VSTREAM *stream;		/**< 与 HTTP 服务器建立的网络流 */
	int   conn_timeout;		/**< 连接 HTTP 服务器的超时时间 */
	int   rw_timeout;		/**< 与 HTTP 服务器通信时每次 IO 的超时时间 */
	ACL_VSTRING *req_buf;		/**< 缓冲区 */
	int   res_body_dlen;		/**< HTTP 响应数据体的长度 */
	ACL_VSTREAM *dump_stream;	/**< 转储接收数据的流 */
	unsigned int   flag;		/**< 标志位 */
#define	HTTP_UTIL_FLAG_SET_DUMP_FILE	(1 << 0)	/**< 允许转储响应体至文件 */
#define	HTTP_UTIL_FLAG_SET_DUMP_STREAM	(1 << 1)	/**< 允许转储响应体至流 */
#define	HTTP_UTIL_FLAG_HAS_RES_BODY	(1 << 2)	/**< 有 HTTP 响应体 */
#define	HTTP_UTIL_FLAG_NO_RES_BODY	(1 << 3)	/**< 无 HTTP 响应体 */
} HTTP_UTIL;

/**
 * 创建一个 HTTP_UTIL 请求对象
 * @param url {const char*} 完整的请求 url
 * @param method {const char*} 请求方法，有效的请求方法有：GET, POST, HEAD, CONNECT
 * @return {HTTP_UTIL*}
 */
HTTP_API HTTP_UTIL *http_util_req_new(const char *url, const char *method);

/**
 * 构建一个 HTTP_UTIL 响应对象
 * @param status {int} 状态码，有效的状态码为: 1xx, 2xx, 3xx, 4xx, 5xx
 * @return {HTTP_UTIL*}
 */
HTTP_API HTTP_UTIL *http_util_res_new(int status);

/**
 * 释放一个 HTTP_UTIL 对象
 * @param http_util {HTTP_UTIL*}
 */
HTTP_API void http_util_free(HTTP_UTIL *http_util);

/**
 * 设置 HTTP 请求头信息, 如: Accept-Encoding: gzip,deflate
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 请求头中字段名称, 如 Accept-Encoding
 * @param value {const char*} 请求头中字段的值, 如 gzip,deflate
 */
HTTP_API void http_util_set_req_entry(HTTP_UTIL *http_util, const char *name, const char *value);

/**
 * 关闭 HTTP 请求头中的某个请求字段，该请求字段不会发往服务器
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 请求头中字段名称, 如 Accept-Encoding
 */
HTTP_API void http_util_off_req_entry(HTTP_UTIL *http_util, const char *name);

/**
 * 获得请求头中某个字段的值
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 请求头中字段名称, 如 Accept-Encoding
 * @return {char*} 如果非空则为请求字段值，否则表明该字段不存在
 */
HTTP_API char *http_util_get_req_value(HTTP_UTIL *http_util, const char *name);

/**
 * 获得请求头中某个字段的 HTTP_HDR_ENTRY 对象
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 请求头中字段名称, 如 Accept-Encoding
 * @return {HTTP_HDR_ENTRY*} 若为空则表示该字段不存在
 */
HTTP_API HTTP_HDR_ENTRY *http_util_get_req_entry(HTTP_UTIL *http_util, const char *name);

/**
 * 设置请求头中 HTTP 数据体的数据长度
 * @param http_util {HTTP_UTIL*}
 * @param len {int} HTTP 数据体长度(必须 >= 0)
 */
HTTP_API void http_util_set_req_content_length(HTTP_UTIL *http_util, int len);

/**
 * 设置请求头中 HTTP 会话保持长连接的存活时间(单位为秒)
 * @param http_util {HTTP_UTIL*}
 * @param timeout {int} HTTP 长连接的存活时间(单位为秒)
 */
HTTP_API void http_util_set_req_keep_alive(HTTP_UTIL *http_util, int timeout);

/**
 * 设置请求头中 Connection 字段
 * @param http_util {HTTP_UTIL*}
 * @param value {const char*} 字段值，有效的字段为: keep-alive, close
 */
HTTP_API void http_util_set_req_connection(HTTP_UTIL *http_util, const char *value);

/**
 * 设置请求头中的 Referer 字段
 * @param http_util {HTTP_UTIL*}
 * @param refer {const char*} 完整的 url, 如: http://www.test.com
 */
HTTP_API void http_util_set_req_refer(HTTP_UTIL *http_util, const char *refer);

/**
 * 设置请求头中的 Cookie 字段，采用的是追加方式
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Cookie 名称
 * @param value {const char*} Cookie 值
 */
HTTP_API void http_util_set_req_cookie(HTTP_UTIL *http_util, const char *name, const char *value);

/**
 * 设置 HTTP 代理服务器地址
 * @param http_util {HTTP_UTIL*}
 * @param proxy {const char*} 代理服务器地址，有效格式为: IP:PORT, DOMAIN:PORT,
 *  如: 192.168.0.1:80, 192.168.0.2:8088, www.g.cn:80
 */
HTTP_API void http_util_set_req_proxy(HTTP_UTIL *http_util, const char *proxy);

/**
 * 设置 HTTP 响应体的转储流，设置后 HTTP 响应体数据便会同时向该流转发
 * @param http_util {HTTP_UTIL*}
 * @param stream {ACL_VSTREAM *} 转储流
 */
HTTP_API void http_util_set_dump_stream(HTTP_UTIL *http_util, ACL_VSTREAM *stream);

/**
 * 设置 HTTP 响应体的转储文件，设置后 HTTP 响应体数据便会转储于该文件
 * @param http_util {HTTP_UTIL*}
 * @param filename {const char*} 转储文件名
 * @return {int} 如果返回值 < 0 则表示无法打开该文件, 否则表示打开文件成功
 */
HTTP_API int http_util_set_dump_file(HTTP_UTIL *http_util, const char *filename);

/**
 * 打开远程 HTTP 服务器或代理服务器连接，同时构建 HTTP 请求头数据并且将该数据
 * 发给新建立的网络连接
 * @param http_util {HTTP_UTIL*}
 * @return {int} 0: 成功; -1: 无法打开连接或发送请求头数据失败
 */
HTTP_API int http_util_req_open(HTTP_UTIL *http_util);

/**
 * 当采用 POST 方法时，可以通过此函数向 HTTP 服务器或代理服务器发送请求体数据,
 * 在一个请求过程中，可以多次调用本函数直至发送完请求体数据
 * @param http_util {HTTP_UTIL*}
 * @param data {const char*} 本次发送的数据地址，必须非空
 * @param dlen {size_t} data 数据长度, 必须大于 0
 * @return {int} > 0 表示本次成功发送的数据; -1: 表示发送数据失败, 应调用
 *  http_util_free 关闭网络流且释放内存资源
 */
HTTP_API int http_util_put_req_data(HTTP_UTIL *http_util, const char *data, size_t dlen);

/**
 * 发送完请求数据后调用此函数从 HTTP 服务器读取完整的 HTTP 响应头
 * @param http_util {HTTP_UTIL*}
 * @return {int} 0: 成功; -1: 失败
 */
HTTP_API int http_util_get_res_hdr(HTTP_UTIL *http_util);

/**
 * 从 HTTP 响应头中获得某个字段值
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 字段名称, 如 Content-Length
 * @return {char*} 对应 name 的字段值, 如果为空则表示该字段不存在
 */
HTTP_API char *http_util_get_res_value(HTTP_UTIL *http_util, const char *name);

/**
 * 从 HTTP 响应头中获得某个字段对象
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 字段名称, 如 Content-Length
 * @return {HTTP_HDR_ENTRY*} 对应 name 的字段对象, 如果为空则表示该字段不存在
 */
HTTP_API HTTP_HDR_ENTRY *http_util_get_res_entry(HTTP_UTIL *http_util, const char *name);

/**
 * 设置 HTTP 响应头中的某个字段值
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 字段名称, 如 Content-Type
 * @param value {const char*} 字段值, 如 text/html
 */
HTTP_API void http_util_set_res_entry(HTTP_UTIL *http_util, const char *name, const char *value);

/**
 * 关闭 HTTP 响应头中的某个字段
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} 字段名称, 如 Content-Type
 */
HTTP_API void http_util_off_res_entry(HTTP_UTIL *http_util, const char *name);

/**
 * 读完 HTTP 响应头后调用此函数判断是否有 HTTP 响应体
 * @param http_util {HTTP_UTIL*}
 * @return {int}  0: 表示无响应体; !0: 表示有响应体
 */
HTTP_API int http_util_has_res_body(HTTP_UTIL *http_util);

/**
 * 读完 HTTP 响应头后调用此函数从 HTTP 服务器读取 HTTP 数据体数据，需要连续调用
 * 此函数，直至返回值 <= 0, 如果之前设置了转储文件或转储则在读取数据过程中同时会
 * 拷贝一份数据给转储文件或转储流
 * @param http_util {HTTP_UTIL*}
 * @param buf {char *} 存储 HTTP 响应体的缓冲区
 * @param size {size_t} buf 的空间大小
 * @return {int} <= 0: 表示读结束; > 0: 表示本次读到的数据长度
 */
HTTP_API int http_util_get_res_body(HTTP_UTIL *http_util, char *buf, size_t size);

/**
 * 将某个 url 的响应体数据转储至某个文件中
 * @param url {const char*} 完整请求 url, 如: http://www.g.cn
 * @param dump {const char*} 转储文件名
 * @return {int} 读到的响应体数据长度, >=0: 表示成功, -1: 表示失败
 */
HTTP_API int http_util_dump_url(const char *url, const char *dump);

/**
 * 将某个 url 的响应体数据转储至某个流中
 * @param url {const char*} 完整请求 url, 如: http://www.g.cn
 * @param stream {ACL_VSTREAM *} 转储流
 * @return {int} 读到的响应体数据长度, >=0: 表示成功, -1: 表示失败
 */
HTTP_API int http_util_dump_url_to_stream(const char *url, ACL_VSTREAM *stream);

#ifdef	__cplusplus
}
#endif

#endif
