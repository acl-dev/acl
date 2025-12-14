#pragma once
#include "../acl_cpp_define.hpp"
#include "ipc_service.hpp"
#if defined(_WIN32) || defined(_WIN64)
 struct acl_pthread_mutex_t;
 struct acl_pthread_cond_t;
#else
# include <pthread.h>
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
# ifndef	acl_pthread_cond_t
#  define	acl_pthread_cond_t	pthread_cond_t
# endif
#endif

namespace acl {

class ipc_client;
class rpc_client;
class rpc_service;

class rpc_request;
struct RPC_DAT
{
	rpc_request* req;
	void* ctx;
};

class ACL_CPP_API rpc_request : public ipc_request
{
public:
	rpc_request(void);
	virtual ~rpc_request(void);

protected:
	friend class rpc_client;
	friend class rpc_service;

	/**
	 * Called in main thread. Subclasses must implement this interface.
	 * This interface will be called after child thread finishes processing request task, so objects of this class can only
	 * be released after this interface is called. Prohibited from releasing objects of this class before calling this interface
	 */
	virtual void rpc_onover(void) = 0;

	/**
	 * Virtual interface: When child thread calls rpc_signal of this object, this interface will be
	 * called in main thread to notify that intermediate state information from child thread running is received before task is completed
	 * (i.e., before calling rpc_onover). Internally automatically supports socket or _WIN32 window
	 * messages. Application scenario, e.g., for HTTP download application, in child thread can
	 * download while sending (calling rpc_signal method) download progress to main thread,
	 * then main thread will call this method of this class instance to handle this message
	 */
	virtual void rpc_wakeup(void* ctx) { (void) ctx; }

protected:
	/**
	 * Called in child thread. Subclasses must implement this interface for handling specific tasks
	 */
	virtual void rpc_run(void) = 0;

	/**
	 * Called in child thread. Internally automatically supports socket or _WIN32 window messages.
	 * rpc_run method of subclass instance can call this method multiple times to send messages to this class instance in main thread.
	 * Main thread calls rpc_wakeup method of this object
	 * @param ctx {void*} Parameter pointer passed. Generally should be dynamic address
	 *  which is better, avoiding problem of same parameter being repeatedly overwritten
	 */
	void rpc_signal(void* ctx);

	/**
	 * After child thread calls rpc_signal to main thread, calling this method can wait
	 * for next instruction from main thread
	 * @param timeout {int} Wait timeout (milliseconds). When this value is 0,
	 *  uses non-blocking wait mode. When this value is < 0, uses completely blocking
	 *  wait mode (i.e., waits until main thread sends cond_signal notification). When this
	 *  value > 0, maximum timeout for waiting is timeout milliseconds
	 * @return {bool} Returns true indicates notification signal from main thread was received.
	 *  Otherwise, need to call cond_wait_timeout to determine whether it was caused by timeout
	 */
	bool cond_wait(int timeout = -1);

	/**
	 * When cond_wait returns false, application should call this method to determine whether
	 * it was caused by wait timeout
	 * @return {bool} Whether wait timed out
	 */
	bool cond_wait_timeout() const
	{
		return wait_timedout_;
	}

	/**
	 * When child thread calls cond_wait, call this method in main thread to notify child thread
	 * to "wake up"
	 * @return {bool} When child thread calls cond_wait, this function notifies child thread
	 *  to "wake up" and returns true, otherwise returns false
	 */
	bool cond_signal(void);

private:
	RPC_DAT dat_;
	ipc_client* ipc_;
	int   cond_count_;
	acl_pthread_mutex_t* lock_;
	acl_pthread_cond_t* cond_;
	bool wait_timedout_;

	// Base class ipc_request virtual function, called in child thread
	virtual void run(ipc_client* ipc);
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * Virtual interface. Subclasses implement this class for handling specific tasks. This interface is suitable
	 * for _WIN32 message mode
	 * @param hWnd {HWND} WIN2 window handle
	 */
	virtual void run(HWND hWnd);
#endif
};

//////////////////////////////////////////////////////////////////////////

class aio_socket_stream;

class ACL_CPP_API rpc_service : public ipc_service
{
public:
	/**
	 * Constructor
	 * @param nthread {int} If this value > 1, internally automatically uses thread pool, otherwise
	 *  it is one request per thread
	 * @param ipc_keep {bool} Whether internal IPC message stream maintains long connection. Maintaining long
	 *  connection helps improve message passing efficiency
	 */
	rpc_service(int nthread, bool ipc_keep = true);

	~rpc_service(void) {}

	/**
	 * Run in main thread: Put request task into task request queue of child thread pool. When a
	 * child thread in thread pool receives this task, calls rpc_request::rpc_run method to call
	 * subclass method. After task processing is complete, sends message to main thread, then calls
	 * rpc_request::rpc_callback in main thread
	 * @param req {rpc_request*} rpc_request subclass instance, non-empty
	 */
	void rpc_fork(rpc_request* req);

private:
	// Base class virtual function: Callback function when main thread object receives IPC
	// connection request from child thread message
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
};

} // namespace acl

