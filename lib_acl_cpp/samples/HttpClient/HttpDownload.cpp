#include "StdAfx.h"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stdlib/string.hpp"

#include ".\httpdownload.h"

CHttpDownload::CHttpDownload(const char* domain, unsigned short port,
	acl::aio_handle* handle)
: acl::http_service_request(domain, port)
, handle_(handle)
{
	read_length_ = 0;
#ifdef WIN32
	hWnd_ = 0;
#endif
}

CHttpDownload::~CHttpDownload(void)
{
#ifdef WIN32
	acl::aio_handle_type handle_type = handle_->get_engine_type();
	if (handle_type == acl::ENGINE_WINMSG)
	{
		ASSERT(hWnd_);
		::PostMessage(hWnd_, WM_USER_DOWNLOAD_OVER, 0, 0);
	}
	else
		handle_->stop();
#else
	// 通知异步事件引擎完全退出
	handle_->stop();
#endif
}

void CHttpDownload::destroy()
{
	delete this;
}

#ifdef WIN32
void CHttpDownload::SetHWnd(HWND hWnd)
{
	hWnd_ = hWnd;
}
#endif

const acl::string* CHttpDownload::get_body()
{
	return (NULL);
}

void CHttpDownload::on_hdr(const char* addr, const HTTP_HDR_RES* hdr)
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

void CHttpDownload::on_body(const char* data, size_t dlen)
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
	printf("%I64d%%\r", n);
#else
	printf("%lld%%\r", n);
#endif

	if (out_.opened())
		out_.write(data, dlen);
}

void CHttpDownload::on_error(acl::http_status_t errnum)
{
	printf(">> error: %d\n", (int) errnum);
}
