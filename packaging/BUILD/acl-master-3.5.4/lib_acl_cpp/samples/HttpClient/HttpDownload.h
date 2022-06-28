#pragma once
#include "lib_acl.h"
#include "lib_protocol.h"  // http 协议相关
#include "acl_cpp/stream/ofstream.hpp"
#include "acl_cpp/http/http_service.hpp"

class acl::string;
class acl::aio_handle;

class CHttpDownload : public acl::http_service_request
{
public:
	CHttpDownload(const char* domain, unsigned short port,
		acl::aio_handle* handle);

#ifdef WIN32
	void SetHWnd(HWND hWnd);
#endif

	// 基类虚接口：销毁过程，由 http_service 类处理完毕后自动调用该回调
	virtual void destroy();

protected:
	~CHttpDownload(void);

	//////////////////////////////////////////////////////////////////////////
	// 基类虚接口

	// 获得HTTP请求的数据体时的回调接口，注意该函数的调用空间与其它函数不在同
	// 一个线程空间，所以如果该函数访问与其它函数相同的资源时需要注意互斥
	virtual const acl::string* get_body();
	// 正常读到 HTTP 响应头时的回调接口
	virtual void on_hdr(const char* addr, const HTTP_HDR_RES* hdr);
	// 正常读 HTTP 响应体时的回调函数
	virtual void on_body(const char* data, size_t dlen);
	// 当请求或响应失败时的回调函数
	virtual void on_error(acl::http_status_t errnum);
private:
	acl::ofstream out_;
	acl::aio_handle* handle_;
#ifdef WIN32
	HWND hWnd_;
#endif
	http_off_t  read_length_;
	http_off_t  content_length_;
	time_t begin_;
};
