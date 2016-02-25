#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/ipc/ipc_client.hpp"
#include "acl_cpp/ipc/rpc.hpp"
#endif

namespace acl
{

// 所有消息类型定义
enum
{
	RPC_MSG,
	RPC_SIG
};

#ifdef ACL_WINDOWS
#define RPC_WIN32_MSG	(WM_USER + 100)
#define RPC_WIN32_SIG	(WM_USER + 101)
#endif

// IPC 通信时，子线程通过发送此数据通知主线程任务处理结果
/*
struct IPC_DAT 
{
	rpc_request* req;
	void* ctx;
};
*/

rpc_request::rpc_request()
: ipc_(NULL)
, wait_timedout_(false)
{
	dat_.req = this;
	dat_.ctx = NULL;

	cond_count_ = 0;
	cond_ = (acl_pthread_cond_t*) acl_mycalloc(
		1, sizeof(acl_pthread_cond_t));
	acl_pthread_cond_init(cond_, NULL);

	lock_ = (acl_pthread_mutex_t*) acl_mycalloc(
		1, sizeof(acl_pthread_mutex_t));
	acl_pthread_mutex_init(lock_, NULL);
}

rpc_request::~rpc_request()
{
	acl_pthread_mutex_destroy(lock_);
	acl_myfree(lock_);

	acl_pthread_cond_destroy(cond_);
	acl_myfree(cond_);
}

// 该函数在子线程中被调用
void rpc_request::run(ipc_client* ipc)
{
	ipc_ = ipc;
	rpc_run();
	/*
	IPC_DAT data;
	data.req = this;
	data.ctx = NULL;
	// 向主线程发送结果
	ipc->send_message(RPC_MSG, &data, sizeof(data));
	*/
	dat_.ctx = NULL;

	// 向主线程发送结果
	ipc->send_message(RPC_MSG, &dat_, sizeof(RPC_DAT));
}

// 该函数在子线程中被调用
#ifdef ACL_WINDOWS
void rpc_request::run(HWND hWnd)
{
	rpc_run();
	// 向窗口句柄发消息，将当前对象的地址发给主线程
	::PostMessage(hWnd, RPC_WIN32_MSG, 0, (LPARAM) this);
}
#endif

// 该函数在子线程中被调用
void rpc_request::rpc_signal(void* ctx)
{
#ifdef ACL_WINDOWS
	HWND hWnd = get_hwnd();
	if (hWnd != NULL)
	{
		// 向窗口句柄发消息，将当前对象的地址发给主线程
		::PostMessage(hWnd, RPC_WIN32_SIG, (WPARAM) ctx, (LPARAM) this);
		return;
	}
#endif
	acl_assert(ipc_ != NULL);
	/*
	IPC_DAT data;
	data.req = this;
	data.ctx = ctx;
	// 向主线程发送结果
	ipc_->send_message(RPC_SIG, &data, sizeof(data));
	*/
	dat_.ctx = ctx;
	// 向主线程发送结果
	ipc_->send_message(RPC_SIG, &dat_, sizeof(RPC_DAT));
}

bool rpc_request::cond_wait(int timeout /* = -1 */)
{
	int status;

	status = acl_pthread_mutex_lock(lock_);
	if (status != 0)
	{
		logger_error("pthread_mutex_lock error: %d", status);
		return false;
	}

	if (--cond_count_ >= 0)
	{
		status = acl_pthread_mutex_unlock(lock_);
		if (status != 0)
		{
			logger_error("pthread_mutex_unlock error: %d", status);
			return false;
		}
		return true;
	}

	if (timeout < 0)
	{
		status = acl_pthread_cond_wait(cond_, lock_);
		if (status != 0)
		{
			logger_error("pthread_cond_wait error: %d", status);
			status = acl_pthread_mutex_unlock(lock_);
			if (status != 0)
				logger_error("pthread_mutex_unlock error: %d", status);
			return false;
		}
		status = acl_pthread_mutex_unlock(lock_);
		if (status != 0)
		{
			logger_error("pthread_mutex_unlock error: %d", status);
			return false;
		}
		return true;
	}

	struct  timeval   tv;
	struct	timespec  when_ttl;
	gettimeofday(&tv, NULL);
	when_ttl.tv_sec = tv.tv_sec + timeout / 1000;
	when_ttl.tv_nsec = tv.tv_usec * 1000 + (timeout % 1000) * 1000;
	wait_timedout_ = false;

	status = acl_pthread_cond_timedwait(cond_, lock_, &when_ttl);
	if (status != 0)
	{
		if (status == ACL_ETIMEDOUT)
			wait_timedout_ = true;
		else
			logger_error("pthread_cond_timedwait error: %d", status);
		status = acl_pthread_mutex_unlock(lock_);
		if (status != 0)
			logger_error("pthread_mutex_unlock error: %d", status);
		return false;
	}
	else
	{
		status = acl_pthread_mutex_unlock(lock_);
		if (status != 0)
		{
			logger_error("pthread_mutex_unlock error: %d", status);
			return false;
		}
		return true;
	}
}

bool rpc_request::cond_signal()
{
	int status;

	status = acl_pthread_mutex_lock(lock_);
	if (status != 0)
	{
		logger_error("pthread_mutex_lock error: %d", status);
		return false;
	}

	cond_count_++;

	status = acl_pthread_cond_signal(cond_);
	if (status != 0)
	{
		(void) acl_pthread_mutex_unlock(lock_);
		logger_error("pthread_cond_signal error: %d", status);
		return false;
	}

	status = acl_pthread_mutex_unlock(lock_);
	if (status != 0)
	{
		logger_error("pthread_mutex_unlock error: %d", status);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

// 该类实例在主线程中运行

class rpc_client : public ipc_client
{
public:
	rpc_client(acl_int64 magic) : ipc_client(magic) {}
	~rpc_client(void) {}

protected:
	/**
	 * 基类虚接口：当收到消息时的回调函数
	 * @param nMsg {int} 用户添加的自定义消息值
	 * @param data {void*} 消息数据
	 * @param dlen {int} 消息数据的长度
	 */
	virtual void on_message(int nMsg, void* data, int dlen)
	{
		acl_assert(data && dlen == sizeof(RPC_DAT));
		RPC_DAT* dat = (RPC_DAT*) data;
		acl_assert(dat->req);

		if (nMsg == RPC_MSG)
			dat->req->rpc_onover();
		else if (nMsg == RPC_SIG)
			dat->req->rpc_wakeup(dat->ctx);
	}

	// 基类虚接口
	virtual void on_close(void)
	{
		delete this;
	}
private:
};

#ifdef ACL_WINDOWS
#include <process.h>
#endif

rpc_service::rpc_service(int nthread, bool ipc_keep /* = true */)
: ipc_service(nthread, ipc_keep)
{
#ifdef ACL_WINDOWS
	magic_ = _getpid() + time(NULL);
#else
	magic_ = getpid() + time(NULL);
#endif
}

void rpc_service::on_accept(aio_socket_stream* client)
{
	// 创建接收来自于子线程消息的 IPC 连接对象
	ipc_client* ipc = new rpc_client(magic_);
	ipc->open(client);

	// 添加消息回调对象
	ipc->append_message(RPC_MSG);
	ipc->append_message(RPC_SIG);
	ipc->wait();
}

#ifdef ACL_WINDOWS
void rpc_service::win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == RPC_WIN32_MSG)
	{
		rpc_request* req = (rpc_request*) lParam;
		acl_assert(req);
		req->rpc_onover();
	}
	else if (msg == RPC_WIN32_SIG)
	{
		rpc_request* req = (rpc_request*) lParam;
		acl_assert(req);
		void* ctx = (void*) wParam;
		req->rpc_wakeup(ctx);
	}
}
#endif

void rpc_service::rpc_fork(rpc_request* req)
{
	request(req);
}

} // namespace acl
