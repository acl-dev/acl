#ifndef	__LIB_HTTP_UTIL_INCLUDE_H__
#define	__LIB_HTTP_UTIL_INCLUDE_H__

#include "lib_http_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct HTTP_UTIL {
	HTTP_HDR_REQ *hdr_req;		/* HTTP request header */
	HTTP_HDR_RES *hdr_res;		/* HTTP response header */
	HTTP_RES *http_res;		/* HTTP response object */
	char  server_addr[256];		/* Remote HTTP server address */
	ACL_VSTREAM *stream;		/* Stream for communicating with HTTP server */
	int   conn_timeout;		/* Timeout for connecting to HTTP server */
	int   rw_timeout;		/* IO timeout during HTTP communication */
	ACL_VSTRING *req_buf;		/* Request buffer */
	int   res_body_dlen;		/* Length of HTTP response body */
	ACL_VSTREAM *dump_stream;	/* Stream for dumping response data */
	unsigned int   flag;		/* Flag bits */
#define	HTTP_UTIL_FLAG_SET_DUMP_FILE	(1 << 0)	/* Set dump response file */
#define	HTTP_UTIL_FLAG_SET_DUMP_STREAM	(1 << 1)	/* Set dump response stream */
#define	HTTP_UTIL_FLAG_HAS_RES_BODY	(1 << 2)	/* Has HTTP response body */
#define	HTTP_UTIL_FLAG_NO_RES_BODY	(1 << 3)	/* No HTTP response body */
} HTTP_UTIL;

/**
 * Create an HTTP_UTIL request object
 * @param url {const char*} Request url
 * @param method {const char*} Request method. Valid methods:
 *  GET, POST, HEAD, CONNECT
 * @return {HTTP_UTIL*}
 */
HTTP_API HTTP_UTIL *http_util_req_new(const char *url, const char *method);

/**
 * Create an HTTP_UTIL response object
 * @param status {int} Status code. Valid status codes:
 *  1xx, 2xx, 3xx, 4xx, 5xx
 * @return {HTTP_UTIL*}
 */
HTTP_API HTTP_UTIL *http_util_res_new(int status);

/**
 * Free an HTTP_UTIL object
 * @param http_util {HTTP_UTIL*}
 */
HTTP_API void http_util_free(HTTP_UTIL *http_util);

/**
 * Set HTTP request header info, e.g.: Accept-Encoding: gzip,deflate
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Request header field name, e.g. Accept-Encoding
 * @param value {const char*} Request header field value, e.g. gzip,deflate
 */
HTTP_API void http_util_set_req_entry(HTTP_UTIL *http_util, const char *name, const char *value);

/**
 * Disable a specific field in HTTP request header, this field won't be
 * sent to server
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Request header field name, e.g. Accept-Encoding
 */
HTTP_API void http_util_off_req_entry(HTTP_UTIL *http_util, const char *name);

/**
 * Get value of a field from request header
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Request header field name, e.g. Accept-Encoding
 * @return {char*} May be NULL if field value doesn't exist or field
 *  doesn't exist
 */
HTTP_API char *http_util_get_req_value(HTTP_UTIL *http_util, const char *name);

/**
 * Get HTTP_HDR_ENTRY object for a field from request header
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Request header field name, e.g. Accept-Encoding
 * @return {HTTP_HDR_ENTRY*} NULL indicates field doesn't exist
 */
HTTP_API HTTP_HDR_ENTRY *http_util_get_req_entry(HTTP_UTIL *http_util, const char *name);

/**
 * Set HTTP request body data length in request header
 * @param http_util {HTTP_UTIL*}
 * @param len {int} HTTP request body length (must be >= 0)
 */
HTTP_API void http_util_set_req_content_length(HTTP_UTIL *http_util, int len);

/**
 * Set HTTP session keep-alive connection duration in request header (seconds)
 * @param http_util {HTTP_UTIL*}
 * @param timeout {int} HTTP connection duration (in seconds)
 */
HTTP_API void http_util_set_req_keep_alive(HTTP_UTIL *http_util, int timeout);

/**
 * Set Connection field in request header
 * @param http_util {HTTP_UTIL*}
 * @param value {const char*} Field value. Valid fields: keep-alive, close
 */
HTTP_API void http_util_set_req_connection(HTTP_UTIL *http_util, const char *value);

/**
 * Set Referer field in request header
 * @param http_util {HTTP_UTIL*}
 * @param refer {const char*} Referrer url, e.g.: http://www.test.com
 */
HTTP_API void http_util_set_req_refer(HTTP_UTIL *http_util, const char *refer);

/**
 * Set Cookie field in request header, uses append method when called
 * multiple times
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Cookie name
 * @param value {const char*} Cookie value
 */
HTTP_API void http_util_set_req_cookie(HTTP_UTIL *http_util, const char *name, const char *value);

/**
 * Set HTTP proxy server address
 * @param http_util {HTTP_UTIL*}
 * @param proxy {const char*} Proxy server address. Valid formats:
 *  IP:PORT, DOMAIN:PORT,
 *  e.g.: 192.168.0.1:80, 192.168.0.2:8088, www.g.cn:80
 */
HTTP_API void http_util_set_req_proxy(HTTP_UTIL *http_util, const char *proxy);

/**
 * Set HTTP response dump stream. After setting, HTTP response body data
 * will be simultaneously dumped
 * @param http_util {HTTP_UTIL*}
 * @param stream {ACL_VSTREAM *} Dump stream
 */
HTTP_API void http_util_set_dump_stream(HTTP_UTIL *http_util, ACL_VSTREAM *stream);

/**
 * Set HTTP response dump file. After setting, HTTP response body data
 * will be dumped to this file
 * @param http_util {HTTP_UTIL*}
 * @param filename {const char*} Dump filename
 * @return {int} Return value < 0 indicates unable to open file,
 *  otherwise indicates file opened successfully
 */
HTTP_API int http_util_set_dump_file(HTTP_UTIL *http_util, const char *filename);

/**
 * Connect to remote HTTP server, send HTTP request header data and parse
 * response, then close connection and return result
 * @param http_util {HTTP_UTIL*}
 * @return {int} 0: success; -1: unable to establish connection or send
 *  header failed
 */
HTTP_API int http_util_req_open(HTTP_UTIL *http_util);

/**
 * When using POST request, send HTTP request body data to server through
 * this function. Can repeatedly call this function in a loop until all
 * data sent
 * @param http_util {HTTP_UTIL*}
 * @param data {const char*} Address of data to send each time, may be NULL
 * @param dlen {size_t} data length, must not be 0
 * @return {int} > 0 indicates number of bytes successfully sent this time;
 *  -1: indicates network failure, should call http_util_free to close
 *  connection and free memory resources
 */
HTTP_API int http_util_put_req_data(HTTP_UTIL *http_util, const char *data, size_t dlen);

/**
 * After sending all request data, call this function to read HTTP response
 * header from HTTP server
 * @param http_util {HTTP_UTIL*}
 * @return {int} 0: success; -1: failure
 */
HTTP_API int http_util_get_res_hdr(HTTP_UTIL *http_util);

/**
 * Get a field value from HTTP response header
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Field name, e.g. Content-Length
 * @return {char*} Field value corresponding to name, NULL indicates field
 *  doesn't exist
 */
HTTP_API char *http_util_get_res_value(HTTP_UTIL *http_util, const char *name);

/**
 * Get a field object from HTTP response header
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Field name, e.g. Content-Length
 * @return {HTTP_HDR_ENTRY*} Field object corresponding to name, NULL
 *  indicates field doesn't exist
 */
HTTP_API HTTP_HDR_ENTRY *http_util_get_res_entry(HTTP_UTIL *http_util, const char *name);

/**
 * Set a field value in HTTP response header
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Field name, e.g. Content-Type
 * @param value {const char*} Field value, e.g. text/html
 */
HTTP_API void http_util_set_res_entry(HTTP_UTIL *http_util, const char *name, const char *value);

/**
 * Disable a field in HTTP response header
 * @param http_util {HTTP_UTIL*}
 * @param name {const char*} Field name, e.g. Content-Type
 */
HTTP_API void http_util_off_res_entry(HTTP_UTIL *http_util, const char *name);

/**
 * After getting HTTP response header, call this function to check if
 * HTTP response body exists
 * @param http_util {HTTP_UTIL*}
 * @return {int}  0: indicates no response body; !0: indicates has body
 */
HTTP_API int http_util_has_res_body(HTTP_UTIL *http_util);

/**
 * After getting HTTP response header, call this function to read HTTP
 * response body data from HTTP server. Need to repeatedly call this
 * function until return value <= 0. If dump file or stream was set before,
 * during data reading process, simultaneously copy data to dump file or
 * stream
 * @param http_util {HTTP_UTIL*}
 * @param buf {char *} Buffer to store HTTP response body
 * @param size {size_t} Size of buf space
 * @return {int} <= 0: indicates read complete; > 0: indicates data length
 *  read this time
 */
HTTP_API int http_util_get_res_body(HTTP_UTIL *http_util, char *buf, size_t size);

/**
 * Dump response body from a url to a file
 * @param url {const char*} Request url, e.g.: http://www.g.cn
 * @param dump {const char*} Dump filename
 * @return {int} Returns response body data length, >=0: success,
 *  -1: failure
 */
HTTP_API int http_util_dump_url(const char *url, const char *dump);

/**
 * Dump response body from a url to a stream
 * @param url {const char*} Request url, e.g.: http://www.g.cn
 * @param stream {ACL_VSTREAM *} Dump stream
 * @return {int} Returns response body data length, >=0: success,
 *  -1: failure
 */
HTTP_API int http_util_dump_url_to_stream(const char *url, ACL_VSTREAM *stream);

#ifdef	__cplusplus
}
#endif

#endif

