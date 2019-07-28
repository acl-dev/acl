#include "stdafx.h"
#include <assert.h>
#include "http_download.h"

// 由子线程动态创建的 DOWN_CTX 对象的数据类型
typedef enum
{
	CTX_T_REQ_HDR,		// 为 HTTP 请求头数据
	CTX_T_RES_HDR,		// 为 HTTP 响应头数据
	CTX_T_CONTENT_LENGTH,	// 为 HTTP 响应体的长度
	CTX_T_PARTIAL_LENGTH,	// 为 HTTP 下载数据体的长度
	CTX_T_END
} ctx_t;

// 子线程动态创建的数据对象，主线程接收此数据
struct DOWN_CTX 
{
	ctx_t type;
	long long int length;
};

// 用来精确计算时间截间隔的函数，精确到毫秒级别
static double stamp_sub(const struct timeval *from,
	const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0)
	{
		--res.tv_sec;
		res.tv_usec += 1000000;
	}

	res.tv_sec -= sub_by->tv_sec;
	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

//////////////////////////////////////////////////////////////////////////

// 子线程处理函数
void http_download::rpc_run()
{
	acl::http_request req(addr_);  // HTTP 请求对象
	// 设置 HTTP 请求头信息
	req.request_header().set_url(url_.c_str())
		.set_content_type("text/html")
		.set_host(addr_.c_str())
		.set_method(acl::HTTP_METHOD_GET);

	req.request_header().build_request(req_hdr_);
	DOWN_CTX* ctx = new DOWN_CTX;
	ctx->type = CTX_T_REQ_HDR;
	rpc_signal(ctx);  // 通知主线程 HTTP 请求头数据

	struct timeval begin, end;;
	gettimeofday(&begin, NULL);

	// 发送 HTTP 请求数据
	if (req.request(NULL, 0) == false)
	{
		logger_error("send request error");
		error_ = false;
		gettimeofday(&end, NULL);
		total_spent_ = stamp_sub(&end, &begin);
		return;
	}

	// 获得 HTTP 请求的连接对象
	acl::http_client* conn = req.get_client();
	assert(conn);

	(void) conn->get_respond_head(&res_hdr_);
	ctx = new DOWN_CTX;
	ctx->type = CTX_T_RES_HDR;
	rpc_signal(ctx);   // 通知主线程 HTTP 响应头数据

	ctx = new DOWN_CTX;
	ctx->type = CTX_T_CONTENT_LENGTH;
	
	ctx->length = conn->body_length();  // 获得 HTTP 响应数据的数据体长度
	content_length_ = ctx->length;
	rpc_signal(ctx);  // 通知主线程 HTTP 响应体数据长度

	acl::string buf(8192);
	int   real_size;
	while (true)
	{
		// 读 HTTP 响应数据体
		int ret = req.read_body(buf, true, &real_size);
		if (ret <= 0)
		{
			ctx = new DOWN_CTX;
			ctx->type = CTX_T_END;
			ctx->length = ret;
			rpc_signal(ctx);  // 通知主线程下载完毕
			break;
		}
		ctx = new DOWN_CTX;
		ctx->type = CTX_T_PARTIAL_LENGTH;
		ctx->length = real_size;
		// 通知主线程当前已经下载的大小
		rpc_signal(ctx);
	}

	// 计算下载过程总时长
	gettimeofday(&end, NULL);
	total_spent_ = stamp_sub(&end, &begin);

	// 至此，子线程运行完毕，主线程的 rpc_onover 过程将被调用
}

//////////////////////////////////////////////////////////////////////////

http_download::http_download(const char* addr, const char* url,
	rpc_callback* callback)
	: addr_(addr)
	, url_(url)
	, callback_(callback)
	, error_(false)
	, total_read_(0)
	, content_length_(0)
	, total_spent_(0)
{

}

//////////////////////////////////////////////////////////////////////////

// 主线程处理过程，收到子线程任务完成的消息
void http_download::rpc_onover()
{
	logger("http download(%s) over, 共 %I64d 字节，耗时 %.3f 毫秒",
		url_.c_str(), total_read_, total_spent_);
	callback_->OnDownloadOver(total_read_, total_spent_);
	delete this;  // 销毁本对象
}

// 主线程处理过程，收到子线程的通知消息
void http_download::rpc_wakeup(void* ctx)
{
	DOWN_CTX* down_ctx = (DOWN_CTX*) ctx;

	// 根据子线程中传来的不同的下载阶段进行处理

	switch (down_ctx->type)
	{
	case CTX_T_REQ_HDR:
		callback_->SetRequestHdr(req_hdr_.c_str());
		break;
	case CTX_T_RES_HDR:
		callback_->SetResponseHdr(res_hdr_.c_str());
		break;
	case CTX_T_CONTENT_LENGTH:
		break;
	case CTX_T_PARTIAL_LENGTH:
		total_read_ += down_ctx->length;
		callback_->OnDownloading(content_length_, total_read_);
		break;
	case CTX_T_END:
		logger("%s: read over", addr_.c_str());
		break;
	default:
		logger_error("%s: ERROR", addr_.c_str());
		break;
	}

	// 删除在子线程中动态分配的对象
	delete down_ctx;
}

//////////////////////////////////////////////////////////////////////////
