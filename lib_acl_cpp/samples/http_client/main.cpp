#include <stdio.h>
#include <stdlib.h>

#include "lib_acl.h"
#include "lib_protocol.h"  // http 协议相关
#include "acl_cpp/lib_acl.hpp"

class http_request : public acl::http_service_request
{
public:
	http_request(const char* domain, unsigned short port,
		acl::aio_handle* handle)
	: acl::http_service_request(domain, port)
	, handle_(handle)
	{
		read_length_ = 0;
	}

	~http_request(void)
	{
		printf("notify aio handle to stop!\r\n");
		// 通知异步事件引擎完全退出
		handle_->stop();
	}
protected:
	//////////////////////////////////////////////////////////////////////////
	// 基类虚接口

	virtual const acl::string* get_body()
	{
		return (NULL);
	}

	// 正常读到 HTTP 响应头时的回调接口
	virtual void on_hdr(const char* addr, const HTTP_HDR_RES* hdr)
	{
		printf(">>server addr: %s, http reply status: %d\n",
			addr, hdr->reply_status);
		http_hdr_print(&hdr->hdr, "http reply hdr");
		content_length_ = hdr->hdr.content_length;
		if (content_length_ > 0)
		{
			if (out_.open_write("test.exe") == false)
				printf("create file error(%s)\n",
					acl_last_serror());
		}
		time(&begin_);
	}

	// 正常读 HTTP 响应体时的回调函数
	virtual void on_body(const char* data, size_t dlen)
	{
		if (data == NULL && dlen == 0)
		{
#ifdef WIN32
			printf("\n>> http reply body over, total: %I64d, %I64d\n",
				content_length_, read_length_);
#else
			printf("\n>> http reply body over, total: %lld, %lld\n",
				content_length_, read_length_);
#endif
			// 出错后，因为本类对象是动态分配的，所以需要在此处释放
			time_t end = time(NULL);
			printf(">>spent %d seconds\n", (int)(end - begin_));

			return;
		}
		read_length_ += dlen;
		http_off_t n =  (read_length_ * 100) / content_length_;
#ifdef WIN32
		printf("%s(%d): n=%I64d%%\r", __FUNCTION__, __LINE__, n);
#else
		printf("%s(%d): n=%lld%%\r", __FUNCTION__, __LINE__, n);
#endif

		if (out_.opened())
			out_.write(data, dlen);
	}

	// 当请求或响应失败时的回调函数
	virtual void on_error(acl::http_status_t errnum)
	{
		printf(">> error: %d\n", (int) errnum);

		// 出错后，因为本类对象是动态分配的，所以需要在此处释放
	}

	virtual void destroy()
	{
		delete this;
	}

private:
	acl::ofstream out_;
	acl::aio_handle* handle_;
	http_off_t  read_length_;
	http_off_t  content_length_;
	time_t begin_;
};

int main()
{
#ifdef WIN32
	acl::acl_cpp_init();
#endif
	acl::atomic_long n;
	n++;
	printf("n=%lld\r\n", n.value());
	getchar();

	acl::aio_handle handle(acl::ENGINE_SELECT);
	acl::http_service* service = new acl::http_service();

	// 使消息服务器监听 127.0.0.1 的地址
	if (service->open(&handle) == false)
	{
		printf(">>open message service error!\n");
		printf(">>enter any key to quit\n");
		getchar();
		return (1);
	}


	// 创建 HTTP 请求过程
	acl::string domain;
	domain = "www.hexun.com";
	//domain = "192.168.1.229";
	//domain = "www.renwou.net";
	http_request* req = new http_request(domain.c_str(), 80, &handle);
	req->set_url("/index.html");
	//req->set_url("/download/banmau_install.exe");
	req->set_host(domain);
	req->set_keep_alive(false);
	req->set_method(acl::HTTP_METHOD_GET);
	req->add_cookie("x-cookie-name", "cookie-value");
	//req->set_redirect(1); // 设置自动重定向的次数限制

	// 通知异步消息服务器处理该 HTTP 请求过程

	//////////////////////////////////////////////////////////////////////////
	//acl::string buf;
	//req->build_request(buf);
	//printf("-----------------------------------------------\n");
	//printf("%s", buf.c_str());
	//printf("-----------------------------------------------\n");
	//////////////////////////////////////////////////////////////////////////

	service->do_request(req);

	while (true)
	{
		if (handle.check() == false)
		{
			printf(">>quit aio process\n");
			break;
		}
	}

	printf("delete http service now\r\n");
	delete service;

	printf("delete delay aio stream\r\n");
	handle.check();

	printf(">>enter any key to exist\n");
	getchar();
	return (0);
}
