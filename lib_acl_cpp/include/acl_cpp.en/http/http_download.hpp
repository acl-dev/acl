#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class http_client;
class http_request;
class http_header;

class ACL_CPP_API http_download : public noncopyable {
public:
	/**
	 * Constructor
	 * @param url {const char*} URL address of file on server
	 * @param addr {const char*} When not empty, sets server address (format:
	 *  ip[|domain]:port, otherwise server address is extracted from url
	 */
	http_download(const char* url, const char* addr = NULL);
	virtual ~http_download();

	/**
	 * Before calling run, can get request header object through this function, convenient for users to set
	 * their own request header fields (but set_method/set_range are automatically set internally)
	 * @return {http_header*} Returns NULL indicates input URL is illegal
	 */
	http_header* request_header() const;

	/**
	 * Calling this function can get http_request object, convenient for setting or querying request header
	 * or parameters in returned data
	 * @return {http_request*} Returns NULL indicates input URL is illegal
	 */
	http_request* request() const;

	/**
	 * Download file. When range_from >= 0 and range_to >= range_from, automatically
	 * uses segmented download mode, otherwise uses full download mode
	 * @param range_from {acl_int64} Download start offset position, index starts from 0.
	 *  When this value >= 0 and range_to >= this value, segmented download mode is used
	 * @param range_to {acl_int64} Download end offset position
	 * @param req_body {const char*} Request data body
	 * @param len {size_t} When req_body is not empty, specifies its length
	 * @return {bool} Whether download was successful. If returns true, it indicates download was successful. Otherwise,
	 *  may be input parameters are illegal, or URL does not exist, or server does not support resumable transfer, or
	 *  subclass returned false during download process to stop continuing download
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool get(__int64 range_from = -1, __int64 range_to = -1,
		const char* req_body = NULL, size_t len = 0);
#else
	bool get(long long int range_from = -1, long long int range_to = -1,
		const char* req_body = NULL, size_t len = 0);
#endif

	/**
	 * Reset internal request state
	 * @param url {const char*} When not empty, uses this URL to replace URL input in constructor,
	 *  otherwise still uses url used in constructor
	 * @param addr {const char*} When not empty, sets server address (format:
	 *  ip[|domain]:port, otherwise server address is extracted from url
	 * @return {bool} Returns false indicates url is illegal
	 */
	bool reset(const char* url = NULL, const char* addr = NULL);

	/**
	 * Get url input by constructor or reset function
	 * @return {const char*} Returns NULL indicates input url is illegal
	 */
	const char* get_url() const;

	/**
	 * Get server address obtained from url input by constructor or reset function, format:
	 * ip[|domain]:port
	 * @return {const char*} Returns NULL indicates input url is illegal
	 */
	const char* get_addr() const;

protected:
	/**
	 * Callback function after sending HTTP request data and reading HTTP server response header
	 * @param conn {http_client*}
	 * @return {bool} If subclass returns false, stops continuing download
	 */
	virtual bool on_response(http_client* conn);

	/**
	 * Callback function after getting complete file length returned by server
	 * @param n {__int64} Complete file length
	 * @return {bool} If subclass returns false, stops continuing download
	 */
#if defined(_WIN32) || defined(_WIN64)
	virtual bool on_length(__int64 n);
#else
	virtual bool on_length(long long int n);
#endif

	/**
	 * During download process, notify subclass of downloaded data and data length while downloading
	 * @param data {const void*} Address of downloaded data
	 * @param len {size_t} Length of downloaded data
	 * @return {bool} If subclass returns false, stops continuing download
	 */
	virtual bool on_save(const void* data, size_t len) = 0;

private:
	char* url_;
	char  addr_[128];
	http_request* req_;

	// Download entire file from beginning
	bool save_total(const char* body, size_t len);

	// Resume download partial file
#if defined(_WIN32) || defined(_WIN64)
	bool save_range(const char* body, size_t len,
		__int64 range_from, __int64 range_to);
#else
	bool save_range(const char* body, size_t len,
		long long int range_from, long long int range_to);
#endif

	// Start download
	bool save(http_request* req);
};

} // namespace acl

