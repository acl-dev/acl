#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../ipc/ipc_service.hpp"
#include "../stream/aio_handle.hpp"
#include "../stream/aio_delay_free.hpp"
#include "string.hpp"

namespace acl {

class ACL_CPP_API dns_res {
public:
	dns_res(const char* domain) : domain_(domain) {}
	~dns_res() { ips_.clear(); }

	string domain_;
	std::list<string> ips_;
protected:
private:
};

class ACL_CPP_API dns_result_callback {
public:
	dns_result_callback(const char* domain) : domain_(domain) {}

	/**
	 * When task processing is complete or error occurs, internal processing will automatically call destroy interface.
	 * Subclasses can perform some release process in this interface, especially when this object is dynamically created,
	 * subclasses should delete this in this function to delete themselves, because this function will definitely
	 * be called, so subclasses should not perform destruction operations elsewhere
	 */
	virtual void destroy() {}

	/**
	 * Subclasses implement this interface to get query results. If res.ips_.size() == 0,
	 * it indicates query result is empty
	 * @param domain {const char*} Domain name entered by user for query
	 * @param res {const dns_res&} Query result set
	 *  Note: In this callback, dns_service object must not be deleted, otherwise it will cause
	 *       illegal memory access, because this callback is called in dns_service,
	 *       and dns_service object will continue to be used after this function returns
	 */
	virtual void on_result(const char* domain, const dns_res& res) = 0;

	/**
	 * Get domain name value set in constructor
	 */
	const string& get_domain() const { return (domain_); }
protected:
	virtual ~dns_result_callback() {}
private:
	string domain_;
};

class ipc_client;

class ACL_CPP_API dns_service
	: public ipc_service
	, public aio_delay_free
{
public:
	/**
	 * Constructor
	 * @param nthread {int} If this value > 1, internally automatically uses thread pool, otherwise
	 *  it is one request per thread
	 * @param win32_gui {bool} Whether it is window class message. If yes, then internally
	 *  communication mode is automatically set to _WIN32 message based, otherwise still uses common socket
	 *  communication method
	 */
	dns_service(int nthread = 1, bool win32_gui = false);
	~dns_service();

	/**
	 * Start domain name resolution process
	 * @param callback {dns_result_callback*} When resolution is complete, callback this class's
	 *  callback function on_result
	 */
	void lookup(dns_result_callback* callback);

	/**
	 * When query thread completes domain name resolution, it will notify the query object in main thread. The query object will
	 * call this callback function to notify main class of query results
	 * @param res {const dns_res&} Query result set
	 */
	void on_result(const dns_res& res);
protected:
	/**
	 * Base class virtual function. Called by base class when new connection arrives
	 * @param client {aio_socket_stream*} Newly received client connection
	 */
	virtual void on_accept(aio_socket_stream* client);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Base class virtual function. Callback function when win32 message from child thread is received
	 * @param hWnd {HWND} Window handle
	 * @param msg {UINT} User-defined message number
	 * @param wParam {WPARAM} Parameter
	 * @param lParam {LPARAM} Parameter
	 */
	virtual void win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
private:
	std::list<dns_result_callback*> callbacks_;
};

}

