#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

struct HTTP_HDR;
struct HTTP_HDR_RES;
struct HTTP_RES;
struct HTTP_HDR_REQ;
struct HTTP_REQ;

namespace acl {

class string;
class zlib_stream;
class socket_stream;
class ostream;
class istream;
class http_header;

/**
 * This class has two uses: 1) HTTP client request object, used when client sends request; 2) HTTP server receives
 * HTTP client request and responds with HTTP client object.
 * This client object supports long connections.
 */
class ACL_CPP_API http_client : public noncopyable {
public:
	/**
	 * Default constructor. When using this constructor to create HTTP client object, you need to explicitly
	 * call http_client::open function to open connection.
	 */
	http_client();

	/**
	 * Constructor using already successfully connected stream object to create HTTP client object. Note:
	 * When http_client object is destroyed, passed client object will not be destroyed, and
	 * you need to destroy it yourself, otherwise resource leak will occur.
	 * @param client {socket_stream*} HTTP connection object, can be server-side connection
	 *  or client-side connection. When object is destroyed, whether client object will be automatically destroyed
	 *  depends on stream_fixed value.
	 * @param is_request {bool} Whether client or server client.
	 * @param unzip {bool} When reading server response body, whether to automatically decompress compressed data.
	 *  This parameter is effective when server response body is compressed data. This parameter affects whether to automatically decompress when calling following function:
	 *  read_body(string&, bool, int*)
	 * @param stream_fixed {bool} When this value is true, when http_client object
	 *  is destroyed, passed client object will not be destroyed, and response object will be destroyed. When
	 *  value is false, when this object is destroyed, client object will also be destroyed.
	 */
	http_client(socket_stream* client, bool is_request = false,
		bool unzip = true, bool stream_fixed = true);

	virtual ~http_client();

	/**
	 * In objects that support long connections, you can call this function to reset intermediate data objects.
	 * This is not necessary, because when calling read_head multiple times, read_head will automatically
	 * call reset to clear last read intermediate data.
	 */
	void reset();

	/**
	 * Connect to remote HTTP server.
	 * @param addr {const char*} Server address, format: IP|PORT or DOMAIN|PORT
	 * @param conn_timeout {int} Connection timeout time (seconds)
	 * @param rw_timeout {int} Read/write timeout time (seconds)
	 * @param unzip {bool} When server response body is compressed data, whether to automatically decompress.
	 * @return {bool} Whether connection was successful.
	 */
	bool open(const char* addr, int conn_timeout = 60, int rw_timeout = 60,
		bool unzip = true);

	/**
	 * Write HTTP request header and wait for response.
	 * @param header {http_header&}
	 * @return {bool} Whether writing header and waiting was successful.
	 */
	bool write_head(const http_header& header);

	/**
	 * Write HTTP request body. You can call this function in a loop. When first call to write function writes
	 * HTTP header, if chunked transfer mode is used, internally automatically uses chunked transfer mode; 
	 * Additionally, when using chunked format, when sending data, you should call this function once more, and data
	 * parameter should be 0 to indicate data sending is complete.
	 * @param data {const void*} Data address.
	 * @param len {size_t} Data length of data.
	 * @return {bool} Whether writing was successful. Returns false to indicate connection closed.
	 */
	bool write_body(const void* data, size_t len);

	/**
	 * When using http_client(socket_stream*, bool) constructor, or
	 * using http_client(void) constructor and calling open function at the same time,
	 * you can call this function to get output stream object.
	 * @return {ostream&} Returns reference to output stream. When stream is not initialized,
	 *  internally automatically initializes stream. It is recommended to use it after initialization.
	 */
	ostream& get_ostream() const;

	/**
	 * When using http_client(socket_stream*, bool) constructor, or
	 * using http_client(void) constructor and calling open function at the same time,
	 * you can call this function to get input stream object.
	 * @return {istream&} Returns reference to input stream. When stream is not initialized,
	 *  internally automatically initializes stream. It is recommended to use it after initialization.
	 */
	istream& get_istream() const;

	/**
	 * When using http_client(socket_stream*, bool) constructor, or
	 * using http_client(void) constructor and calling open function at the same time,
	 * you can call this function to get stream object.
	 * @return {socket_stream&} Returns reference to stream. When stream is not initialized,
	 *  internally automatically initializes stream. It is recommended to use it after initialization.
	 */
	socket_stream& get_stream() const;

	/**
	 * Read response header data from HTTP server or read request data from HTTP client.
	 * In long connection objects, this function automatically clears last read intermediate data objects.
	 * @return {bool} Whether successful.
	 */
	bool read_head();

	/**
	 * Get HTTP server response body length.
	 * @return {int64) Return value is -1 when HTTP header does not exist or has no Content-Length field.
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 body_length(void) const;
#else
	long long int body_length() const;
#endif

	/**
	 * When object is request client, this function gets start address and end address from request header.
	 * @param range_from {long long int&} Start offset position.
	 * @param range_to {long long int&} End offset position.
	 * @return {bool} When request is range data, returns false.
	 *  When not range data, returns true, and sets range_from and range_to values.
	 *  Note: range_from/range_to subscript starts from 0.
	 *  Data format:
	 *  Range: bytes={range_from}-{range_to} or
	 *  Range: bytes={range_from}-
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool request_range(__int64& range_from, __int64& range_to);
#else
	bool request_range(long long int& range_from, long long int& range_to);
#endif

	/**
	 * When object is response server, this function gets start address and end address from response header.
	 * @param range_from {long long int&} Start offset position.
	 * @param range_to {long long int&} End offset position.
	 * @param total {long long int} Total resource length.
	 * @return {bool} When response is range response, returns false.
	 *  When not range response, returns true, and sets range_from and range_to values.
	 *  Note: range_from/range_to subscript starts from 0.
	 *  Data format:
	 *  Content-Range: bytes {range_from}-{range_to}/{total_length}
	 *  e.g.: Content-Range: bytes 2250000-11665200/11665201
	 */ 
#if defined(_WIN32) || defined(_WIN64)
	bool response_range(__int64& range_from, __int64& range_to,
		__int64& total);
#else
	bool response_range(long long int& range_from,
		long long int& range_to, long long int& total);
#endif

	/**
	 * Get version number in HTTP header.
	 * @param major {unsigned&} Major version number.
	 * @param minor {unsigned&} Minor version number.
	 * @return {bool} Whether successfully got version number.
	 */
	bool get_version(unsigned& major, unsigned& minor) const;

	/**
	 * HTTP keep-alive (whether response supports long connection).
	 * @return {bool}
	 */
	bool is_keep_alive() const;
	bool keep_alive() const;

	/**
	 * When object is client request, this function determines whether server-returned HTTP header
	 * supports long connection.
	 * @return {bool}
	 */
	bool is_server_keep_alive() const;

	/**
	 * When object is server response, this function determines whether client request HTTP header
	 * supports long connection.
	 * @return {bool}
	 */
	bool is_client_keep_alive() const;

	/**
	 * Get a certain field value in HTTP request header or response header.
	 * @param name {const char*} Field name.
	 * @return {const char*} Field value. Returns empty when not found.
	 */
	const char* header_value(const char* name) const;

	/**
	 * Disable certain fields in HTTP request/response header.
	 * @param name {const char*} Field name.
	 */
	void header_disable(const char* name);

	/**
	 * Update or replace a certain field in HTTP header.
	 * @param name {const char*} HTTP header field name, e.g.: Content-Length.
	 *   Field name is case-insensitive.
	 * @param value {const char*} New header field value.
	 * @param force_add {bool} When header field does not exist, whether to force add.
	 * @return {bool} Returns false to indicate error occurred or header field does not exist and
	 *  force_add is false.
	 */
	bool header_update(const char* name, const char* value,
		bool force_add = true);

	/**
	 * In a certain field in HTTP header, replace source string with target string. Supports
	 * multiple match replacements.
	 * @param name {const char*} HTTP header field name, e.g.: Content-Length.
	 *   Field name is case-insensitive.
	 * @param match {const char*} Field value matching string.
	 * @param to {const char*} Target string value to replace with.
	 * @param case_sensitive {bool} Whether case-sensitive when performing replacement internally.
	 * @return {int} Number of match replacements. 0 indicates no replacement occurred. < 0 indicates error.
	 */
	int header_update(const char* name, const char* match,
		const char* to, bool case_sensitive = false);

	/**
	 * Get HTTP response status code returned by HTTP server.
	 * 1xx, 2xx, 3xx, 4xx, 5xx
	 * @return {int} Return value is -1 to indicate error occurred or no session exists
	 *  or HTTP request header data reading process is not complete.
	 */
	int response_status() const;

	/**
	 * Get HOST field value in HTTP client request header.
	 * @return {const char*} Returns NULL to indicate this field does not exist.
	 */
	const char* request_host() const;

	/**
	 * Get PORT port number in HTTP client request header.
	 * @return {int} Returns -1 to indicate error.
	 */
	int request_port() const;

	/**
	 * Get HTTP request method in HTTP client request header: GET, POST, CONNECT
	 * @return {const char*} Return value is empty to indicate error.
	 */
	const char* request_method() const;

	/**
	 * Get URL in HTTP client request header excluding HTTP://domain part.
	 * e.g.: For http://test.com.cn/cgi-bin/test?name=value, this function
	 * should return: /cgi-bin/test?name=value
	 * @return {const char*} Returns NULL to indicate error.
	 */
	const char* request_url() const;

	/**
	 * Get path part (excluding parameters) in URL in HTTP client request header.
	 * e.g.: For http://test.com.cn/cgi-bin/test?name=value, this function
	 * should return: /path/test.cgi
	 * @return {const char*} Returns NULL to indicate error.
	 */
	const char* request_path() const;

	/**
	 * Get all parameters in URL in HTTP client request header, e.g.:
	 * http://test.com.cn/cgi-bin/test?name=value, this function should return:
	 * name=value
	 * @return {const char*} Returns NULL to indicate error.
	 */
	const char* request_params() const;

	/**
	 * Get specified parameter value in URL in HTTP client request header, e.g.:
	 * http://test.com.cn/cgi-bin/test?name=value, through this function to get
	 * name parameter value is value.
	 * @param name {const char*} Parameter name.
	 * @return {const char*} Parameter value. Returns NULL to indicate error.
	 */
	const char* request_param(const char* name) const;

	/**
	 * Get cookie value in HTTP client request header.
	 * @param name {const char*} Cookie name.
	 * @return {const char*} Cookie value. Returns NULL to indicate not found.
	 */
	const char* request_cookie(const char* name) const;

	/**
	 * Read response body data from HTTP server or read request body data from HTTP client.
	 * This function automatically decompresses received compressed data.
	 * @param out {string&} Buffer to store result.
	 * @param clean {bool} Whether to automatically clear buf buffer before reading data.
	 * @param real_size {int*} When pointer is not empty, records uncompressed response body length.
	 *  Usually, value returned through pointer is always >= 0.
	 * @return {int} Return value as below: (Applications need to determine whether response body is finished or connection is closed through body_finish function and
	 *       disconnected function.)
	 *  > 0: Indicates data has been read, but data is not finished yet, need to continue reading.
	 *  == 0: For various reasons, 0 will be returned. When data is finished, 0 will be returned. You can call body_finish
	 *        function to determine whether HTTP response body has been completely read. When decompressing compressed data end, when
	 *        decompressing compressed data's 8-byte end (which is a check field), it may not be returned as response body,
	 *        at this time 0 will also be returned.
	 *        Applications can determine whether connection has been closed through disconnected() function.
	 *        When data is finished but connection is not closed, it means long connection is supported.
	 *  < 0: Indicates connection closed.
	 * Note: read_body function's return value may be inaccurate:
	 *     When decompressing compressed data, returned value is decompressed data length.
	 */
	int read_body(string& out, bool clean = true, int* real_size = NULL);
	
	/**
	 * Read response body data from HTTP server or read request body data from HTTP client.
	 * This function does not decompress data.
	 * @param buf {char*} Buffer to store result. Buffer cannot be empty.
	 * @param size {size_t} Buffer size.
	 * @return {int} Return value as below:
	 *  > 0: Indicates data has been read, but data is not finished yet, need to continue reading.
	 *  == 0: Indicates HTTP response body data has been completely read, but connection is not closed.
	 *  < 0: Indicates connection closed.
	 */
	int read_body(char* buf, size_t size);

	/**
	 * Read one line of data from HTTP server response body or client request body. This function internally
	 * automatically decompresses original data. You can call this function in a loop until it returns false
	 * or body_finish() returns true. When this function returns false, it means connection has been
	 * closed. When it returns true, it means one line was read. At this time, you can determine by checking
	 * body_finish() function value whether response body has been completely read.
	 * @param out {string&} Buffer to store result. This function internally automatically clears
	 *  buffer. Users do not need to clear it before calling this function (can call: out.clear()).
	 * @param nonl {bool} Whether to automatically remove "\r\n" or "\n" at the end when reading one line.
	 * @param size {size_t*} When reading one line, stores length of read data.
	 *  When reading one line and nonl is true, this value is 0.
	 * @return {bool} Whether one line was read. When this function returns false, it means connection
	 *  has been closed or no more data to read. When it returns true, it means one line was read. When reading one line, this function also returns true, only *size = 0.
	 */
	bool body_gets(string& out, bool nonl = true, size_t* size = NULL);

	/**
	 * Determine whether HTTP response body has been completely read.
	 * @return {bool}
	 */
	bool body_finish() const;

	/**
	 * Determine whether connection stream has been closed.
	 * @return {bool}
	 */
	bool disconnected() const;

	/**
	 * Get HTTP response header object found through read_head function and input buffer.
	 * When not empty, HTTP response header data can be obtained.
	 * @param buf {string*} When not empty, stores HTTP response header data.
	 * @return {const HTTP_HDR_RES*} HTTP response header object. Returns empty to indicate
	 *  response header data was not read.
	 */
	HTTP_HDR_RES* get_respond_head(string* buf);

	/**
	 * Get HTTP request header object found through read_head function and input buffer.
	 * When not empty, HTTP request header data can be obtained.
	 * @param buf {string*} When not empty, stores HTTP request header data.
	 * @return {const HTTP_HDR_REQ*} HTTP request header object. Returns empty to indicate
	 *  request header data was not read.
	 */
	HTTP_HDR_REQ* get_request_head(string* buf);

	/**
	 * Print HTTP response header information returned by server to standard output.
	 * @param prompt {const char*} When not empty, same as HTTP header information, printed together.
	 */
	void print_header(const char* prompt = NULL);

	/**
	 * Print HTTP response header information returned by server to file stream.
	 * @param out {ostream&} Output stream, can be file stream, can also be standard output.
	 * @param prompt {const char*} When not empty, same as HTTP header information, printed together.
	 */
	void fprint_header(ostream& out, const char* prompt = NULL);

	/**
	 * Print HTTP response header information returned by server to string buffer.
	 * @param out {string&} Buffer to store result data.
	 * @param prompt {const char*} When not empty, same as HTTP header information, printed together.
	 */
	void sprint_header(string& out, const char* prompt = NULL);

private:
	socket_stream* stream_;     // HTTP connection stream.
	bool stream_fixed_;         // Whether to release stream_ object.

	HTTP_HDR_RES* hdr_res_;     // HTTP response header object.
	struct HTTP_RES* res_;      // HTTP response object.
	HTTP_HDR_REQ* hdr_req_;     // HTTP request header object.
	struct HTTP_REQ* req_;      // HTTP request object.
	bool unzip_;                // Whether to decompress compressed data.
	zlib_stream* zstream_;      // Decompression stream.
	bool is_request_;           // Whether it is client request.
	int  gzip_header_left_;     // Remaining length of gzip header.
	int  last_ret_;             // Last data reading recorded return value.
	bool head_sent_;            // Whether header has been sent.
	bool body_finish_;          // Whether HTTP response body has been completely read.
	bool disconnected_;         // Whether connection stream has been closed.
	bool chunked_transfer_;     // Whether it is chunked transfer mode.
	unsigned gzip_crc32_;       // CRC32 value when decompressing gzip compressed data.
	unsigned gzip_total_in_;    // Data length before gzip compression.      
	string* buf_;               // Internal temporary buffer, used for partial reading.

	bool read_request_head();
	bool read_response_head();
	int  read_request_body(char* buf, size_t size);
	int  read_response_body(char* buf, size_t size);
	int  read_request_body(string& out, bool clean, int* real_size);
	int  read_response_body(string& out, bool clean, int* real_size);

	HTTP_HDR* get_http_hdr() const;

public:
	bool write_chunk(ostream& out, const void* data, size_t len);
	bool write_chunk_trailer(ostream& out);

	bool write_gzip(ostream& out, const void* data, size_t len);
	bool write_gzip_trailer(ostream& out);
};

}  // namespace acl

