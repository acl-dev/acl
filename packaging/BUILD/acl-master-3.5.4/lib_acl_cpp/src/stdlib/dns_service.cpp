#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <list>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/ipc/ipc_client.hpp"
#include "acl_cpp/ipc/ipc_service.hpp"
#include "acl_cpp/stdlib/dns_service.hpp"
#endif

namespace acl
{

struct DNS_IPC_DATA
{
	dns_res* res;
};


enum
{
	IPC_REQ,
	IPC_RES
};

///////////////////////////////////////////////////////////////////////

class dns_request : public ipc_request
{
public:
	dns_request(const char* domain) : domain_(domain) {}

	~dns_request(void) {}

	virtual void run(ipc_client* ipc)
	{
		ACL_DNS_DB* db = acl_gethostbyname(domain_.c_str(), NULL);
		data_.res = NEW dns_res(domain_.c_str());

		if (db != NULL) {
			ACL_ITER iter;
			acl_foreach(iter, db) {
				ACL_HOSTNAME* hn = (ACL_HOSTNAME*) iter.data;
				data_.res->ips_.push_back(hn->ip);
			}

			acl_netdb_free(db);
		}

		// 向主线程发送结果
		ipc->send_message(IPC_RES, &data_, sizeof(data_));

		// 销毁本类对象，因为其是动态分配的
		delete this;
	}

#ifdef ACL_WINDOWS

	// 基类虚接口，使子线程可以在执行完任务后向主线程发送 ACL_WINDOWS 窗口消息

	virtual void run(HWND hWnd)
	{
		ACL_DNS_DB* db = acl_gethostbyname(domain_.c_str(), NULL);
		DNS_IPC_DATA* data = (DNS_IPC_DATA*)
			acl_mymalloc(sizeof(DNS_IPC_DATA));
		data->res = NEW dns_res(domain_.c_str());

		if (db != NULL) {
			ACL_ITER iter;
			acl_foreach(iter, db) {
				ACL_HOSTNAME* hn = (ACL_HOSTNAME*) iter.data;
				data->res->ips_.push_back(hn->ip);
			}

			acl_netdb_free(db);
		}

		// 向主线程发送结果
		::PostMessage(hWnd, IPC_RES + WM_USER, 0, (LPARAM) data);

		// 销毁本类对象，因为其是动态分配的
		delete this;
	}
#endif
private:
	string  domain_;
	DNS_IPC_DATA data_;
};

///////////////////////////////////////////////////////////////////////

class dns_ipc : public ipc_client
{
public:
	dns_ipc(dns_service* server, acl_int64 magic)
	: ipc_client(magic)
	, server_(server)
	{
	}

	~dns_ipc(void) {}

	virtual void on_message(int nMsg acl_unused,
		void* data, int dlen acl_unused)
	{
		if (nMsg != IPC_RES) {
			logger_error("invalid nMsg(%d)", nMsg);
			this->close();
			return;
		}

		DNS_IPC_DATA* dat = (DNS_IPC_DATA*) data;
		dns_res* res = dat->res;

		server_->on_result(*res);
		delete res;
	}
protected:
	virtual void on_close(void)
	{
		delete this;
	}

private:
	dns_service* server_;
};

///////////////////////////////////////////////////////////////////////
#ifdef ACL_WINDOWS
#include <process.h>
#endif

dns_service::dns_service(int nthread /* = 1 */, bool win32_gui /* = false */)
: ipc_service(nthread, win32_gui)
{
#ifdef ACL_WINDOWS
	magic_ = _getpid() + time(NULL);
#else
	magic_ = getpid() + time(NULL);
#endif
}

dns_service::~dns_service(void) {}

void dns_service::on_accept(aio_socket_stream* client)
{
	ipc_client* ipc = NEW dns_ipc(this, magic_);
	ipc->open(client);

	// 添加消息回调对象
	ipc->append_message(IPC_RES);
	ipc->wait();
}

#ifdef ACL_WINDOWS

void dns_service::win32_proc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg != IPC_RES + WM_USER) {
		logger_error("invalid nMsg(%d)", nMsg);
		return;
	} else if (lParam == 0) {
		logger_error("lParam invalid");
		return;
	}

	DNS_IPC_DATA* dat = (DNS_IPC_DATA*) lParam;
	dns_res* res = dat->res;

	on_result(*res);
	delete res;

	// 在采用 ACL_WINDOWS 消息时该对象空间是动态分配的，所以需要释放
	acl_myfree(dat);
}

#endif

void dns_service::lookup(dns_result_callback* callback)
{
	std::list<dns_result_callback*>::iterator it;
	const char* domain = callback->get_domain().c_str();

	for (it= callbacks_.begin(); it != callbacks_.end(); ++it) {
		if ((*it)->get_domain() == domain) {
			callbacks_.push_back(callback);
			return;
		}
	}

	callbacks_.push_back(callback);

	ipc_request* req = NEW dns_request(domain);

	// 调用基类 ipc_service 请求过程
	request(req);
}

void dns_service::on_result(const dns_res& res)
{
	for (std::list<dns_result_callback*>::iterator it= callbacks_.begin();
		it != callbacks_.end();) {

		if ((*it)->get_domain() == res.domain_.c_str()) {
			// 通知请求对象的解析结果
			(*it)->on_result((*it)->get_domain(), res);
			(*it)->destroy(); // 调用请求对象的销毁过程
			it = callbacks_.erase(it);
		} else {
			++it;
		}
	}
}

}  // namespace acl
