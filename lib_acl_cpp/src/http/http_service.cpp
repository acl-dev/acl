#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/ipc/ipc_client.hpp"
#include "acl_cpp/http/http_service.hpp"
#endif

namespace acl
{

//////////////////////////////////////////////////////////////////////////

http_service_request::http_service_request(const char* domain,
	unsigned short port)
{
	acl_assert(domain && *domain);
	domain_ = acl_mystrdup(domain);
	port_ = port;
}

http_service_request::~http_service_request(void)
{
	acl_myfree(domain_);
}

const string* http_service_request::get_body()
{
	return (NULL);
}

const char* http_service_request::get_domain(void) const
{
	return (domain_);
}

unsigned short http_service_request::get_port(void) const
{
	return (port_);
}

//////////////////////////////////////////////////////////////////////////

enum
{
	HTTP_MSG_HDR,
	HTTP_MSG_DAT,
	HTTP_MSG_ERR,
};

struct HTTP_IPC_DAT 
{
	http_service_request* callback;
	struct
	{
		HTTP_HDR_RES* hdr_res;
		char addr[64];
	} http_hdr;
	struct 
	{
		char* ptr;
		size_t dlen;
	} http_dat;
	http_status_t error;
#define i_hdr_res	http_hdr.hdr_res
#define i_addr		http_hdr.addr
#define i_ptr		http_dat.ptr
#define i_dlen		http_dat.dlen
#define i_error		error
};

//////////////////////////////////////////////////////////////////////////

class http_ipc_request : public ipc_request
{
public:
	http_ipc_request(http_service_request* callback, int nwait)
	{
		nwait_ = nwait;
		memset(&data_, 0, sizeof(data_));
		data_.callback = callback;
		hdr_res_ = NULL;
		res_ = NULL;
		vstream_ = NULL;
		respond_over_ = false;
	}

	~http_ipc_request(void)
	{
		if (res_)
		{
			res_->hdr_res = NULL;
			http_res_free(res_);
		}
	}

	// 基类 ipc_request 会自动调用此回调处理请求过程
	virtual void run(ipc_client* ipc)
	{
		unsigned int nredirect_limit = data_.callback->get_redirect();
		unsigned int nredirect = 0;
		http_status_t ret;

		while (true)
		{
			if ((ret = connect_server()) != HTTP_OK)
			{
				report_error(ipc, ret);
				break;
			}
			if ((ret = send_request()) != HTTP_OK)
			{
				report_error(ipc, ret);
				break;
			}
			if ((ret = read_respond_hdr()) != HTTP_OK)
			{
				report_error(ipc, ret);
				break;
			}

			acl_assert(hdr_res_);

			// 如果服务器返回的是重定向信息，则进行重定向访问

			if ((hdr_res_->reply_status != 301
				&& hdr_res_->reply_status != 302)
				|| nredirect_limit == 0)
			{
				// 发送响应头收到消息给主线程
				report(ipc, HTTP_MSG_HDR);

				// 连续接收响应体并发响应体收到消息给主线程
				read_respond_body(ipc);
				break;
			}

			// 开始重定向过程

			nredirect++;
			// 防止重定向次数太多而造成了循环
			if (nredirect > nredirect_limit)
			{
				report_error(ipc, HTTP_ERR_REDIRECT_MAX);
				break;
			}

			const char* url = http_hdr_entry_value(
				&hdr_res_->hdr, "Location");

			if (url == NULL)
			{
				logger_error("redirect Location null");
				report_error(ipc, HTTP_ERR_INVALID_HDR);
				break;
			}
			if (data_.callback->redirect(url) == false)
			{
				http_hdr_res_free(hdr_res_);
				hdr_res_ = NULL;
				report_error(ipc, HTTP_ERR_INVALID_HDR);
				break;
			}
			http_hdr_res_free(hdr_res_);
			hdr_res_ = NULL;
			data_.callback->redicrect_reset();
		}

		delete this;
	}

#ifdef ACL_WINDOWS
	// 基类会自动调用此回调处理请求过程
	// 基类虚接口，使子线程可以在执行完任务后向主线程发送 ACL_WINDOWS 窗口消息
	virtual void run(HWND hWnd)
	{
		unsigned int nredirect_limit = data_.callback->get_redirect();
		unsigned int nredirect = 0;
		http_status_t ret;

		while (true)
		{
			if ((ret = connect_server()) != HTTP_OK)
			{
				report_error(hWnd, ret);
				break;
			}
			if ((ret = send_request()) != HTTP_OK)
			{
				report_error(hWnd, ret);
				break;
			}
			if ((ret = read_respond_hdr()) != HTTP_OK)
			{
				report_error(hWnd, ret);
				break;
			}

			acl_assert(hdr_res_);

			// 如果服务器返回的是重定向信息，则进行重定向访问

			if ((hdr_res_->reply_status != 301
				&& hdr_res_->reply_status != 302)
				|| nredirect_limit == 0)
			{
				// 发送响应头收到消息给主线程
				report(hWnd, HTTP_MSG_HDR);

				// 连续接收响应体并发响应体收到消息给主线程
				read_respond_body(hWnd);
				break;
			}

			// 开始重定向过程

			nredirect++;
			// 防止重定向次数太多而造成了循环
			if (nredirect > nredirect_limit)
			{
				report_error(hWnd, HTTP_ERR_REDIRECT_MAX);
				break;
			}

			const char* url = http_hdr_entry_value(
				&hdr_res_->hdr, "Location");

			if (url == NULL)
			{
				logger_error("redirect Location null");
				report_error(hWnd, HTTP_ERR_INVALID_HDR);
				break;
			}
			if (data_.callback->redirect(url) == false)
			{
				http_hdr_res_free(hdr_res_);
				hdr_res_ = NULL;
				report_error(hWnd, HTTP_ERR_INVALID_HDR);
				break;
			}
			http_hdr_res_free(hdr_res_);
			hdr_res_ = NULL;
			data_.callback->redicrect_reset();
		}

		delete this;
	}
#endif
protected:
private:
	int  nwait_;
	HTTP_IPC_DAT data_;
	socket_stream client_;
	ACL_VSTREAM* vstream_;
	//char  addr_[64];
	HTTP_HDR_RES* hdr_res_;
	HTTP_RES* res_;
	bool respond_over_;

	void report_error(ipc_client* ipc, http_status_t errnum)
	{
		data_.i_hdr_res = hdr_res_;
		data_.i_error = errnum;
		ipc->send_message(HTTP_MSG_ERR, &data_, sizeof(data_));
	}

	void report(ipc_client* ipc, unsigned int nMsg)
	{
		data_.i_hdr_res = hdr_res_;
		data_.i_error = HTTP_OK;
		ipc->send_message(nMsg, &data_, sizeof(data_));
	}

#ifdef ACL_WINDOWS
	void report_error(HWND hWnd, http_status_t errnum)
	{
		HTTP_IPC_DAT* data = (HTTP_IPC_DAT*)
			acl_mymalloc(sizeof(HTTP_IPC_DAT));
		memcpy(data, &data_, sizeof(HTTP_IPC_DAT));
		data->i_hdr_res = hdr_res_;
		data->i_error = errnum;
		// 向主线程发送结果
		::PostMessage(hWnd, HTTP_MSG_ERR + WM_USER, 0, (LPARAM) data);
	}

	void report(HWND hWnd, UINT nMsg)
	{
		HTTP_IPC_DAT* data = (HTTP_IPC_DAT*)
			acl_mymalloc(sizeof(HTTP_IPC_DAT));
		memcpy(data, &data_, sizeof(HTTP_IPC_DAT));
		data->i_hdr_res = hdr_res_;
		data->i_error = HTTP_OK;
		::PostMessage(hWnd, nMsg + WM_USER, 0, (LPARAM) data);
	}
#endif

	http_status_t connect_server(void)
	{
		string addr(data_.callback->get_domain());
		addr << ':' << data_.callback->get_port();

		if (client_.opened())
			client_.close();

		if (client_.open(addr.c_str(), 60, 60) == false)
		{
			logger_error("connect %s error(%s)",
				addr.c_str(), acl_last_serror());
			return (HTTP_ERR_CONN);
		}

		vstream_ = client_.get_vstream();
		return (HTTP_OK);
	}

	http_status_t send_request(void)
	{
		string hdr_req;
		if (data_.callback->build_request(hdr_req) == false)
			return (HTTP_ERR_REQ);

		// 写 HTTP 请求头
		if (client_.write(hdr_req.c_str(), hdr_req.length()) == false)
			return (HTTP_ERR_SEND);

		// 循环从请求对象中获得请求体数据，并写 HTTP 请求体数据
		while (true)
		{
			const string* data = data_.callback->get_body();
			if (data == NULL || data->empty())
				break;
			if (client_.write(data->c_str(), data->length()) == false)
				return (HTTP_ERR_SEND);
		}
		return (HTTP_OK);
	}

	http_status_t read_respond_hdr(void)
	{
		acl_assert(vstream_);
		safe_snprintf(data_.i_addr, sizeof(data_.i_addr),
			"%s", ACL_VSTREAM_PEER(vstream_));
		hdr_res_ = http_hdr_res_new();
		int ret = http_hdr_res_get_sync(hdr_res_, vstream_, 60);
		if (ret < 0)
		{
			logger_error("get http respond hdr from %s error %s",
				data_.i_addr, acl_last_serror());
			http_hdr_res_free(hdr_res_);
			hdr_res_ = NULL;
			return (HTTP_ERR_READ_HDR);
		}

		if (http_hdr_res_parse(hdr_res_) < 0)
		{
			logger_error("parse http respond hdr error from %s",
				data_.i_addr);

			http_hdr_res_free(hdr_res_);
			hdr_res_ = NULL;
			return (HTTP_ERR_INVALID_HDR);
		}

		return (HTTP_OK);
	}

	void read_respond_body(ipc_client* ipc)
	{
		acl_assert(hdr_res_);

		/* 如果 HTTP 响应没有数据体则仅输出 HTTP 响应头即可 */

		if (hdr_res_->hdr.content_length == 0
			|| (hdr_res_->hdr.content_length == -1
			&& hdr_res_->reply_status > 300
			&& hdr_res_->reply_status < 400))
		{
			data_.i_ptr = NULL;
			data_.i_dlen = 0;
			// 如果没有数据体，也发送消息通知调用者数据结束
			report(ipc, HTTP_MSG_DAT);
			return;
		}

		res_ = http_res_new(hdr_res_);

#define BUF_LEN		8192

		char* buf;
		int   ret;

		while (true)
		{
			buf = (char*) acl_mymalloc(BUF_LEN);
			ret = read_respond_body(buf, BUF_LEN - 1);
			if (ret <= 0)
			{
				acl_myfree(buf);
				break;
			}
			buf[ret] = 0;

			data_.i_ptr = buf;
			data_.i_dlen = ret;
			report(ipc, HTTP_MSG_DAT);

			if (respond_over_)
				break;
			if (nwait_ > 0)
				acl_doze(nwait_);
		}

		data_.i_ptr = NULL;
		data_.i_dlen = 0;
		report(ipc, HTTP_MSG_DAT);
	}

#ifdef ACL_WINDOWS
	void read_respond_body(HWND hWnd)
	{
		acl_assert(hdr_res_);

		/* 如果 HTTP 响应没有数据体则仅输出 HTTP 响应头即可 */

		if (hdr_res_->hdr.content_length == 0
			|| (hdr_res_->hdr.content_length == -1
			&& hdr_res_->reply_status > 300
			&& hdr_res_->reply_status < 400))
		{
			data_.i_ptr = NULL;
			data_.i_dlen = 0;
			// 如果没有数据体，也发送消息通知调用者数据结束
			report(hWnd, HTTP_MSG_DAT);
			return;
		}

		res_ = http_res_new(hdr_res_);

#define BUF_LEN		8192

		char* buf;
		int   ret;

		while (true)
		{
			buf = (char*) acl_mymalloc(BUF_LEN);
			ret = read_respond_body(buf, BUF_LEN - 1);
			if (ret <= 0)
			{
				acl_myfree(buf);
				break;
			}
			buf[ret] = 0;

			data_.i_ptr = buf;
			data_.i_dlen = (size_t) ret;
			report(hWnd, HTTP_MSG_DAT);

			if (respond_over_)
				break;
			if (nwait_ > 0)
				acl_doze(nwait_);
		}

		data_.i_ptr = NULL;
		data_.i_dlen = 0;
		report(hWnd, HTTP_MSG_DAT);
	}
#endif

	int read_respond_body(char* buf, int dlen)
	{
		acl_assert(vstream_);
		acl_assert(res_);

		char* ptr = buf;
		int   dlen_saved = dlen;
		int   ret;

		// 因为 buf 是动态分配的，所以应尽量让 buf 填满，
		// 这样可以节省内存分配的消耗

		while (true)
		{
			ret = (int) http_res_body_get_sync(res_, vstream_,
					ptr, dlen);
			if (ret <= 0)
			{
				respond_over_ = true;
				break;
			}
			dlen -= ret;
			if (dlen <= 0)
				break;
			ptr += ret;
		}

		return (dlen_saved - dlen);
	}
};

//////////////////////////////////////////////////////////////////////////

class http_ipc : public ipc_client
{
public:
	http_ipc(acl_int64 magic) : ipc_client(magic)
	{

	}

	~http_ipc(void)
	{

	}

	virtual void on_message(int nMsg, void* data, int dlen)
	{
		acl_assert(dlen == sizeof(HTTP_IPC_DAT));
		HTTP_IPC_DAT* dat = (HTTP_IPC_DAT*) data;

		switch (nMsg)
		{
		case HTTP_MSG_HDR:
			dat->callback->on_hdr(dat->i_addr, dat->i_hdr_res);
			break;
		case HTTP_MSG_DAT:
			dat->callback->on_body(dat->i_ptr, dat->i_dlen);
			if (dat->i_ptr && dat->i_dlen > 0)
				acl_myfree(dat->i_ptr);
			else
			{
				// 调用请求对象的销毁过程
				dat->callback->destroy();
				if (dat->i_hdr_res)
					http_hdr_res_free(dat->i_hdr_res);
				this->close();  // 自动触发析构过程
			}
			break;
		case HTTP_MSG_ERR:
			dat->callback->on_error(dat->i_error);
			// 调用请求对象的销毁过程
			dat->callback->destroy();
			if (dat->i_hdr_res)
				http_hdr_res_free(dat->i_hdr_res);
			this->close();  // 自动触发析构过程
			break;
		default:
			// 出现了异常消息
			logger_error("unknown ipc msg: %d", nMsg);
			break;
		}
	}
protected:
	virtual void on_close(void)
	{
		delete this;
	}
private:
	//HTTP_HDR_RES* hdr_res_;
};

//////////////////////////////////////////////////////////////////////////
#ifdef ACL_WINDOWS
#include <process.h>
#endif

http_service::http_service(int nthread /* = 1 */, int nwait,
	bool win32_gui /* = false */)
: ipc_service(nthread, win32_gui)
, addr_(NULL)
, nwait_(nwait)
, handle_type_(ENGINE_SELECT)
{
#ifdef ACL_WINDOWS
	magic_ = _getpid() + time(NULL);
#else
	magic_ = getpid() + time(NULL);
#endif
}

http_service::~http_service()
{
	if (addr_)
		acl_myfree(addr_);
}

#ifdef ACL_WINDOWS

void http_service::win32_proc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (lParam == 0)
		return;

	HTTP_IPC_DAT* dat = (HTTP_IPC_DAT*) lParam;

	switch (nMsg - WM_USER)
	{
	case HTTP_MSG_HDR:
		dat->callback->on_hdr(dat->i_addr, dat->i_hdr_res);
		acl_myfree(dat);
		break;
	case HTTP_MSG_DAT:
		dat->callback->on_body(dat->i_ptr, dat->i_dlen);
		if (dat->i_ptr && dat->i_dlen > 0)
			acl_myfree(dat->i_ptr);
		else
		{
			// 调用请求对象的销毁过程
			dat->callback->destroy();
			if (dat->i_hdr_res)
				http_hdr_res_free(dat->i_hdr_res);
		}
		acl_myfree(dat);
		break;
	case HTTP_MSG_ERR:
		dat->callback->on_error(dat->i_error);
		// 调用请求对象的销毁过程
		dat->callback->destroy();
		if (dat->i_hdr_res)
			http_hdr_res_free(dat->i_hdr_res);
		acl_myfree(dat);
		break;
	default:
		// 出现了异常消息
		logger_error("unknown ipc msg: %d", nMsg);
		break;
	}
}

#endif

void http_service::on_accept(aio_socket_stream* client)
{
	ipc_client* ipc = NEW http_ipc(magic_);
	ipc->open(client);

	// 添加消息处理

	ipc->append_message(HTTP_MSG_HDR);
	ipc->append_message(HTTP_MSG_DAT);
	ipc->append_message(HTTP_MSG_ERR);

	// 异步等待消息
	ipc->wait();
}

void http_service::on_open(const char*addr)
{
	aio_handle& handle = get_handle();
	handle_type_ = handle.get_engine_type();
	addr_ = acl_mystrdup(addr);
}

void http_service::on_close(void)
{

}

void http_service::do_request(http_service_request* req)
{
	http_ipc_request* ipc_req = NEW http_ipc_request(req, nwait_);
	// 调用基类 ipc_service 请求过程
	request(ipc_req);
}

}
