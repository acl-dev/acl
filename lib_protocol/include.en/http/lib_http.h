#ifndef	__LIB_HTTP_INCLUDE_H__
#define	__LIB_HTTP_INCLUDE_H__

#include "lib_http_status.h"
#include "lib_http_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*-------------------------- Common HTTP header functions -------------------*/
/* in http_hdr.c */

/**
 * Create a common HTTP protocol header structure
 * @param size {size_t} Pre-allocated memory size, for HTTP_HDR_REQ or
 *  HTTP_HDR_RES size
 * @return {HTTP_HDR*} !NULL: pointer to HTTP_HDR structure; NULL: error.
 */
HTTP_API HTTP_HDR *http_hdr_new(size_t size);

/**
 * Clone internal members from source HTTP common header, creating a new
 * HTTP common header structure
 * @param src {const HTTP_HDR*} Source HTTP common header, not NULL
 * @param dst {HTTP_HDR*} Destination HTTP common header, not NULL
 */
HTTP_API void http_hdr_clone(const HTTP_HDR *src, HTTP_HDR *dst);

/**
 * Free an HTTP_HDR structure memory
 * @param hh {HTTP_HDR*} Passed pointer, not NULL
 */
HTTP_API void http_hdr_free(HTTP_HDR *hh);

/**
 * Reset an HTTP common header to initial state and free internal members.
 * Mainly for reusing long-lived keep-alive connections
 * @param hh {HTTP_HDR*} HTTP common header pointer, not NULL
 */
HTTP_API void http_hdr_reset(HTTP_HDR *hh);

/**
 * Append an entry to HTTP_HDR header
 * @param hh {HTTP_HDR*} Common header pointer, not NULL
 * @param entry {HTTP_HDR_ENTRY*} HTTP header entry pointer, not NULL
 */
HTTP_API void http_hdr_append_entry(HTTP_HDR *hh, HTTP_HDR_ENTRY *entry);

/**
 * Parse protocol, major/minor version from data and store in common
 * HTTP header structure
 * @param hh {HTTP_HDR*} Passed pointer, not NULL
 * @param data {const char*} Data format: HTTP/1.0
 * @return {int} 0: OK; < 0: error.
 */
HTTP_API int http_hdr_parse_version(HTTP_HDR *hh, const char *data);

/**
 * Parse common HTTP protocol header from stream and store in hh structure
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @return {int} 0: ok; < 0: error
 */
HTTP_API int http_hdr_parse(HTTP_HDR *hh);

/**
 * Build an HTTP_HDR_ENTRY object from name and value strings
 * @param name {const char*} Entry name
 * @param value {const char*} Entry value
 * @return {HTTP_HDR_ENTRY*} Not NULL on success, NULL on error
 */
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_build(const char *name, const char *value);

/**
 * Parse a data line to create an HTTP_HDR_ENTRY
 * @param data {const char*} One line in HTTP protocol header,
 *  e.g.: Content-Length: 200
 * @return {HTTP_HDR_ENTRY*} !NULL: ok; NULL: err.
 */
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_new(const char *data);
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_head(char *data);
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry_new2(char *data);

/**
 * Get an HTTP_HDR_ENTRY entry
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param name {const char*} HTTP_HDR_ENTRY entry identifier, not NULL.
 *  e.g.: Content-Length.
 * @return ret {HTTP_HDR_ENTRY *} ret != NULL: ok; ret == NULL: not found.
 */
HTTP_API HTTP_HDR_ENTRY *http_hdr_entry(const HTTP_HDR *hh, const char *name);

/**
 * Get field value from HTTP protocol header. For example, if a field is
 * Host: www.test.com, to get value of Host field, call this function
 * to retrieve www.test.com
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param name {const char*} HTTP_HDR_ENTRY entry identifier, not NULL.
 *  e.g.: Content-Length
 * @return ret {char*} ret != NULL: ok; ret == NULL: not found.
 */
HTTP_API char *http_hdr_entry_value(const HTTP_HDR *hh, const char *name);

/**
 * Replace a field in HTTP header. This function is mainly used for
 * replacing keep-alive field
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param name {const char*} HTTP_HDR_ENTRY entry identifier, not NULL.
 *  e.g.: Content-Length
 * @param value {const char*} New value corresponding to name field
 * @param force {int} If field doesn't exist in original HTTP header,
 *  force insert new entry field and add to header. When value is non-zero,
 *  force insert. If name field doesn't exist, return error.
 * @return {int} 0 indicates successful replacement; < 0 indicates name
 *  field doesn't exist in HTTP request header
 */
HTTP_API int http_hdr_entry_replace(HTTP_HDR *hh, const char *name, const char *value, int force);

/**
 * Replace source string with target string in HTTP header field,
 * supports multiple matches and replacements
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param name {const char*} HTTP_HDR_ENTRY entry identifier, not NULL.
 *  e.g.: Cookie
 * @param from {const char*} Source string for replacement
 * @param to {const char*} Target string for replacement
 * @param ignore_case {int} Whether to ignore case when finding source string
 * @return {int} 0: no replacement made, > 0: number of replacements
 */
HTTP_API int http_hdr_entry_replace2(HTTP_HDR *hh, const char *name,
        const char *from, const char *to, int ignore_case);

/**
 * Disable a field in HTTP protocol header
 * @param hh {HTTP_HDR* } Common HTTP header pointer, not NULL
 * @param name {const char*} HTTP_HDR_ENTRY entry identifier, not NULL.
 *  e.g.: Content-Length
 */
HTTP_API void http_hdr_entry_off(HTTP_HDR *hh, const char *name);

/**
 * Print HTTP protocol header data, debugging interface
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param msg {const char*} User message to print before custom message,
 *  may be NULL
 */
HTTP_API void http_hdr_print(const HTTP_HDR *hh, const char *msg);

/**
 * Print HTTP protocol header data, debugging interface
 * @param fp {ACL_VSTREAM*} Stream pointer, output will be bound to this
 *  stream (can also be a file stream)
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param msg {const char*} User message to print before custom message,
 *  may be NULL
*/
HTTP_API void http_hdr_fprint(ACL_VSTREAM *fp, const HTTP_HDR *hh, const char *msg);

/**
 * Print HTTP protocol header data, debugging interface
 * @param bf {ACL_VSTRING*} Buffer to store output data
 * @param hh {HTTP_HDR*} Common HTTP header pointer, not NULL
 * @param msg {const char*} User message to print before custom message,
 *  may be NULL
*/
HTTP_API void http_hdr_sprint(ACL_VSTRING *bf, const HTTP_HDR *hh, const char *msg);

/*------------------------ HTTP request header functions --------------------*/
/* in http_hdr_req.c */

/**
 * Set flag to disable converting question mark '?' in HTTP request URI
 * (avoid converting to %3F), whether to enable automatic correction
 * @param onoff {int} When 0, disable automatic correction. Default is 1
 */
HTTP_API void http_uri_correct(int onoff);

/**
 * Create a request HTTP protocol header object
 * @return {HTTP_HDR_REQ*} HTTP request header object
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_new(void);

/**
 * Create an HTTP request header object from URL, method, and HTTP version
 * @param url {const char*} Request URL, complete URL format, e.g.:
 *  http://www.test.com/path/proc?name=value
 *  http://www.test.com/path/proc
 *  http://www.test.com/
 * @param method {const char*} HTTP request method, one of:
 *  GET, POST, CONNECT, HEAD, all uppercase
 * @param version {const char *} HTTP version, one of:
 *  HTTP/1.0, HTTP/1.1
 * @return {HTTP_HDR_REQ*} HTTP request header object
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_create(const char *url,
		const char *method, const char *version);

/**
 * Clone an HTTP request header object, but internal chat_ctx,
 * chat_free_ctx_fn members are not copied
 * @param hdr_req {const HTTP_HDR_REQ*} HTTP request header object
 * @return {HTTP_HDR_REQ*} Cloned HTTP request header object
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_clone(const HTTP_HDR_REQ* hdr_req);

/**
 * Based on previous HTTP request header data and redirect URL,
 * create a new HTTP request header
 * @param hh {const HTTP_HDR_REQ*} Previous HTTP request header object
 * @param url {const char *} Redirect URL, may not have http[s]:// prefix,
 *  can be relative. New Host field in URL will be extracted from this URL,
 *  otherwise inherits Host field from source HTTP request header
 * @return {HTTP_HDR_REQ*} Newly created redirect HTTP request header
 */
HTTP_API HTTP_HDR_REQ *http_hdr_req_rewrite(const HTTP_HDR_REQ *hh, const char *url);

/**
 * Reset HTTP request header data based on redirect URL, updating
 * the HTTP request header info
 * @param hh {const HTTP_HDR_REQ*} Previous HTTP request header object
 * @param url {const char *} Redirect URL, may not have http[s]:// prefix,
 *  can be relative. New Host field in URL will be extracted from this URL,
 *  otherwise inherits Host field from source HTTP request header
 * @return {int} 0: ok; < 0: error
 */
HTTP_API int http_hdr_req_rewrite2(HTTP_HDR_REQ *hh, const char *url);

/**
 * Free HTTP request header object
 * @param hh {HTTP_HDR_REQ*} HTTP request header object
 */
HTTP_API void http_hdr_req_free(HTTP_HDR_REQ *hh);

/**
 * Reset and reinitialize members of HTTP request header object
 * @param hh {HTTP_HDR_REQ*} HTTP request header object
 */
HTTP_API void http_hdr_req_reset(HTTP_HDR_REQ *hh);

/**
 * Parse cookies in HTTP protocol header
 * @param hh {HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_cookies_parse(HTTP_HDR_REQ *hh);

/**
 * Parse HTTP request first line (e.g.:
 * GET /cgi-bin/test.cgi?name=value&name2=value2 HTTP/1.0)
 * Request method (GET) --> hdr_request_method
 * URL data fields (name=value) --> hdr_request_table
 * HTTP protocol version (HTTP/1.0) --> hdr_request_proto
 * URL path (/cgi-bin/test.cgi) --> hdr_request_url
 * @param hh {HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_line_parse(HTTP_HDR_REQ *hh);

/**
 * Parse HTTP request header protocol data, calls http_hdr_req_line_parse,
 * http_hdr_req_cookies_parse internally
 * @param hh {HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_parse(HTTP_HDR_REQ *hh);

/**
 * Parse HTTP request header protocol data, calls http_hdr_req_line_parse,
 * http_hdr_req_cookies_parse internally. If parse_params is 0, doesn't
 * parse parameters in HTTP request url; if parse_cookie is 0, doesn't
 * parse cookie data in HTTP request
 * @param hh {HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @param parse_params {int} Whether to parse parameters in url
 * @param parse_cookie {int} Whether to parse cookie data in request
 * @return {int} 0: ok;  -1: err.
 */
HTTP_API int http_hdr_req_parse3(HTTP_HDR_REQ *hh, int parse_params, int parse_cookie);

/**
 * Get a cookie value from HTTP request header
 * @param hh {HTTP_HDR_REQ*) HTTP request header pointer, not NULL
 * @param name {const char*} Cookie identifier, not NULL
 * @return {const char*} !NULL: return value is the required cookie;
 *  NULL: specified cookie doesn't exist
 */
HTTP_API const char *http_hdr_req_cookie_get(HTTP_HDR_REQ *hh, const char *name);

/**
 * Get HTTP request method from HTTP request header, e.g.: POST, GET, CONNECT
 * @param hh {const HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {const char*} Method string. NULL: error; !NULL: OK.
 */
HTTP_API const char *http_hdr_req_method(const HTTP_HDR_REQ *hh);

/**
 * Get a parameter field value from request URL in HTTP request header,
 * e.g.: get value v2 of n2 from: /cgi-bin/test.cgi?n1=v1&n2=v2
 * @param hh {const HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @param name {const char*} Parameter identifier in request
 * @return {const char*} !NULL: ok, returns parameter value memory pointer;
 *  NULL: parameter with specified identifier doesn't exist.
 */
HTTP_API const char *http_hdr_req_param(const HTTP_HDR_REQ *hh, const char *name);

/**
 * Get request path and parameters from HTTP request header, including
 * parameters, without domain and port.
 * If original request line is:
 *   GET /cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 *  or
 *   GET http://www.test.com[:80]/cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 * Then return result is:
 *   /cgi-bin/test.cgi?n1=v1&n2=v2
 * @param hh {const HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {const char*} URL string. !NULL: OK; NULL: error.
 */
HTTP_API const char *http_hdr_req_url_part(const HTTP_HDR_REQ *hh);

/**
 * Get request path from HTTP request header, without parameters.
 * If original request line is:
 *   GET /cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 *  or
 *   GET http://www.test.com[:80]/cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 * Then return result is:
 *   /cgi-bin/test.cgi
 * @param hh {const HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {const char*} URL string. !NULL: OK; NULL: error.
 */
HTTP_API const char *http_hdr_req_url_path(const HTTP_HDR_REQ *hh);

/**
 * Get server IP address from HTTP request protocol header,
 * format is: IP|domain[:PORT]
 * e.g.: 192.168.0.22:80, or www.test.com:8088
 * @param hh {const HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {const char*} String indicating server address. !NULL: ok;
 *  NULL: error.
 */
HTTP_API const char *http_hdr_req_host(const HTTP_HDR_REQ *hh);

/**
 * Get complete URL string from HTTP request header protocol
 * If original HTTP request header is:
 * GET /cgi-bin/test.cgi?n1=v1&n2=v2 HTTP/1.1
 * HOST: www.test.com
 * Then this function returns:
 * http://www.test.com/cgi-bin/test.cgi?n1=v1&n2=v2
 * @param hh {const HTTP_HDR_REQ*} HTTP request header pointer, not NULL
 * @return {const char*} URL string. !NULL: OK; NULL: error.
 * @example:
 *  void test(HTTP_HDR_REQ *hh)
 *  {
 *    const char *url = http_hdr_req_url(hh);
 *    printf(">>> url: %s\r\n", url ? url : "null");
 *  }
 *  Note: http_hdr_req_url internally uses a local thread-specific
 *  static memory buffer, so cannot use like this, returned data will
 *  be overwritten:
 *  void test(HTTP_HDR_REQ *hh1, HTTP_HDR_REQ *hh2)
 *  {
 *    const char *url1 = http_hdr_req_url(hh1);
 *    const char *url2 = http_hdr_req_url(hh2);
 *    printf(">>> url1: %s, url2: %s\n", url1, url2);
 *  }
 *  Because url1, url2 actually point to same memory buffer, final result
 *  is url1, url2 are identical. To avoid this, use following approach:
 *  void test(HTTP_HDR_REQ *hh1, HTTP_HDR_REQ *hh2)
 *  {
 *    const char *ptr;
 *    static char dummy[1];
 *    char *url1 = dummy, *url2 = dummy;
 *    ptr = http_hdr_req_url(hh1);
 *    if (ptr)
 *      url1 = acl_mystrdup(ptr);
 *    ptr = http_hdr_req_url(hh2);
 *    if (ptr)
 *      url2 = acl_mystrdup(ptr);
 *    printf(">>> url1: %s, url2: %s\n", url1, url2);
 *    if (url1 != dummy)
 *      acl_myfree(url1);
 *    if (url2 != dummy)
 *      acl_myfree(url2);
 *  }
 */
HTTP_API const char *http_hdr_req_url(const HTTP_HDR_REQ *hh);

/**
 * Parse Range field in HTTP request header
 * @param hdr_req {HTTP_HDR_REQ*} Request HTTP protocol header, not NULL
 * @param range_from {http_off_t*} Store offset start position
 * @param range_to {http_off_t*} Store offset end position
 * Note: {range_from}, {range_to} index starts from 0
 * Range format:
 *   Range: bytes={range_from}-, bytes={range_from}-{range_to}
 */
HTTP_API int http_hdr_req_range(const HTTP_HDR_REQ *hdr_req,
	http_off_t *range_from, http_off_t *range_to);

/*----------------------- HTTP response header functions --------------------*/
/* in http_hdr_res.c */

/**
 * Parse status line in HTTP response header
 *@param hh {HTTP_HDR_RES*} HTTP response header pointer, not NULL
 *@param dbuf {const char*} Status line data, e.g.: HTTP/1.0 200 OK, not NULL
 *@return {int} 0: ok;  < 0: error. Results stored in hh structure
 */
HTTP_API int http_hdr_res_status_parse(HTTP_HDR_RES *hh, const char *dbuf);

/**
 * Create a new HTTP response header
 * @return {HTTP_HDR_RES*}
 */
HTTP_API HTTP_HDR_RES *http_hdr_res_new(void);

/**
 * Clone an HTTP response header
 * @param hdr_res {const HTTP_HDR_RES*} Source HTTP response header
 * @return {HTTP_HDR_RES *} Newly created HTTP response header
 */
HTTP_API HTTP_HDR_RES *http_hdr_res_clone(const HTTP_HDR_RES *hdr_res);

/**
 * Free an HTTP response header
 * @param hh {HTTP_HDR_RES*} HTTP response header
 */
HTTP_API void http_hdr_res_free(HTTP_HDR_RES *hh);

/**
 * Reinitialize HTTP response header, freeing internal members
 * @param hh {HTTP_HDR_RES*} HTTP response header
 */
HTTP_API void http_hdr_res_reset(HTTP_HDR_RES *hh);

/**
 * Parse HTTP response header protocol data and store in structure
 * @param hdr_res {HTTP_HDR_RES*} HTTP response header
 */
HTTP_API int http_hdr_res_parse(HTTP_HDR_RES *hdr_res);

/**
 * Parse Range field in HTTP response header
 * @param hdr_res {HTTP_HDR_RES*} Response HTTP protocol header, not NULL
 * @param range_from {http_off_t*} Store offset start position, not NULL
 * @param range_to {http_off_t*} Store offset end position, not NULL
 * @param total_length {http_off_t*} Store total file length, may be NULL
 * @return {int} Returns 0 on success, -1 on failure
 * Note: {range_from}, {range_to} index starts from 0
 * Response Range format:
 *   Content-Range: bytes {range_from}-{range_to}/{total_length}
 */
HTTP_API int http_hdr_res_range(const HTTP_HDR_RES *hdr_res,
	http_off_t *range_from, http_off_t *range_to, http_off_t *total_length);

/* in http_rfc1123.c */

/**
 * Convert time value to RFC1123 required format
 * @param buf {char*} Storage space
 * @param size {size_t} buf space size
 * @param t {time_t} Time value
 */
HTTP_API const char *http_mkrfc1123(char *buf, size_t size, time_t t);

/*---------------------- HTTP async request functions -----------------------*/
/* in http_chat_async.c */

/**
 * Asynchronously read an HTTP REQUEST protocol header, store data in hdr.
 * After reading complete HTTP header successfully, call user-registered
 * callback notify
 * @param hdr {HTTP_HDR_REQ*} HTTP request header structure pointer, not NULL
 * @param astream {ACL_ASTREAM*} Stream from client connection, not NULL
 * @param notify {HTTP_HDR_NOTIFY} Callback called when HTTP protocol header
 *  read completes
 * @param arg {void*} Parameter passed to notify
 * @param timeout {int} Timeout during data read process
 */
HTTP_API void http_hdr_req_get_async(HTTP_HDR_REQ *hdr, ACL_ASTREAM *astream,
		HTTP_HDR_NOTIFY notify, void *arg, int timeout);

/**
 * Asynchronously read an HTTP RESPOND protocol header, store data in hdr.
 * After reading complete HTTP header successfully, call user-registered
 * callback notify
 * @param hdr {HTTP_HDR_REQ*} HTTP response header structure pointer, not NULL
 * @param astream {ACL_ASTREAM*} Stream from server connection, not NULL
 * @param notify {HTTP_HDR_NOTIFY} Callback called when HTTP protocol header
 *  read completes
 * @param arg {void*} Parameter passed to notify
 * @param timeout {int} Timeout during data read process
 */
HTTP_API void http_hdr_res_get_async(HTTP_HDR_RES *hdr, ACL_ASTREAM *astream,
		HTTP_HDR_NOTIFY notify, void *arg, int timeout);

/**
 * Asynchronously read request BODY protocol data from client. During
 * receive process, call user notify callback at boundaries. If notify
 * returns < 0, read stops and won't call the callback again
 * @param request {HTTP_REQ*} HTTP request object pointer, not NULL,
 *  and request->hdr not NULL
 * @param astream {ACL_ASTREAM*} Stream from client connection, not NULL
 * @param notify {HTTP_BODY_NOTIFY} User-registered callback for receiving
 *  client data
 * @param arg {void*} Parameter passed to notify
 * @param timeout {int} Timeout during data read process
 */
HTTP_API void http_req_body_get_async(HTTP_REQ *request, ACL_ASTREAM *astream,
		 HTTP_BODY_NOTIFY notify, void *arg, int timeout);
/*
 * Asynchronously read response BODY protocol data from server. During
 * receive process, call user notify callback. If notify returns < 0,
 * read stops and won't call the callback again
 * @param respond {HTTP_RES*} HTTP response object pointer, not NULL,
 *  and respond->hdr not NULL
 * @param astream {ACL_ASTREAM*} Stream from server connection, not NULL
 * @param notify {HTTP_BODY_NOTIFY} User-registered callback for receiving
 *  server data
 * @param arg {void*} Parameter passed to notify
 * @param timeout {int} Timeout during data read process
 */
HTTP_API void http_res_body_get_async(HTTP_RES *respond, ACL_ASTREAM *astream,
		HTTP_BODY_NOTIFY notify, void *arg, int timeout);

/*---------------------- HTTP sync request functions ------------------------*/
/* in http_chat_sync.c */

/**
* Synchronously read an HTTP REQUEST protocol header, store data in hdr.
* After reading complete HTTP header successfully, call user-registered
* callback notify
* @param hdr {HTTP_HDR_REQ*} HTTP request header structure pointer, not NULL
* @param stream {ACL_VSTREAM*} Stream from client connection, not NULL
* @param timeout {int} Timeout during data read process
* @return {int} 0: success; < 0: failure
*/
HTTP_API int http_hdr_req_get_sync(HTTP_HDR_REQ *hdr,
		 ACL_VSTREAM *stream, int timeout);

/**
 * Synchronously read an HTTP RESPOND protocol header, store data in hdr.
 * After reading complete HTTP header successfully, call user-registered
 * callback notify
 * @param hdr {HTTP_HDR_REQ*} HTTP response header structure pointer, not NULL
 * @param stream {ACL_VSTREAM*} Stream from server connection, not NULL
 * @param timeout {int} Timeout during data read process
 * @return {int} 0: success; < 0: failure
 */
HTTP_API int http_hdr_res_get_sync(HTTP_HDR_RES *hdr,
		ACL_VSTREAM *stream, int timeout);

/**
 * Synchronously read request BODY protocol data from client
 * @param request {HTTP_REQ*} HTTP request object pointer, not NULL,
 *  and request->hdr not NULL
 * @param stream {ACL_VSTREAM*} Stream from client connection, not NULL
 * @param buf {void *} Storage space for received data
 * @param size {int} Size of buf space
 * @return ret {http_off_t} Number of bytes read from HTTP request body:
 *             0: All HTTP request body data read, possibly remote closed;
 *             < 0: Remote connection closed or other error;
 *             > 0: Not finished, currently read ret bytes
 */
HTTP_API http_off_t http_req_body_get_sync(HTTP_REQ *request, ACL_VSTREAM *stream,
		void *buf, int size);
#define http_req_body_get_sync2	http_req_body_get_sync

/**
 * Synchronously read response BODY protocol data from server
 * @param respond {HTTP_RES*} HTTP response object pointer, not NULL,
 *  and respond->hdr not NULL
 * @param stream {ACL_VSTREAM*} Stream from client connection, not NULL
 * @param buf {void *} Storage space for received data
 * @param size {int} Size of buf space
 * @return ret {http_off_t} Number of bytes read from HTTP response body:
 *             0: All HTTP response data read, possibly remote closed;
 *             < 0: Remote connection closed or other error;
 *             > 0: Not finished, currently read ret bytes
 */
HTTP_API http_off_t http_res_body_get_sync(HTTP_RES *respond, ACL_VSTREAM *stream,
		void *buf, int size);
#define http_res_body_get_sync2	http_res_body_get_sync

/**
 * Set control flag bits for request protocol
 * @param request {HTTP_REQ*} HTTP request object pointer, not NULL,
 *  and request->hdr not NULL
 * @param name {int} First flag bit, when last flag bit is
 *  HTTP_CHAT_SYNC_CTL_END, indicates end
 */
HTTP_API void http_chat_sync_reqctl(HTTP_REQ *request, int name, ...);

/**
 * Set control flag bits for response protocol
 * @param respond {HTTP_RES*} HTTP response object pointer, not NULL,
 *  and respond->hdr not NULL
 * @param name {int} First flag bit, when last flag bit is
 *  HTTP_CHAT_SYNC_CTL_END, indicates end
 */
HTTP_API void http_chat_sync_resctl(HTTP_RES *respond, int name, ...);
#define	HTTP_CHAT_SYNC_CTL_END      0  /* End flag bit */
#define	HTTP_CHAT_CTL_BUFF_ONOFF    1  /* Pre-buffer during data reception */

/*---------------- HTTP request object creation/free functions --------------*/
/* in http_req.c */

/**
 * Create a request object based on HTTP request header
 * @param hdr_req {HTTP_HDR_REQ*} Request header object
 * @return {HTTP_REQ*} Request object
 */
HTTP_API HTTP_REQ *http_req_new(HTTP_HDR_REQ *hdr_req);

/**
 * Free request object
 * @param request {HTTP_REQ*} Request object
 */
HTTP_API void http_req_free(HTTP_REQ *request);

/*--------------- HTTP response object creation/free functions --------------*/
/* in http_res.c */

/**
* Create a response object based on HTTP response header
* @param hdr_res {HTTP_HDR_RES*} Response header object
* @return {HTTP_RES*} Response object
*/
HTTP_API HTTP_RES *http_res_new(HTTP_HDR_RES *hdr_res);

/**
 * Free response object
 * @param respond {HTTP_RES*} Response object
 */
HTTP_API void http_res_free(HTTP_RES *respond);

/*------------------------- HTTP header build functions ---------------------*/
/* in http_hdr_build.c */

/**
 * Add string entry to common HTTP header
 * @param hdr {HTTP_HDR*} Common HTTP header object
 * @param name {const char*} Entry name, e.g. Accept-Encoding in:
 *  Accept-Encoding: deflate, gzip
 * @param value {const char*} Entry value, e.g. deflate, gzip in:
 *  Accept-Encoding: deflate, gzip
 */
HTTP_API void http_hdr_put_str(HTTP_HDR *hdr, const char *name, const char *value);

/**
 * Add integer entry to common HTTP header
 * @param hdr {HTTP_HDR*} Common HTTP header object
 * @param name {const char*} Entry name, e.g. Content-Length in:
 *  Content-Length: 1024
 * @param value {const int} Entry value, e.g. 1024 in: Content-Length: 1024
 */
HTTP_API void http_hdr_put_int(HTTP_HDR *hdr, const char *name, int value);

/**
 * Add formatted string entry to common HTTP header
 * @param hdr {HTTP_HDR*} Common HTTP header object
 * @param name {const char*} Entry name, e.g. Accept-Encoding in:
 *  Accept-Encoding: deflate, gzip
 * @param fmt {const char*} Format string
 */
# if defined(WIN32) || defined(WIN64)
HTTP_API void http_hdr_put_fmt(HTTP_HDR *hdr, const char *name, const char *fmt, ...);
#else
HTTP_API void __attribute__((format(printf,3,4)))
	http_hdr_put_fmt(HTTP_HDR *hdr, const char *name, const char *fmt, ...);
#endif

/**
 * Add time entry to common HTTP header
 * @param hdr {HTTP_HDR*} Common HTTP header object
 * @param name {const char*} Entry name
 * @param t {time_t} Time value
 */
HTTP_API void http_hdr_put_time(HTTP_HDR *hdr, const char *name, time_t t);

/**
 * Check if keep-alive is set in HTTP request header fields and store
 * result in HTTP response header
 * @param req {const HTTP_HDR_REQ*} HTTP request header
 * @param res {HTTP_HDR_RES*} HTTP response header to store result
 */
HTTP_API int http_hdr_set_keepalive(const HTTP_HDR_REQ *req, HTTP_HDR_RES *res);

/**
 * Initialize an HTTP response header with status code (1xx, 2xx, 3xx,
 * 4xx, 5xx)
 * @param hdr_res {HTTP_HDR_RES*} HTTP response header to store result
 * @param status {int} Status code: nxx(1xx, 2xx, 3xx, 4xx, 5xx)
 */
HTTP_API void http_hdr_res_init(HTTP_HDR_RES *hdr_res, int status);

/**
 * Create an HTTP response header with status code (nxx)
 * @param status {int} Status code: nxx(1xx, 2xx, 3xx, 4xx, 5xx)
 * @return {HTTP_HDR_RES*} Generated HTTP response header
 */
HTTP_API HTTP_HDR_RES *http_hdr_res_static(int status);

/**
* Create an HTTP response header with status code (nxx)
* @param status {int} Status code: nxx(4xx, 5xx)
* @return {HTTP_HDR_RES*} Generated HTTP response header
*/
HTTP_API HTTP_HDR_RES *http_hdr_res_error(int status);

/**
 * Build HTTP common header data and store header data in BUF
 * @param hdr {const HTTP_HDR*} Common HTTP header
 * @param strbuf {ACL_VSTRING*} Buffer to store results
 */
HTTP_API void http_hdr_build(const HTTP_HDR *hdr, ACL_VSTRING *strbuf);

/**
 * Build HTTP request header and store header data in BUF
 * @param hdr_req {const HTTP_HDR_REQ*} HTTP request header
 * @param strbuf {ACL_VSTRING*} Buffer to store results
 */
HTTP_API void http_hdr_build_request(const HTTP_HDR_REQ *hdr_req, ACL_VSTRING *strbuf);

/*---------------------- HTTP response status info functions ----------------*/
/* in http_status.c */

/**
 * Get string representation of HTTP response code (nxx)
 * @param status {int} Status code: nxx(1xx, 2xx, 3xx, 4xx, 5xx)
 * @return {const char*} String representation corresponding to response code
 */
HTTP_API const char *http_status_line(int status);

/*---------------------- HTTP HTML template functions -----------------------*/
/* in http_tmpl.c */

/**
 * Load HTTP response default HTML template
 * @param tmpl_path {const char*} Path where HTML template file is located
 */
HTTP_API void http_tmpl_load(const char *tmpl_path);

/**
 * Get template info corresponding to HTTP response status
 * @param status {int} HTTP status response code
 * @return {const ACL_VSTRING*} Template info corresponding to HTTP response
 */
HTTP_API const ACL_VSTRING *http_tmpl_get(int status);

/**
 * Get title display info corresponding to HTTP response status
 * @param status {int} HTTP status response code
 * @return {const char*} Title display info corresponding to HTTP response
 */
HTTP_API const char *http_tmpl_title(int status);

/**
 * Get length of template display info corresponding to HTTP response status
 * @param status {int} HTTP status response code
 * @return {int} Length of template display info
 */
HTTP_API int http_tmpl_size(int status);

/*-------------------- HTTP HTML template init functions --------------------*/
/* in http_init.c */

/**
 * Initialize HTTP application protocol
 * @param tmpl_path {const char*} Storage path for template info file
 */
HTTP_API void http_init(const char *tmpl_path);

/**
 * Whether to automatically cache and reuse freed HTTP header objects,
 * enabling memory buffer reuse. This function should only be called once
 * during program initialization
 * @param max {int} When value > 0, enable HTTP header object cache function
 */
HTTP_API void http_hdr_cache(int max);

/**
 * Set buffer size during HTTP protocol data transfer
 * @param size {http_off_t} Buffer size
 */
HTTP_API void http_buf_size_set(http_off_t size);

/**
 * Get buffer size during HTTP protocol data transfer
 * @return {http_off_t} Buffer size
 */
HTTP_API http_off_t http_buf_size_get(void);

#ifdef	__cplusplus
}
#endif

#endif

