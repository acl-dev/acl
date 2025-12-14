#pragma once
#include "../stream/aio_handle.hpp"
#include "../ipc/ipc_service.hpp"
#include "http_header.hpp"

namespace acl {

class string;

/**
 * HTTP service request class. Subclasses must inherit this class
 */
class ACL_CPP_API http_service_request : public http_header {
public:
	/**
	 * Constructor
	 * @param domain {const char*} Domain name of HTTP server (can also be IP),
	 * non-empty.
	 *  If empty value is passed, will fatal
	 * @param port {unsigned short} HTTP service port
	 */
	http_service_request(const char* domain, unsigned short port);

	/**
	 * Get domain input by constructor
	 * @return {const char*} Never empty
	 */
	const char* get_domain() const;

	/**
	 * Get port input by constructor
	 * @return {unsigned short}
	 */
	unsigned short get_port() const;

	/**
	 * When task processing is complete or error occurs, internal processing will
	 * automatically call destroy interface.
	 * Subclasses can perform some release process in this interface, especially
	 * when this object is dynamically created,
	 * subclasses should delete this in this function to delete themselves, because
	 * this function will definitely
	 * be called, so subclasses should not perform destruction operations elsewhere
	 */
	virtual void destroy() {}

	//////////////////////////////////////////////////////////////////////
	// Subclasses must implement the following virtual interfaces

	/**
	 * Get HTTP request body data. This function will be called in a loop during
	 * request process until returned data
	 * object's data is empty
	 * @return {const string*} Request body result data. If returns NULL pointer or
	 * returned buffer
	 * object's data is empty (i.e., string->empty()), it indicates HTTP request
	 * body data ends
	 * Note: Different from other functions, this virtual interface is called in
	 * another child thread, so if subclass
	 * implements this interface and needs to call resources that compete with
	 * original thread, should pay attention to lock protection
	 */
	virtual const string* get_body();

	/**
	 * Callback interface when HTTP response header from HTTP server is obtained
	 * @param addr {const char*} Connection address with server, format: IP:PORT
	 * @param hdr {const HTTP_HDR_RES*} HTTP response header. Structure definition
	 * see:
	 *  acl_project/lib_protocol/include/http/lib_http_struct.h
	 */
	virtual void on_hdr(const char* addr, const HTTP_HDR_RES* hdr) = 0;

	/**
	 * Callback interface when HTTP response body from HTTP server is obtained.
	 * When HTTP response body data
	 * is relatively large, this callback will be called multiple times until error
	 * (will call on_error) or data is read completely.
	 * When both parameters of this callback are 0, when data and dlen are both 0,
	 * it indicates reading HTTP
	 * response body ends
	 * @param data {const char*} HTTP response body data during a read operation
	 * @param dlen {size_t} HTTP response body data length during a read operation
	 * Note: If HTTP response only has header data and no body data, this function
	 * will also be called to notify user
	 *     that HTTP session ends
	 */
	virtual void on_body(const char* data, size_t dlen) = 0;

	/**
	 * If error occurs during HTTP request or response process, this interface will
	 * be called to notify subclass of error.
	 * After calling this interface
	 * @param errnum {http_status_t} Error code
	 */
	virtual void on_error(http_status_t errnum) = 0;

protected:
	virtual ~http_service_request();

private:
	char* domain_;
	unsigned short port_;
};

class aio_socket_stream;

class ACL_CPP_API http_service : public ipc_service {
public:
	/**
	 * Constructor
	 * @param nthread {int} If this value > 1, internally automatically uses thread
	 * pool, otherwise
	 *  it is one request per thread
	 * @param nwait {int} When asynchronous engine uses ENGINE_WINMSG, to avoid
	 * blocking main thread's _WIN32 message loop due to task thread sending data
	 * messages too fast,
	 * automatically sleeps for milliseconds when task thread sends data messages.
	 * For other asynchronous engines,
	 *  this value can also be used for rate limiting function
	 * @param win32_gui {bool} Whether it is window class message. If yes, then
	 * internally
	 * communication mode is automatically set to _WIN32 message based, otherwise
	 * still uses common socket
	 *  communication method
	 */
	explicit http_service(int nthread = 1, int nwait = 1, bool win32_gui = false);
	~http_service();

	/**
	 * Application calls this function to start HTTP session process. http_service
	 * class object is responsible
	 * for asynchronously sending HTTP request to server, and asynchronously
	 * reading response from HTTP server
	 * @param req {http_service_request*} HTTP request class object
	 */
	void do_request(http_service_request* req);
protected:
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Base class virtual function. Callback function when win32 message from child
	 * thread is received
	 * @param hWnd {HWND} Window handle
	 * @param msg {UINT} User-defined message number
	 * @param wParam {WPARAM} Parameter
	 * @param lParam {LPARAM} Parameter
	 */
	virtual void win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
	/**
	 * Base class virtual function. Called by base class when new connection
	 * arrives
	 * @param client {aio_socket_stream*} Newly received client connection
	 */
	virtual void on_accept(aio_socket_stream* client);

	/**
	 * Base class virtual function. Callback function after listening stream is
	 * successfully opened
	 * @param addr {const char*} Actual listening address, format: IP:PORT
	 */
	virtual void on_open(const char*addr);

	/**
	 * Base class virtual function. Callback function when listening stream closes
	 */
	virtual void on_close();

private:
	char* addr_;
	int   nwait_;
	aio_handle_type handle_type_;
};

}  // namespace acl

