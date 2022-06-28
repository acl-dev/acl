#include "stdafx.h"
#include "status/HttpClient.h"
#include "status/HttpClientRpc.h"

/////////////////////////////////////////////////////////////////////////////

HttpClientRpc::HttpClientRpc(acl::string* buf, const char* server_addrs)
: buf_(buf)
, server_addrs_(server_addrs)
{
}

HttpClientRpc::~HttpClientRpc()
{
	delete buf_;
}

/////////////////////////////////////////////////////////////////////////////

// 子线程中运行
void HttpClientRpc::rpc_run()
{
	std::vector<acl::string>& servers = server_addrs_.split2(";, \t");
	size_t size = servers.size();
	if (size == 0)
	{
		logger_error("invalid server_addrs: %s", server_addrs_.c_str());
		return;
	}
	else if (size == 1)
	{
		// 当只有一个管理服务器时，则不必启动线程，直接在本线程中处理
		HttpClient job(servers[0], buf_);
		job.run();
		return;
	}

	// 当需要向多个管理服务，则为了保证实时性，则将发送任务放在线程池
	// 中去处理
	acl::thread_pool threads;

	// 启动线程池过程
	threads.start();

	// 将发送任务交给线程池处理
	std::vector<acl::string>::const_iterator cit = servers.begin();
	for (; cit != servers.end(); ++cit)
	{
		HttpClient* job = new HttpClient((*cit).c_str(), buf_);
		job->set_auto_free(true);
		threads.execute(job);
	}

	// 等待所有线程发送完毕
	threads.wait();
}


/////////////////////////////////////////////////////////////////////////////

// 主线程中运行
void HttpClientRpc::rpc_onover()
{
	delete this;
}
