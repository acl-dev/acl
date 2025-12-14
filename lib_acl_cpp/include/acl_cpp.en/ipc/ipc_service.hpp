#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include "ipc_server.hpp"

struct acl_pthread_pool_t;

namespace acl {

class ipc_client;

class ACL_CPP_API ipc_request : public noncopyable
{
public:
	ipc_request();
	virtual ~ipc_request();

	/**
	 * Virtual interface, subclasses implement this class to handle specific tasks. This interface is suitable
	 * for IO message mode
	 */
	virtual void run(ipc_client* ipc);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Virtual interface, subclasses implement this class to handle specific tasks. This interface is suitable
	 * for _WIN32 message mode
	 */
	virtual void run(HWND hWnd);

	/**
	 * Set _WIN32 window handle
	 * @param hWnd {HWND} Window handle
	 */
	void set_hwnd(HWND hWnd)
	{
		hWnd_ = hWnd;
	}

	/**
	 * Get _WIN32 window handle
	 * @return {HWND} Window handle
	 */
	HWND get_hwnd(void) const
	{
		return (hWnd_);
	}
#endif
private:
#if defined(_WIN32) || defined(_WIN64)
	HWND  hWnd_;
#endif
};

class ACL_CPP_API ipc_service : public ipc_server
{
public:
	/**
	 * Constructor
	 * @param nthread {int} If this value > 1, internally automatically uses thread pool, otherwise
	 *  it is one request per thread
	 * @param ipc_keep {bool} Whether internal IPC message stream maintains long connection. Maintaining long
	 *  connection helps improve message passing efficiency
	 */
	ipc_service(int nthread, bool ipc_keep = true);

	virtual ~ipc_service();

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * When using _WIN32 message mode, subclasses need to implement this virtual interface to handle specific
	 * message process. Subclasses must implement this interface
	 * @param hWnd {HWND} Window handle
	 * @param msg {UINT} User-defined message number
	 * @param wParam {WPARAM} Parameter
	 * @param lParam {LPARAM} Parameter
	 */
	virtual void win32_proc(HWND hWnd, UINT msg,
		WPARAM wParam, LPARAM lParam);
#endif

	/**
	 * Get a connection from IPC message stream connection pool
	 * @return {ipc_client*} Returns NULL if unable to connect to message server
	 */
	ipc_client* peek_conn();

	/**
	 * Put used IPC message connection back into connection pool
	 * @param conn {ipc_client*} IPC message connection stream
	 */
	void push_conn(ipc_client* conn);
protected:
#if defined(_WIN32) || defined(_WIN64)
	__int64   magic_;
#else
	long long int magic_;
#endif

	/**
	 * Subclasses call this function to send request service
	 * @param req {ipc_request*}
	 */
	void request(ipc_request* req);
private:
	bool  ipc_keep_;
	acl_pthread_pool_t* thread_pool_;
#if defined(_WIN32) || defined(_WIN64)
	HWND hWnd_;
	HINSTANCE hInstance_;

	/**
	 * Base class virtual function: In Windows message mode, create hidden window handle
	 */
	virtual bool create_window(void);

	/**
	 * Base class virtual function: In Windows message mode, close hidden window handle
	 */
	virtual void close_window(void);
#endif

	locker lock_;
	std::list<ipc_client*> conn_pool_;
};

}  // namespace acl

