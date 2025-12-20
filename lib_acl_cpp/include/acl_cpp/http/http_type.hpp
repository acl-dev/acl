#pragma once

namespace acl {

struct HTTP_PARAM {
	char* name;
	char* value;
};

// HTTP response status
typedef enum {
	HTTP_OK,                // Everything is normal
	HTTP_ERR_DNS,           // Domain name resolution failed
	HTTP_ERR_CONN,          // Connection to server failed
	HTTP_ERR_REQ,           // Failed to create request protocol
	HTTP_ERR_READ,          // Read data failed
	HTTP_ERR_SEND,          // Write data failed
	HTTP_ERR_TIMO,          // Read/write data timeout
	HTTP_ERR_READ_HDR,      // Failed to read HTTP response header
	HTTP_ERR_READ_BODY,     // Failed to read HTTP response body
	HTTP_ERR_INVALID_HDR,   // HTTP response header invalid
	HTTP_ERR_UNKNOWN,       // Unknown error occurred
	HTTP_ERR_REDIRECT_MAX,	// Too many redirects in HTTP response header
} http_status_t;

// HTTP request method
typedef enum {
	HTTP_METHOD_UNKNOWN,    // Unknown method
	HTTP_METHOD_GET,        // GET method
	HTTP_METHOD_POST,       // POST method
	HTTP_METHOD_PUT,        // PUT method
	HTTP_METHOD_CONNECT,    // CONNECT method
	HTTP_METHOD_PURGE,      // PURGE method
	HTTP_METHOD_DELETE,     // DELETE method
	HTTP_METHOD_HEAD,       // HEAD method
	HTTP_METHOD_OPTION,     // OPTION method
	HTTP_METHOD_PROPFIND,	// PROPFIND method
	HTTP_METHOD_PATCH,	// PATCH method
	HTTP_METHOD_OTHER,	// Other methods
} http_method_t;

typedef enum {
	// Content-Type: application/x-www-form-urlencoded
	HTTP_REQUEST_NORMAL,

	// Content-Type: multipart/form-data; boundary=xxx
	HTTP_REQUEST_MULTIPART_FORM,

	// Content-Type: application/octet-stream
	HTTP_REQUEST_OCTET_STREAM,

	// Content-Type: text/xml or application/xml
	HTTP_REQUEST_TEXT_XML,

	// Content-Type: text/json or application/json
	HTTP_REQUEST_TEXT_JSON,

	// Other types
	HTTP_REQUEST_OTHER
} http_request_t;

typedef enum {
	// ok
	HTTP_REQ_OK,

	// network io error
	HTTP_REQ_ERR_IO,

	// invalid request method
	HTTP_REQ_ERR_METHOD
} http_request_error_t;

typedef enum {
	HTTP_MIME_PARAM,        // http mime node is parameter type
	HTTP_MIME_FILE          // http mime node is file type
} http_mime_t;

} // namespace acl end

