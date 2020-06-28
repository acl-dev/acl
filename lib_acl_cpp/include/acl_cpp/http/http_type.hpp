#pragma once

namespace acl {

struct HTTP_PARAM 
{
	char* name;
	char* value;
};

// HTTP 响应状态
typedef enum
{
	HTTP_OK,                // 一切正常
	HTTP_ERR_DNS,           // 域名解析失败
	HTTP_ERR_CONN,          // 连接服务器失败
	HTTP_ERR_REQ,           // 创建请求协议失败
	HTTP_ERR_READ,          // 读数据失败
	HTTP_ERR_SEND,          // 写数据失败
	HTTP_ERR_TIMO,          // 读写数据超时
	HTTP_ERR_READ_HDR,      // 读 HTTP 响应头失败
	HTTP_ERR_READ_BODY,     // 读 HTTP 响应体失败
	HTTP_ERR_INVALID_HDR,   // HTTP 响应头无效
	HTTP_ERR_UNKNOWN,       // 出现了未知错误
	HTTP_ERR_REDIRECT_MAX,	// HTTP 响应头中重定向次数太多
} http_status_t;

// HTTP 请求方法
typedef enum
{
	HTTP_METHOD_UNKNOWN,    // 未知方法
	HTTP_METHOD_GET,        // GET 方法
	HTTP_METHOD_POST,       // POST 方法
	HTTP_METHOD_PUT,        // PUT 方法
	HTTP_METHOD_CONNECT,    // CONNECT 方法
	HTTP_METHOD_PURGE,      // PURGE 方法
	HTTP_METHOD_DELETE,     // DELETE 方法
	HTTP_METHOD_HEAD,       // HEAD 方法
	HTTP_METHOD_OPTION,     // OPTION 方法
	HTTP_METHOD_PROPFIND,	// PROPFIND 方法
	HTTP_METHOD_PATCH,	// PATCH 方法
	HTTP_METHOD_OTHER,	// 其它的方法
} http_method_t;

typedef enum
{
	// Content-Type: application/x-www-form-urlencoded
	HTTP_REQUEST_NORMAL,

	// Content-Type: multipart/form-data; boundary=xxx
	HTTP_REQUEST_MULTIPART_FORM,

	// Content-Type: application/octet-stream
	HTTP_REQUEST_OCTET_STREAM,

	// Content-Type: text/xml 或 application/xml
	HTTP_REQUEST_TEXT_XML,

	// Content-Type: text/json 或 application/json
	HTTP_REQUEST_TEXT_JSON,

	// 其它类型
	HTTP_REQUEST_OTHER
} http_request_t;

typedef enum
{
	// ok
	HTTP_REQ_OK,

	// network io error
	HTTP_REQ_ERR_IO,

	// invalid request method
	HTTP_REQ_ERR_METHOD
} http_request_error_t;

typedef enum
{
	HTTP_MIME_PARAM,        // http mime 结点为参数类型
	HTTP_MIME_FILE          // http mime 结点为文件类型
} http_mime_t;

} // namespace acl end
