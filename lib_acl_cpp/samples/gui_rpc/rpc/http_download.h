#pragma once

//////////////////////////////////////////////////////////////////////////

// 纯虚类，子类须实现该类中的纯虚接口
class rpc_callback
{
public:
	rpc_callback() {}
	virtual ~rpc_callback() {}

	// 设置 HTTP 请求头数据虚函数
	virtual void SetRequestHdr(const char* hdr) = 0;
	// 设置 HTTP 响应头数据虚函数
	virtual void SetResponseHdr(const char* hdr) = 0;
	// 下载过程中的回调函数虚函数
	virtual void OnDownloading(long long int content_length,
		long long int total_read) = 0;
	// 下载完成时的回调函数虚函数
	virtual void OnDownloadOver(long long int total_read,
		double spent) = 0;
};

//////////////////////////////////////////////////////////////////////////

/**
 * http 请求过程类，该类对象在子线程中发起远程 HTTP 请求过程，将处理结果
 * 返回给主线程
 */
class http_download : public acl::rpc_request
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} HTTP 服务器地址，格式：domain:port
	 * @param url {const char*} http url 地址
	 * @param callback {rpc_callback*} http 请求结果通过此类对象
	 *  通知主线程过程
	 */
	http_download(const char* addr, const char* url,
		rpc_callback* callback);
protected:
	~http_download() {}

	// 基类虚函数：子线程处理函数
	virtual void rpc_run();

	// 基类虚函数：主线程处理过程，收到子线程任务完成的消息
	virtual void rpc_onover();

	// 基类虚函数：主线程处理过程，收到子线程的通知消息
	virtual void rpc_wakeup(void* ctx);
private:
	acl::string addr_;
	acl::string url_;
	acl::string req_hdr_;
	acl::string res_hdr_;
	bool  error_;
	long long int total_read_;
	long long int content_length_;
	double total_spent_;

	rpc_callback* callback_;
};
