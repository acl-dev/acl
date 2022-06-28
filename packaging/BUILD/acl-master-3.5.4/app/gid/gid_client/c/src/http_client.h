#ifndef	__HTTP_CLIENT_INCLUDE_H__
#define	__HTTP_CLIENT_INCLUDE_H__

/**
 * 向服务器按 POST 方式发送请求
 * @param client {ACL_VSTREAM*} 连接流
 * @param url {const char*} URL 字符串
 * @param keepalive {int} 是否与服务端保持长连接
 * @param gid_fmt {const char*} 数据格式：xml 或 json
 * @param body {char*} 数据体地址
 * @param len {int} 数据体长度
 * @param errnum {int*} 若非空则记录出错时的原因
 * @return {int} 0 表示成功，否则表示失败
 */
int http_client_post_request(ACL_VSTREAM *client, const char *url, int keepalive,
	const char *gid_fmt, char* body, int len, int *errnum);

/**
 * 从服务器读取响应数据
 * @param client {ACL_VSTREAM*} 连接流
 * @param json {ACL_JSON*} 若非空，则采用 json 格式进行解析
 * @param xml {ACL_XML*} 若非空，则采用 xml 格式进行解析
 * @param errnum {int*} 若非空则记录出错时的原因
 * @param dump {ACL_VSTRING*} 非空则存储响应数据
 * @return {int} 0 表示成功，否则表示失败
 * 注：ACL_JSON* 和 ACL_XML* 必须有且只有一个非空
 */
int http_client_get_respond(ACL_VSTREAM* client, ACL_JSON *json,
	ACL_XML *xml, int *errnum, ACL_VSTRING *dump);

#endif
