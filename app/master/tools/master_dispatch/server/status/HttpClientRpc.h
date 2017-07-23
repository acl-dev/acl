#pragma once

class HttpClientRpc : public acl::rpc_request
{
public:
	HttpClientRpc(acl::string* buf, const char* server_addrs);

protected:
	// 实现基类虚函数

	// 在子线程开始运行
	void rpc_run();

	// 当 rpc_run 返回后在主线程中运行
	void rpc_onover();

private:
	acl::string* buf_;
	acl::string  server_addrs_;

	~HttpClientRpc();
};
