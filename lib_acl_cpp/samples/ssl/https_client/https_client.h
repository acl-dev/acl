#pragma once

class https_client : public acl::thread
{
public:
	https_client(const char* server_addr, const char* domain,
		bool keep_alive, int count, int length);
	~https_client();

	void set_ssl_conf(acl::sslbase_conf* conf);

protected:
	virtual void* run();		// 基类虚函数，在子线程中被调用

private:
	acl::string server_addr_;	// 服务器地址
	acl::string domain_;		// 域名
	bool  keep_alive_;		// 是否采用长连接方式
	int   count_;			// IO 会话次数
	int   length_;			// 每次 IO 的数据长度
	acl::sslbase_conf* ssl_conf_;

	bool connect_server(acl::http_client& client);
	int http_request(int count);
};
