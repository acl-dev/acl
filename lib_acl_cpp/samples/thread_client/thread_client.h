#pragma once

class thread_client : public acl::thread
{
public:
	thread_client(const char* server_addr, bool keep_alive,
		int count, int length);
	~thread_client();

protected:
	virtual void* run();		// 基类虚函数，在子线程中被调用

private:
	acl::string server_addr_;	// 服务器地址
	bool  keep_alive_;		// 是否采用长连接方式
	int   count_;			// IO 会话次数
	int   length_;			// 每次 IO 的数据长度
};
